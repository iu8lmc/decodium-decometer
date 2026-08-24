#include "MeterBridge.hpp"

#include "DecoPortLink.h"

#include <QVariantList>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#include <QGuiApplication>
#endif

#if defined(Q_OS_IOS)
void iosSetIdleTimerDisabled(bool disabled);
void iosVibra();
void iosSafeAreaInsets(double* top, double* bottom, double* left, double* right);
#endif

MeterBridge::MeterBridge(QObject* parent)
    : QObject(parent)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_lastHost = m_settings.value(QStringLiteral("catHost")).toString();
    m_lastPort = m_settings.value(QStringLiteral("catPort"), 5559).toInt();
    m_authKey = m_settings.value(QStringLiteral("authKey")).toString();
    m_rigMetersOn = m_settings.value(QStringLiteral("rigMetersOn"), true).toBool();
    m_keepScreenOn = m_settings.value(QStringLiteral("keepScreenOn"), true).toBool();
    m_catStatus = tr("CAT non connesso");

    // Il collegamento e la scoperta. Due oggetti della libreria, e nessun
    // ciclo di interrogazione qui dentro: con DecoPort e' il gateway a mandare
    // il contesto quando cambia, invece di essere il telefono a chiedere dodici
    // volte al secondo. Sul telefono la differenza si sente sulla batteria,
    // sulla radio si sente sul bus seriale.
    m_link = new DecoPortLink(this);
    connect(m_link, &DecoPortLink::stateChanged, this, &MeterBridge::onLinkState);
    connect(m_link, &DecoPortLink::linkedChanged, this, &MeterBridge::onLinkLinked);

    // La scoperta parte sempre, anche prima di essere collegati: e' cosi' che
    // la schermata di rete puo' proporre le radio invece di chiedere un
    // indirizzo IP a memoria. Senza chiave non mostra niente, e va bene cosi'.
    m_scoperta = new DecoPortDiscovery(this);
    connect(m_scoperta, &DecoPortDiscovery::radiosChanged,
            this, &MeterBridge::radiosTrovateChanged);
    if (!m_authKey.isEmpty()) {
        QByteArray const chiave = decoport::deriveKeyFromPassword(m_authKey);
        m_link->setAuthKey(chiave);
        m_scoperta->setAuthKey(chiave);
    }
    m_scoperta->start();

    // La linea cade: il WiFi vacilla, il telefono passa a un altro access
    // point, il PC va in sospensione. Ci si riprova da se' finche' l'utente non
    // chiede di staccare.
    m_ritenta.setSingleShot(true);
    connect(&m_ritenta, &QTimer::timeout, this, [this] {
        if (!m_vuoleConnesso) return;
        chiudiSocket();
        avviaConnessione();
    });

    applyKeepScreenOn();

    // Chi ha gia' collegato una volta si aspetta di riaprire l'app e trovare
    // il quadrante vivo, non una schermata di rete da ricompilare ogni volta:
    // e' un misuratore, si guarda mentre si trasmette. Il tentativo parte
    // appena il ciclo degli eventi gira, cosi' il QML e' gia' in piedi e vede
    // cambiare lo stato invece di perderselo.
    if (!m_lastHost.isEmpty() && !m_authKey.isEmpty()) {
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

// I margini che il sistema si tiene per se': barre di stato e navigazione, e
// l'incavo della fotocamera. Senza toglierli, l'intestazione del quadrante
// finisce sotto l'orologio e il lato corto sotto la barra dei gesti.
//
// TUTTA LA PARTE ANDROID GIRA SUL THREAD INTERFACCIA DI ANDROID, non su
// quello di Qt. getWindow(), getDecorView() e getRootWindowInsets() sono
// metodi di View, e le View di Android si toccano SOLO dal loro thread.
// Chiamarli dal thread di Qt — dove arrivano i cambi di dimensione e il timer
// che rilegge i margini — non da' un errore: corrompe lo stato del renderer, e
// il processo muore poco dopo con "pthread_mutex_lock called on a destroyed
// mutex". Dal telefono si vede l'app che si apre e si richiude subito, senza
// un messaggio che spieghi niente. E' una lezione dell'app completa, da cui
// questo codice viene: non la si riscopre due volte.
void MeterBridge::refreshSafeArea()
{
    double t = 0, b = 0, l = 0, r = 0;

#if defined(Q_OS_IOS)
    iosSafeAreaInsets(&t, &b, &l, &r);
#elif defined(Q_OS_ANDROID)
    // getInsets(int) esiste dall'API 30. Sotto quella soglia il sistema rientra
    // ancora la finestra da se', quindi zero e' la risposta giusta.
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 30)
        return;

    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([this] {
        QJniObject act = QNativeInterface::QAndroidApplication::context();
        if (!act.isValid()) return;
        QJniObject win = act.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!win.isValid()) return;
        QJniObject decor = win.callObjectMethod("getDecorView", "()Landroid/view/View;");
        if (!decor.isValid()) return;
        QJniObject insets = decor.callObjectMethod("getRootWindowInsets",
                                                   "()Landroid/view/WindowInsets;");
        if (!insets.isValid()) return;

        // Barre di sistema E incavo dello schermo: sui telefoni con la
        // fotocamera nel display il secondo e' piu' alto del primo, e fermarsi
        // alle sole barre lascerebbe la riga di stato sotto l'isola.
        jint const tipo =
            QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type",
                                               "systemBars", "()I")
            | QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type",
                                                 "displayCutout", "()I");
        QJniObject in = insets.callObjectMethod("getInsets", "(I)Landroid/graphics/Insets;", tipo);
        if (!in.isValid()) return;

        // Android li da' in pixel fisici, QML ragiona in punti.
        double const dpr = qMax(1.0, qApp->devicePixelRatio());
        double const nt = in.getField<jint>("top")    / dpr;
        double const nb = in.getField<jint>("bottom") / dpr;
        double const nl = in.getField<jint>("left")   / dpr;
        double const nr = in.getField<jint>("right")  / dpr;

        // Il risultato torna al thread di Qt con una chiamata accodata: e'
        // l'unico punto in cui si possono toccare i membri ed emettere.
        QMetaObject::invokeMethod(this, [this, nt, nb, nl, nr] {
            if (qFuzzyCompare(nt + 1.0, m_safeTop + 1.0)
                && qFuzzyCompare(nb + 1.0, m_safeBottom + 1.0)
                && qFuzzyCompare(nl + 1.0, m_safeLeft + 1.0)
                && qFuzzyCompare(nr + 1.0, m_safeRight + 1.0))
                return;
            m_safeTop = nt; m_safeBottom = nb; m_safeLeft = nl; m_safeRight = nr;
            emit safeAreaChanged();
        }, Qt::QueuedConnection);
    });
    return;
