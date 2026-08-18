#include "MeterBridge.hpp"

#include <QTcpSocket>

MeterBridge::MeterBridge(QObject* parent)
    : QObject(parent)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_lastHost = m_settings.value(QStringLiteral("catHost")).toString();
    m_lastPort = m_settings.value(QStringLiteral("catPort"), 4533).toInt();
    m_rigMetersOn = m_settings.value(QStringLiteral("rigMetersOn"), true).toBool();
    m_catStatus = tr("CAT non connesso");

    // Il PTT si interroga sempre, ogni secondo: e' l'unico modo di sapere
    // quando cominciare (e smettere) a chiedere i tre misuratori. Chiederli
    // anche a riposo saturerebbe il server per niente: a riposo non
    // misurano nulla, e il server condiviso lo dice gia' da solo (RPRT -11).
    m_pttPoll.setInterval(1000);
    connect(&m_pttPoll, &QTimer::timeout, this, &MeterBridge::onPttPoll);

    // I livelli si interrogano piu' spesso SOLO mentre si trasmette: chi
    // guarda il quadrante durante un over vuole vederlo muoversi, non
    // aggiornarsi una volta al secondo come la frequenza.
    m_levelPoll.setInterval(350);
    connect(&m_levelPoll, &QTimer::timeout, this, [this] {
        if (!m_catConnected || !m_rigMetersOn || !m_rigPtt) return;
        if (m_cat && m_cat->state() == QAbstractSocket::ConnectedState) {
            m_cat->write("+\\get_level RFPOWER_METER_WATTS\n");
            m_cat->write("+\\get_level SWR\n");
            m_cat->write("+\\get_level ALC\n");
        }
    });

    // Chi ha gia' collegato una volta si aspetta di riaprire l'app e trovare
    // il quadrante vivo, non una schermata di rete da ricompilare ogni volta:
    // e' un misuratore, si guarda mentre si trasmette. Il tentativo parte
    // appena il ciclo degli eventi gira, cosi' il QML e' gia' in piedi e vede
    // cambiare lo stato invece di perderselo.
    if (!m_lastHost.isEmpty()) {
        QTimer::singleShot(0, this, [this] { catConnect(m_lastHost, m_lastPort); });
    }
}

MeterBridge::~MeterBridge()
{
    catDisconnect();
}

void MeterBridge::setRigMetersOn(bool on)
{
    if (m_rigMetersOn == on) return;
    m_rigMetersOn = on;
    m_settings.setValue(QStringLiteral("rigMetersOn"), on);
    if (!on) {
        m_levelPoll.stop();
        resetTxMeters();
    } else if (m_rigPtt && m_catConnected) {
        m_levelPoll.start();   // riattivato a meta' di un over: riparte subito
    }
    emit rigCtlChanged();
}

void MeterBridge::catConnect(const QString& host, int port)
{
    catDisconnect();
    if (host.isEmpty() || port <= 0 || port > 65535) {
        m_catStatus = tr("Indirizzo non valido");
        emit catChanged();
        return;
    }

    m_lastHost = host;
    m_lastPort = port;
    m_settings.setValue(QStringLiteral("catHost"), host);
    m_settings.setValue(QStringLiteral("catPort"), port);
    emit lastEndpointChanged();

    m_cat = new QTcpSocket(this);
    connect(m_cat, &QTcpSocket::readyRead, this, &MeterBridge::onCatReadyRead);
    connect(m_cat, &QTcpSocket::connected, this, [this, host, port] {
        m_catConnected = true;
        m_catStatus = tr("CAT connesso a %1:%2").arg(host).arg(port);
        emit catChanged();
        m_pttPoll.start();
        onPttPoll();          // subito, senza aspettare il primo giro del timer
    });
    connect(m_cat, &QTcpSocket::disconnected, this, [this] {
        m_pttPoll.stop();
        m_levelPoll.stop();
        if (m_catConnected) {
            m_catConnected = false;
            m_catStatus = tr("CAT disconnesso");
            resetTxMeters();
            emit catChanged();
        }
    });
    connect(m_cat, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_pttPoll.stop();
        m_levelPoll.stop();
        m_catConnected = false;
        m_catStatus = tr("CAT errore: %1").arg(m_cat ? m_cat->errorString() : QString());
        resetTxMeters();
        emit catChanged();
    });

    m_catStatus = tr("CAT: connessione a %1:%2…").arg(host).arg(port);
    emit catChanged();
    m_cat->connectToHost(host, quint16(port));
}

