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
    // 80 ms: piu' fitto della cadenza con cui il dato cambia davvero, ed e'
    // voluto. Decodium legge la radio ogni 250 ms in trasmissione (dalla
    // 1.0.566), e chi interroga a passo uguale prende in media mezzo periodo
    // di ritardo solo per essersi trovato fuori fase. Chiedere piu' spesso di
    // quanto il dato cambi costa qualche byte in rete e toglie quel ritardo:
    // il valore nuovo viene raccolto entro 80 ms da quando esiste.
    m_poll.setInterval(80);
    connect(&m_poll, &QTimer::timeout, this, &MeterBridge::onPoll);

    // La linea cade: il WiFi vacilla, il telefono passa a un altro access
    // point, il PC va in sospensione. Prima da li' non si tornava piu' da
    // soli — il quadrante restava spento e bisognava rientrare nelle
    // impostazioni e ripremere Connetti, mentre la radio magari trasmetteva.
    // Adesso ci riprova da se' finche' l'utente non chiede di staccare.
    m_ritenta.setSingleShot(true);
    connect(&m_ritenta, &QTimer::timeout, this, [this] {
        if (!m_vuoleConnesso) return;
        chiudiSocket();
        avviaConnessione();
    });

    // Il tentativo che non finisce mai: senza questa il ritentativo non
    // partirebbe mai, perche' un tentativo formalmente ancora in corso non
    // e' un errore.
    m_attesaConn.setSingleShot(true);
    m_attesaConn.setInterval(kTimeoutConn);
    connect(&m_attesaConn, &QTimer::timeout, this, [this] {
        if (!m_cat || m_cat->state() == QAbstractSocket::ConnectedState) return;
        m_catStatus = tr("CAT: nessuna risposta da %1:%2").arg(m_lastHost).arg(m_lastPort);
        emit catChanged();
        programmaRitentativo();
    });

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

    // Da qui in avanti il collegamento e' voluto: se cade, si riprende da se'.
    m_vuoleConnesso = true;
    m_ritardoRitentativo = kRitardoMin;
    avviaConnessione();
}

// Il tentativo vero e proprio, senza toccare ne' l'intenzione dell'utente ne'
// le impostazioni salvate: e' quello che rifa' il ritentativo a ogni giro.
void MeterBridge::avviaConnessione()
{
    m_cat = new QTcpSocket(this);
    // Niente attesa di Nagle: i comandi sono corti e vanno spediti adesso, non
    // quando il buffer si riempie. Su un misuratore quei millisecondi si
    // vedono.
    m_cat->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_cat, &QTcpSocket::readyRead, this, &MeterBridge::onCatReadyRead);
    connect(m_cat, &QTcpSocket::connected, this, [this] {
        m_attesaConn.stop();
        m_ritenta.stop();
        m_ritardoRitentativo = kRitardoMin;   // la prossima caduta riparte svelta
        m_catConnected = true;
        m_catStatus = tr("CAT connesso a %1:%2").arg(m_lastHost).arg(m_lastPort);
        emit catChanged();
        m_pollInVolo = false;
        m_ultimaRisposta.start();
        m_poll.start();
        onPoll();             // subito, senza aspettare il primo giro del timer
    });
    connect(m_cat, &QTcpSocket::disconnected, this, [this] {
        m_poll.stop();
        m_attesaConn.stop();
        if (m_catConnected) {
            m_catConnected = false;
            m_catStatus = tr("CAT disconnesso");
            resetTxMeters();
            emit catChanged();
        }
        programmaRitentativo();
    });
    connect(m_cat, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_poll.stop();
        m_attesaConn.stop();
        m_catConnected = false;
        m_catStatus = tr("CAT errore: %1").arg(m_cat ? m_cat->errorString() : QString());
        resetTxMeters();
        emit catChanged();
        programmaRitentativo();
    });

    m_catStatus = m_ritardoRitentativo > kRitardoMin
                      ? tr("CAT: riconnessione a %1:%2…").arg(m_lastHost).arg(m_lastPort)
                      : tr("CAT: connessione a %1:%2…").arg(m_lastHost).arg(m_lastPort);
    emit catChanged();
    m_attesaConn.start();
    m_cat->connectToHost(m_lastHost, quint16(m_lastPort));
}