#endif

    if (qFuzzyCompare(t + 1.0, m_safeTop + 1.0) && qFuzzyCompare(b + 1.0, m_safeBottom + 1.0)
        && qFuzzyCompare(l + 1.0, m_safeLeft + 1.0) && qFuzzyCompare(r + 1.0, m_safeRight + 1.0))
        return;
    m_safeTop = t; m_safeBottom = b; m_safeLeft = l; m_safeRight = r;
    emit safeAreaChanged();
}

// Un colpo di vibrazione. E' il beep del misuratore da tavolo tradotto per
// una cosa che sta in tasca: chi trasmette dall'altra stanza non guarda lo
// schermo, e un allarme che si vede soltanto non e' un allarme.
void MeterBridge::vibra(int ms)
{
#if defined(Q_OS_ANDROID)
    int const durata = ms;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([durata] {
        QJniObject ctx = QNativeInterface::QAndroidApplication::context();
        if (!ctx.isValid()) return;
        QJniObject nome = QJniObject::fromString(QStringLiteral("vibrator"));
        QJniObject vib = ctx.callObjectMethod("getSystemService",
                                              "(Ljava/lang/String;)Ljava/lang/Object;",
                                              nome.object<jstring>());
        if (!vib.isValid()) return;
        if (!vib.callMethod<jboolean>("hasVibrator", "()Z")) return;
        // vibrate(long) e' deprecato dall'API 26 ma c'e' ancora e funziona
        // fin dalla 24, che e' il minimo di questa app: una VibrationEffect
        // qui aggiungerebbe un ramo per due righe di guadagno.
        vib.callMethod<void>("vibrate", "(J)V", jlong(durata));
    });
#elif defined(Q_OS_IOS)
    Q_UNUSED(ms)   // iOS decide da se' la durata: la vibrazione e' una sola
    iosVibra();
#else
    Q_UNUSED(ms)
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

// ── DecoPort ────────────────────────────────────────────────────────────────
// Il trasporto sta tutto nella libreria: qui dentro resta soltanto la
// traduzione fra il contesto che arriva e i campi che il quadrante si aspetta,
// piu' la volonta' dell'utente di restare collegato.

void MeterBridge::setAuthKey(const QString& k)
{
    QString const pulita = k.trimmed();
    if (pulita == m_authKey) return;
    m_authKey = pulita;
    m_settings.setValue(QStringLiteral("authKey"), m_authKey);
    // La password NON viaggia e non si usa com'e': se ne deriva una chiave, e
    // quella derivazione deve essere identica alle due estremita'. Passando
    // qui il testo grezzo il collegamento falliva in silenzio — i pacchetti
    // arrivavano e venivano scartati uno per uno, senza un errore da nessuna
    // parte, che e' il modo peggiore in cui una cosa possa non funzionare.
    QByteArray const chiave = decoport::deriveKeyFromPassword(m_authKey);
    if (m_link) m_link->setAuthKey(chiave);
    // Anche la scoperta: un annuncio non firmato puo' venire da chiunque, e una
    // radio falsa nell'elenco e' un invito a collegarsi alla macchina sbagliata.
    if (m_scoperta) m_scoperta->setAuthKey(chiave);
    emit lastEndpointChanged();
}

QVariantList MeterBridge::radiosTrovate() const
{
    return m_scoperta ? m_scoperta->radios() : QVariantList();
}

void MeterBridge::catConnect(const QString& host, int port)
{
    catDisconnect();
    if (host.isEmpty() || port <= 0 || port > 65535) {
        m_catStatus = tr("Indirizzo non valido");
        emit catChanged();
        return;
    }
    // DecoPort non ha una modalita' in chiaro, ed e' bene che non l'abbia: la
    // porta espone una radio. Dirlo qui evita il tentativo muto, che finirebbe
    // in "nessuna risposta" senza spiegare che il problema e' la chiave.
    if (m_authKey.isEmpty()) {
        m_catStatus = tr("Manca la chiave: DecoPort non si collega senza");
        emit catChanged();
        return;
    }

    m_lastHost = host;
    m_lastPort = port;
    m_settings.setValue(QStringLiteral("catHost"), host);
    m_settings.setValue(QStringLiteral("catPort"), port);
    emit lastEndpointChanged();

    m_vuoleConnesso = true;
    m_ritardoRitentativo = kRitardoMin;
    avviaConnessione();
}

void MeterBridge::avviaConnessione()
{
    if (!m_link) return;
    m_link->setAuthKey(decoport::deriveKeyFromPassword(m_authKey));
    m_catStatus = m_ritardoRitentativo > kRitardoMin
                      ? tr("CAT: riconnessione a %1:%2...").arg(m_lastHost).arg(m_lastPort)
                      : tr("CAT: connessione a %1:%2...").arg(m_lastHost).arg(m_lastPort);
    emit catChanged();

    if (!m_link->connectTo(m_lastHost, m_lastPort)) {
        m_catStatus = tr("CAT: impossibile aprire la porta locale");
        emit catChanged();
        programmaRitentativo();
    }
}

void MeterBridge::chiudiSocket()
{
    if (m_link) m_link->disconnectFromGateway();
}

void MeterBridge::programmaRitentativo()
{
    if (!m_vuoleConnesso) return;
    m_ritenta.start(m_ritardoRitentativo);
    m_ritardoRitentativo = qMin(kRitardoMax, m_ritardoRitentativo * 2);
}

void MeterBridge::catDisconnect()
{
    // Stacco voluto: si azzera prima l'intenzione, altrimenti il ritentativo
    // gia' programmato rimetterebbe su la linea appena chiusa.
    m_vuoleConnesso = false;
    m_ritenta.stop();
    chiudiSocket();

    if (m_catConnected) {
        m_catConnected = false;
        resetTxMeters();
    }
    m_catStatus = tr("CAT non connesso");
    emit catChanged();
}

// Il collegamento e' salito o caduto. La caduta la dichiara la libreria anche
// quando il socket resta aperto e muto — il gateway smette di mandare contesto
// e dopo qualche secondo il collegamento si considera perso — che e'
// esattamente la caduta tipica del telefono che cambia access point.
void MeterBridge::onLinkLinked()
{
    bool const su = m_link && m_link->isLinked();
    if (su == m_catConnected) return;
    m_catConnected = su;

    if (su) {
        m_ritenta.stop();
        m_ritardoRitentativo = kRitardoMin;   // la prossima caduta riparte svelta
        m_catStatus = m_rigModel.isEmpty()
                          ? tr("CAT connesso a %1").arg(m_link->peerAddress())
                          : tr("CAT connesso a %1 (%2)").arg(m_link->peerAddress(), m_rigModel);
    } else {
        resetTxMeters();
        m_strengthVeri = false;
        m_catStatus = tr("CAT disconnesso");
        programmaRitentativo();
    }
    emit catChanged();
}

// Un contesto e' arrivato. Tutto quello che segue e' traduzione, con una regola
// sola: se il campo non c'e', la sua bandiera va giu'. Il gateway manda un
// contesto completo ogni volta, quindi un campo che sparisce vuol dire che la
// radio ha smesso di darlo, non che il pacchetto era corto.
void MeterBridge::onLinkState()
{
    if (!m_link) return;

    bool cambiato = false;

    if (m_rigModel != m_link->rigLabel()) {
        m_rigModel = m_link->rigLabel();
        emit catChanged();
    }

    double const hz = m_link->frequencyHz();
    if (hz > 0.0 && m_rigFreqHz != hz) { m_rigFreqHz = hz; cambiato = true; }

    // S-meter: vale solo in ricezione, e la sua bandiera arriva dal filo.
    bool const sVeri = m_link->hasSMeter();
    int const sDb = qRound(m_link->sMeterDbm());
    if (m_strengthVeri != sVeri || (sVeri && m_rigStrengthDb != sDb)) {
        m_strengthVeri = sVeri;
        m_rigStrengthDb = sVeri ? sDb : 0;
        cambiato = true;
    }

    // I tre di trasmissione. meterVeri resta la bandiera unica che il quadrante
    // gia' conosce: e' vera quando la potenza c'e', perche' senza quella le
    // altre due non hanno un contesto in cui significare qualcosa.
    if (m_rigMetersOn) {
        bool const pVeri = m_link->hasForwardPower();
        double const w = m_link->forwardPowerW();
        if (m_meterVeri != pVeri || (pVeri && m_rigWatt != w)) {
            m_meterVeri = pVeri;
            m_rigWatt = pVeri ? w : 0.0;
            cambiato = true;
        }
        double const ros = m_link->hasSwr() ? m_link->swr() : 1.0;
        if (m_rigRos != ros) { m_rigRos = ros; cambiato = true; }

        // ALC: sul filo e' una percentuale, sul frontalino la scala 0-255 che
        // l'ago si aspetta. La conversione sta qui e in nessun altro posto.
        int const alc = m_link->hasAlc()
                            ? qBound(0, qRound(m_link->alcPct() * 2.55), 255)
                            : 0;
        if (m_rigAlc != alc) { m_rigAlc = alc; cambiato = true; }
    }

    // Gli strumenti del finale: ognuno con la sua bandiera, nessuna scala da
    // convertire — arrivano nelle unita' in cui si leggono.
    auto const posa = [&cambiato](bool ok, double v, bool& veri, double& dest) {
        if (veri != ok || (ok && dest != v)) {
            veri = ok;
            dest = ok ? v : 0.0;
            cambiato = true;
        }
    };
    posa(m_link->hasDrainVoltage(),  m_link->drainVoltage(),    m_vdVeri,     m_rigVd);
    posa(m_link->hasDrainCurrent(),  m_link->drainCurrent(),    m_idVeri,     m_rigId);
    posa(m_link->hasPaTemperature(), m_link->paTemperature(),   m_tempVeri,   m_rigTemp);
    posa(m_link->hasCompression(),   m_link->compressionDb(),   m_compVeri,   m_rigComp);
    posa(m_link->hasPowerSetting(),  m_link->powerSettingPct(), m_pwrSetVeri, m_rigPwrSet);

    // Il PTT per ultimo: quando scende azzera i misuratori di trasmissione, e
    // farlo prima di averli aggiornati cancellerebbe la lettura appena arrivata.
    setPttState(m_link->ptt());

    if (cambiato) {
        valutaAllarmeSwr();
        emit rigCtlChanged();
    }
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
        m_swrAlarmAttivo = false;
        emit rigCtlChanged();
    }
}