void MeterBridge::catDisconnect()
{
    m_pttPoll.stop();
    m_levelPoll.stop();
    if (m_cat) { m_cat->abort(); m_cat->deleteLater(); m_cat = nullptr; }
    m_catBuf.clear();
    if (m_catConnected) {
        m_catConnected = false;
        m_catStatus = tr("CAT non connesso");
        resetTxMeters();
        emit catChanged();
    }
}

void MeterBridge::onPttPoll()
{
    if (m_cat && m_cat->state() == QAbstractSocket::ConnectedState)
        m_cat->write("t\n");
}

void MeterBridge::setPttState(bool active)
{
    if (m_rigPtt == active) return;
    m_rigPtt = active;
    if (active) {
        if (m_rigMetersOn) m_levelPoll.start();
    } else {
        m_levelPoll.stop();
        resetTxMeters();
    }
    emit rigCtlChanged();
}

void MeterBridge::resetTxMeters()
{
    // Tornati a riposo i misuratori di trasmissione non hanno piu' niente da
    // dire: l'ultima lettura resterebbe stampata sul quadrante. Per il ROS
    // vuol dire tornare a 1.0, non a zero, perche' sotto uno non esiste.
    if (m_rigWatt != 0.0 || m_rigRos != 1.0 || m_rigAlc != 0) {
        m_rigWatt = 0.0;
        m_rigRos = 1.0;
        m_rigAlc = 0;
        emit rigCtlChanged();
    }
}

void MeterBridge::onCatReadyRead()
{
    if (!m_cat) return;
    parseCatLines(m_cat->readAll());
}

// Interpreta le righe di risposta del server CAT condiviso.
void MeterBridge::parseCatLines(const QByteArray& data)
{
    m_catBuf += data;
    bool changed = false;
    int nl;
    while ((nl = m_catBuf.indexOf('\n')) >= 0) {
        QByteArray const line = m_catBuf.left(nl).trimmed();
        m_catBuf.remove(0, nl + 1);
        if (line.isEmpty() || line.startsWith("RPRT")) continue;   // ack/errore semplice

        // Risposte estese di \get_level (fix 1.0.565 sul server condiviso):
        // due righe, prima il nome del livello e poi il valore. La riga del
        // valore, da sola, non dice a cosa si riferisce: si tiene da parte
        // il nome della prima.
        if (line.startsWith("get_level:")) {
            m_livelloAtteso = QString::fromLatin1(line.mid(10)).trimmed();
            continue;
        }
        if (line.startsWith("Level Value:")) {
            bool okv = false;
            double const val = QString::fromLatin1(line.mid(12)).trimmed().toDouble(&okv);
            if (okv) {
                if (m_livelloAtteso == QLatin1String("RFPOWER_METER_WATTS")) {
                    m_rigWatt = val; changed = true;
                } else if (m_livelloAtteso == QLatin1String("SWR")) {
                    m_rigRos = val >= 1.0 ? val : 1.0; changed = true;
                } else if (m_livelloAtteso == QLatin1String("ALC")) {
                    // Hamlib lo da' normalizzato 0..1: si riporta sulla scala
                    // 0-255 che il frontalino si aspetta, la stessa dell'ago
                    // fisico del rig.
                    m_rigAlc = qBound(0, qRound(val * 255.0), 255); changed = true;
                }
                m_meterVeri = true;
                emit rigCtlChanged();
            }
            m_livelloAtteso.clear();
            continue;
        }

        // La sola risposta rimasta e' quella al poll del PTT ("t"): "0" o "1".
        if (line == "0" || line == "1") {
            setPttState(line == "1");
        }
    }
    if (changed) emit rigCtlChanged();
}