// Chiude il socket senza rinunciare a riprovare: la usano il ritentativo e
// lo stacco voluto, che pero' prima azzera l'intenzione.
void MeterBridge::chiudiSocket()
{
    m_poll.stop();
    m_attesaConn.stop();
    m_pollInVolo = false;
    m_ultimaRisposta.invalidate();
    if (m_cat) {
        // Prima si staccano i segnali, poi si abortisce: altrimenti abort()
        // fa scattare disconnected/errorOccurred e il gestore programmerebbe
        // un ritentativo per una chiusura che abbiamo deciso noi.
        m_cat->disconnect(this);
        m_cat->abort();
        m_cat->deleteLater();
        m_cat = nullptr;
    }
    m_catBuf.clear();
    m_livelloAtteso.clear();
}

void MeterBridge::programmaRitentativo()
{
    if (!m_vuoleConnesso || m_lastHost.isEmpty()) return;
    // Una caduta sola annuncia se stessa due volte — errorOccurred e poi
    // disconnected — e senza questa guardia il ritardo raddoppierebbe due
    // volte per un solo inciampo.
    if (m_ritenta.isActive()) return;
    m_ritenta.start(m_ritardoRitentativo);
    m_ritardoRitentativo = qMin(m_ritardoRitentativo * 2, kRitardoMax);
}

void MeterBridge::catDisconnect()
{
    // Stacco VOLUTO: si smette anche di riprovare.
    bool const stavaProvando = m_vuoleConnesso;
    m_vuoleConnesso = false;
    m_ritenta.stop();
    m_ritardoRitentativo = kRitardoMin;
    chiudiSocket();
    // Anche senza essere mai arrivati a connettersi la riga di stato puo'
    // essere ferma su "riconnessione…": lasciarla li' direbbe che si sta
    // ancora provando, che e' esattamente cio' che non succede piu'.
    if (m_catConnected || stavaProvando) {
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

    // Il giro precedente non ha risposto: non se ne manda un altro sopra.
    // Senza questa guardia, su una rete che rallenta le domande si
    // accumulerebbero a dodici al secondo e le risposte arriverebbero con un
    // ritardo che cresce da solo: l'ago indicherebbe il passato, e con l'aria
    // di funzionare benissimo.
    if (m_pollInVolo) {
        // Muto da troppo tempo pur avendo chiesto: il socket e' vivo solo per
        // il sistema operativo. Si taglia e si ricomincia, che e' l'unica cosa
        // che rimette in moto il quadrante.
        if (m_ultimaRisposta.isValid() && m_ultimaRisposta.elapsed() > kSilenzioMax) {
            m_catStatus = tr("CAT muto: riconnessione…");
            m_catConnected = false;
            resetTxMeters();
            emit catChanged();
            chiudiSocket();
            programmaRitentativo();
        }
        return;
    }

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

    m_pollInVolo = true;
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
    // meterVeri torna falso insieme ai valori: e' la bandiera che dice "questi
    // numeri vengono da una lettura vera", e lasciarla alzata su valori
    // azzerati significherebbe dichiarare misurato uno zero che nessuno ha
    // misurato.
    if (m_rigWatt != 0.0 || m_rigRos != 1.0 || m_rigAlc != 0 || m_meterVeri) {
        m_rigWatt = 0.0;
        m_rigRos = 1.0;
        m_rigAlc = 0;
        m_meterVeri = false;
        emit rigCtlChanged();
    }
}

void MeterBridge::onCatReadyRead()
{
    if (!m_cat) return;
    // Qualunque cosa sia arrivata, il server e' vivo e il giro e' chiuso: il
    // prossimo puo' partire.
    m_pollInVolo = false;
    m_ultimaRisposta.restart();
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
            // setPttState emette gia' di suo quando lo stato cambia davvero:
            // segnare anche 'changed' farebbe partire due volte la stessa
            // notifica, e con essa tutte le rivalutazioni del quadrante.
            setPttState(line == "1");
        }
    }
    if (changed) emit rigCtlChanged();
}