// La banda dalla frequenza. Gli stessi confini che usa Decodium sul computer:
// una frequenza fuori da ogni banda amatoriale non si forza dentro la piu'
// vicina, si lascia senza etichetta.
QString MeterBridge::bandaDaHz(double hz)
{
    struct Banda { double da, a; char const* nome; };
    static const Banda bande[] = {
        {   135700.0,    137800.0, "2200m" }, {    472000.0,    479000.0, "630m" },
        {  1800000.0,   2000000.0,  "160m" }, {   3500000.0,   4000000.0,   "80m" },
        {  5250000.0,   5450000.0,   "60m" }, {   7000000.0,   7300000.0,   "40m" },
        { 10100000.0,  10150000.0,   "30m" }, {  14000000.0,  14350000.0,   "20m" },
        { 18068000.0,  18168000.0,   "17m" }, {  21000000.0,  21450000.0,   "15m" },
        { 24890000.0,  24990000.0,   "12m" }, {  28000000.0,  29700000.0,   "10m" },
        { 50000000.0,  54000000.0,    "6m" }, {  70000000.0,  71000000.0,    "4m" },
        {144000000.0, 148000000.0,    "2m" }, { 222000000.0, 225000000.0, "1.25m" },
        {420000000.0, 450000000.0,   "70cm"},
    };
    for (const Banda& b : bande) {
        if (hz >= b.da && hz <= b.a)
            return QString::fromLatin1(b.nome);
    }
    return {};
}

