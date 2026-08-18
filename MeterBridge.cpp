#include "MeterBridge.hpp"

#include <QTcpSocket>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#include <QGuiApplication>
#endif

#if defined(Q_OS_IOS)
void iosSetIdleTimerDisabled(bool disabled);
#endif

MeterBridge::MeterBridge(QObject* parent)
    : QObject(parent)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_lastHost = m_settings.value(QStringLiteral("catHost")).toString();
    m_lastPort = m_settings.value(QStringLiteral("catPort"), 4533).toInt();
    m_rigMetersOn = m_settings.value(QStringLiteral("rigMetersOn"), true).toBool();
    m_keepScreenOn = m_settings.value(QStringLiteral("keepScreenOn"), true).toBool();
    m_catStatus = tr("CAT non connesso");

    // Un solo ciclo, e veloce. Il PTT e i tre livelli si chiedono INSIEME, in
    // una sola scrittura: cosi' fra il momento in cui parte la portante e il
    // momento in cui l'ago si muove passa il tempo di un giro, non la somma
    // di due attese. La prima versione interrogava il PTT una volta al
    // secondo e solo DOPO averlo visto alto cominciava a chiedere i livelli:
    // fino a 1,35 s di ritardo, che su uno strumento da guardare MENTRE si
    // trasmette lo rende inutile.
    //
    // Interrogare spesso non costa: il server legge dalla propria memoria e
    // risponde in circa 3 ms senza toccare la seriale della radio.
    //
    // 150 ms e' scelto per l'occhio: sotto questa soglia il movimento
    // dell'ago si legge come continuo, sopra comincia a sembrare a scatti.
    m_poll.setInterval(150);
    connect(&m_poll, &QTimer::timeout, this, &MeterBridge::onPoll);

    applyKeepScreenOn();

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

// Schermo sempre acceso finche' l'app e' in primo piano. E' il flag della
// finestra dell'Activity (FLAG_KEEP_SCREEN_ON), non un wake lock: lo rilascia
// Android da solo quando l'app va in background, quindi non puo' restare
// incastrato e non consuma batteria a schermo spento. Va impostato sul thread
// UI di Android.
void MeterBridge::applyKeepScreenOn()
{
#if defined(Q_OS_IOS)
    iosSetIdleTimerDisabled(m_keepScreenOn);
#elif defined(Q_OS_ANDROID)
    bool const on = m_keepScreenOn;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([on] {
        QJniObject act = QNativeInterface::QAndroidApplication::context();
        if (!act.isValid()) return;
        QJniObject win = act.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!win.isValid()) return;
        constexpr jint kFlagKeepScreenOn = 128;   // WindowManager.LayoutParams
        win.callMethod<void>(on ? "addFlags" : "clearFlags", "(I)V", kFlagKeepScreenOn);
    });
#endif
}

void MeterBridge::setKeepScreenOn(bool on)
{
    if (m_keepScreenOn == on) return;
    m_keepScreenOn = on;
    m_settings.setValue(QStringLiteral("keepScreenOn"), on);
    applyKeepScreenOn();
    emit keepScreenOnChanged();
}

void MeterBridge::setRigMetersOn(bool on)
{
    if (m_rigMetersOn == on) return;
    m_rigMetersOn = on;
    m_settings.setValue(QStringLiteral("rigMetersOn"), on);
    if (!on) resetTxMeters();
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
    // Niente attesa di Nagle: i comandi sono corti e vanno spediti adesso, non
    // quando il buffer si riempie. Su un misuratore quei millisecondi si
    // vedono.
    m_cat->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_cat, &QTcpSocket::readyRead, this, &MeterBridge::onCatReadyRead);
    connect(m_cat, &QTcpSocket::connected, this, [this, host, port] {
        m_catConnected = true;
        m_catStatus = tr("CAT connesso a %1:%2").arg(host).arg(port);
        emit catChanged();
        m_poll.start();
        onPoll();             // subito, senza aspettare il primo giro del timer
    });
    connect(m_cat, &QTcpSocket::disconnected, this, [this] {
        m_poll.stop();
        if (m_catConnected) {
            m_catConnected = false;
            m_catStatus = tr("CAT disconnesso");
            resetTxMeters();
            emit catChanged();
        }
    });
    connect(m_cat, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_poll.stop();
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
    m_poll.stop();
    if (m_cat) { m_cat->abort(); m_cat->deleteLater(); m_cat = nullptr; }
    m_catBuf.clear();
    m_livelloAtteso.clear();
    if (m_catConnected) {
        m_catConnected = false;
        m_catStatus = tr("CAT non connesso");
        resetTxMeters();
        emit catChanged();
    }
}

void MeterBridge::onPoll()
{
    if (!m_cat || m_cat->state() != QAbstractSocket::ConnectedState)
        return;

    // Una sola write con tutto dentro: quattro comandi in un pacchetto invece
    // di quattro scambi separati. I livelli si chiedono anche a trasmettitore
    // fermo, e non e' uno spreco: e' proprio cosi' che il primo valore arriva
    // INSIEME al primo "PTT alto" invece che un giro dopo. A riposo il server
    // risponde "non disponibile" e la risposta e' di pochi byte.
    if (m_rigMetersOn)
        m_cat->write("t\n"
                     "+\\get_level RFPOWER_METER_WATTS\n"
                     "+\\get_level SWR\n"
                     "+\\get_level ALC\n");
    else
        m_cat->write("t\n");
}

void MeterBridge::setPttState(bool active)
{
    if (m_rigPtt == active) return;
    m_rigPtt = active;
    if (!active) resetTxMeters();
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
        if (line.isEmpty()) continue;

        if (line.startsWith("RPRT")) {
            // Errore riferito al livello appena annunciato: la misura non c'e'
            // (a riposo, o perche' la radio non la fornisce). Si dimentica
            // l'attesa, altrimenti il prossimo valore finirebbe nel campo
            // sbagliato.
            m_livelloAtteso.clear();
            continue;
        }

        // Risposte estese di \get_level (dal server Decodium 1.0.565): due
        // righe, prima il nome del livello e poi il valore. La riga del valore,
        // da sola, non dice a cosa si riferisce: si tiene da parte il nome.
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
            }
            m_livelloAtteso.clear();
            continue;
        }

        // La sola risposta rimasta e' quella al poll del PTT ("t"): "0" o "1".
        // Si accetta solo se NON si sta aspettando il valore di un livello,
        // altrimenti un livello che valesse esattamente 0 o 1 verrebbe
        // scambiato per lo stato del trasmettitore.
        if (m_livelloAtteso.isEmpty() && (line == "0" || line == "1")) {
            bool const attivo = (line == "1");
            if (m_rigPtt != attivo) {
                setPttState(attivo);
                changed = true;
            }
        }
    }
    if (changed) emit rigCtlChanged();
}