QString MeterBridge::rigBand() const
{
    return bandaDaHz(m_rigFreqHz);
}

void MeterBridge::setSwrAlarmSoglia(double v)
{
    // Sotto uno il ROS non esiste, e una soglia irraggiungibile e' un allarme
    // spento senza dirlo.
    double const s = qBound(1.1, v, 10.0);
    if (qFuzzyCompare(s, m_swrAlarmSoglia)) return;
    m_swrAlarmSoglia = s;
    m_settings.setValue(QStringLiteral("swrAlarmSoglia"), s);
    emit alarmChanged();
}

void MeterBridge::setSwrAlarmVibra(bool on)
{
    if (m_swrAlarmVibra == on) return;
    m_swrAlarmVibra = on;
    m_settings.setValue(QStringLiteral("swrAlarmVibra"), on);
    emit alarmChanged();
}

// L'allarme guarda solo mentre si trasmette e solo con una lettura vera: a
// riposo il ROS che resta stampato non e' una misura, e far vibrare il
// telefono per un valore vecchio e' il modo piu' rapido per far disattivare
// l'allarme all'utente.
void MeterBridge::valutaAllarmeSwr()
{
    bool const alto = m_rigPtt && m_meterVeri && m_rigRos >= m_swrAlarmSoglia;
    if (alto != m_swrAlarmAttivo)
        m_swrAlarmAttivo = alto;
    if (!alto || !m_swrAlarmVibra)
        return;
    // Un colpo ogni quattro secondi finche' dura: continuo sarebbe un guasto,
    // uno solo si perde se il telefono e' in tasca.
    if (m_ultimaVibrazione.isValid() && m_ultimaVibrazione.elapsed() < kIntervalloVibrazione)
        return;
    m_ultimaVibrazione.restart();
    vibra(600);
}
