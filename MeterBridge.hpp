#ifndef DECOMETER_METER_BRIDGE_HPP
#define DECOMETER_METER_BRIDGE_HPP

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>

class DecoPortLink;
class DecoPortDiscovery;

// Ponte CAT minimo per Decometer standalone: SOLO lettura, SOLO in rete
// locale, SOLO i tre misuratori di trasmissione. Non decodifica, non
// trasmette, non tocca la seriale — parla DecoPort col gateway di Decodium 4
// (UDP, porta 5559), che si annuncia da se' in rete locale sulla 5560.
//
// Prima parlava rigctl in TCP con la CAT condivisa sulla 4533. Quella porta
// pero' Decodium la apre solo su 127.0.0.1: dal telefono la connessione veniva
// rifiutata sempre, per costruzione, e il quadrante non poteva funzionare.
// DecoPort ascolta su tutte le interfacce, si annuncia da solo — quindi non
// c'e' piu' un indirizzo da digitare a memoria — ed e' autenticato, il che su
// una porta che espone una radio non e' un dettaglio.
//
// La superficie di proprieta' e il nome dei campi ricalcano DELIBERATAMENTE
// quelli di AppBridge in decodium-mobile/androidapp: e' quel bridge che
// Decometer.qml si aspetta, e tenerli identici e' cio' che permette di
// portare qui il file QML COSI' COM'E', senza toccarlo.
//
// Un'unica differenza deliberata: rigAlc (il livello ALC, scala 0-255 come
// il frontalino se lo aspetta) qui arriva SEMPRE dalla lettura calibrata
// Hamlib (\get_level ALC, forma estesa) scalata sulla stessa scala, mai da
// un comando proprietario Yaesu grezzo ("W RM5;"). Il server CAT condiviso
// di Decodium non inoltra comandi seriali grezzi — solo i livelli Hamlib
// standard — quindi qui non ce n'e' bisogno: il valore che arriva e' quello
// vero, non l'approssimazione della posizione dell'ago.
class MeterBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool catConnected READ catConnected NOTIFY catChanged)
    Q_PROPERTY(QString catStatus READ catStatus NOTIFY catChanged)
    Q_PROPERTY(QString rigModel READ rigModel NOTIFY catChanged)

    Q_PROPERTY(bool rigMetersOn READ rigMetersOn WRITE setRigMetersOn NOTIFY rigCtlChanged)
    Q_PROPERTY(bool rigPtt READ rigPtt NOTIFY rigCtlChanged)
    Q_PROPERTY(bool txActive READ txActive NOTIFY txActiveChanged)   // sempre falso: niente TX qui
    Q_PROPERTY(int rigAlc READ rigAlc NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigWatt READ rigWatt NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigRos READ rigRos NOTIFY rigCtlChanged)
    Q_PROPERTY(bool meterVeri READ meterVeri NOTIFY rigCtlChanged)

    // Frequenza e banda: sul FX77X sono in cima allo schermo perche' un
    // misuratore senza sapere DOVE si sta trasmettendo dice meta' della cosa.
    // Qui non serve un contatore: la frequenza la sa gia' la radio, e il CAT
    // la da' calibrata.
    Q_PROPERTY(double rigFreqHz READ rigFreqHz NOTIFY rigCtlChanged)
    Q_PROPERTY(QString rigBand READ rigBand NOTIFY rigCtlChanged)
    // S-meter, in ricezione. Hamlib lo da' in dB rispetto a S9: negativo
    // sotto S9 (-6 dB per unita' S), positivo sopra.
    // Gli strumenti del finale. Ognuno con il suo "veri", come gia' fanno le
    // misure di trasmissione: senza, uno zero e un silenzio si somiglierebbero.
    Q_PROPERTY(double rigVd READ rigVd NOTIFY rigCtlChanged)
    Q_PROPERTY(bool vdVeri READ vdVeri NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigId READ rigId NOTIFY rigCtlChanged)
    Q_PROPERTY(bool idVeri READ idVeri NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigTemp READ rigTemp NOTIFY rigCtlChanged)
    Q_PROPERTY(bool tempVeri READ tempVeri NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigComp READ rigComp NOTIFY rigCtlChanged)
    Q_PROPERTY(bool compVeri READ compVeri NOTIFY rigCtlChanged)
    Q_PROPERTY(double rigPwrSet READ rigPwrSet NOTIFY rigCtlChanged)
    Q_PROPERTY(bool pwrSetVeri READ pwrSetVeri NOTIFY rigCtlChanged)

    // DecoPort non ha una modalita' in chiaro: senza chiave il gateway non si
    // accende e il client non si collega. Quindi la chiave e' parte delle
    // impostazioni quanto l'indirizzo, e si ricorda allo stesso modo.
    Q_PROPERTY(QString authKey READ authKey WRITE setAuthKey NOTIFY lastEndpointChanged)
    // Le radio che si sono annunciate: host, porta, etichetta, se il CAT e' su.
    Q_PROPERTY(QVariantList radiosTrovate READ radiosTrovate NOTIFY radiosTrovateChanged)

    Q_PROPERTY(int rigStrengthDb READ rigStrengthDb NOTIFY rigCtlChanged)
    Q_PROPERTY(bool strengthVeri READ strengthVeri NOTIFY rigCtlChanged)

    // Allarme di ROS alto: soglia scelta dall'utente e avviso che si sente
    // anche senza guardare. Sul FX77X e' un beep; su un telefono che sta in
    // tasca mentre si trasmette dall'altra stanza, la vibrazione arriva dove
    // il beep non arriverebbe.
    Q_PROPERTY(double swrAlarmSoglia READ swrAlarmSoglia WRITE setSwrAlarmSoglia NOTIFY alarmChanged)
    Q_PROPERTY(bool swrAlarmVibra READ swrAlarmVibra WRITE setSwrAlarmVibra NOTIFY alarmChanged)
    Q_PROPERTY(bool swrAlarmAttivo READ swrAlarmAttivo NOTIFY rigCtlChanged)

    // Schermo sempre acceso: un misuratore che si spegne da solo mentre si
    // trasmette non e' un misuratore. E' il flag della finestra
    // (FLAG_KEEP_SCREEN_ON), che Android rilascia da se' quando l'app va in
    // background: non puo' restare incastrato.
    Q_PROPERTY(bool keepScreenOn READ keepScreenOn WRITE setKeepScreenOn NOTIFY keepScreenOnChanged)
    // Margini delle aree di sistema: barre e incavo dello schermo. Senza,
    // l'intestazione del quadrante finisce sotto l'orologio e il lato corto
    // sotto la barra dei gesti. E' la stessa soluzione dell'app completa, da
    // cui questa parte viene: la' e' costata un processo che moriva senza
    // messaggio, e la nota nel .cpp dice perche'.
    Q_PROPERTY(double safeTop READ safeTop NOTIFY safeAreaChanged)
    Q_PROPERTY(double safeBottom READ safeBottom NOTIFY safeAreaChanged)
    Q_PROPERTY(double safeLeft READ safeLeft NOTIFY safeAreaChanged)
    Q_PROPERTY(double safeRight READ safeRight NOTIFY safeAreaChanged)

    // Vero quando il gateway dichiara che questa stazione puo' davvero essere
    // messa in trasmissione da qui. Lo decide Decodium, non l'app: serve il CAT
    // per alzare il PTT e una radio che non stia gia' trasmettendo per conto
    // suo. Senza, lo sweep non parte nemmeno.
    Q_PROPERTY(bool puoTrasmettere READ puoTrasmettere NOTIFY rigCtlChanged)

    Q_PROPERTY(QString lastHost READ lastHost NOTIFY lastEndpointChanged)
    Q_PROPERTY(int lastPort READ lastPort NOTIFY lastEndpointChanged)

public:
    explicit MeterBridge(QObject* parent = nullptr);
    ~MeterBridge() override;

    bool catConnected() const { return m_catConnected; }
    QString catStatus() const { return m_catStatus; }
    // Ora la radio si presenta: DecoPort porta l'etichetta nel contesto.
    QString rigModel() const { return m_rigModel; }

    bool rigMetersOn() const { return m_rigMetersOn; }
    void setRigMetersOn(bool on);
    bool rigPtt() const { return m_rigPtt; }
    bool txActive() const { return false; }
    int rigAlc() const { return m_rigAlc; }
    double rigWatt() const { return m_rigWatt; }
    double rigRos() const { return m_rigRos; }
    bool meterVeri() const { return m_meterVeri; }

    double rigVd() const { return m_rigVd; }
    bool vdVeri() const { return m_vdVeri; }
    double rigId() const { return m_rigId; }
    bool idVeri() const { return m_idVeri; }
    double rigTemp() const { return m_rigTemp; }
    bool tempVeri() const { return m_tempVeri; }
    double rigComp() const { return m_rigComp; }
    bool compVeri() const { return m_compVeri; }
    double rigPwrSet() const { return m_rigPwrSet; }
    bool pwrSetVeri() const { return m_pwrSetVeri; }

    QString authKey() const { return m_authKey; }
    void setAuthKey(const QString& k);
    QVariantList radiosTrovate() const;

    double rigFreqHz() const { return m_rigFreqHz; }
    QString rigBand() const;
    int rigStrengthDb() const { return m_rigStrengthDb; }
    bool strengthVeri() const { return m_strengthVeri; }

    double swrAlarmSoglia() const { return m_swrAlarmSoglia; }
    void setSwrAlarmSoglia(double v);
    bool swrAlarmVibra() const { return m_swrAlarmVibra; }
    void setSwrAlarmVibra(bool on);
    bool swrAlarmAttivo() const { return m_swrAlarmAttivo; }

    // L'etichetta di banda dalla frequenza. Statica perche' e' una tabella,
    // non uno stato: gli stessi confini che usa Decodium sul computer.
    static QString bandaDaHz(double hz);

    bool keepScreenOn() const { return m_keepScreenOn; }
    void setKeepScreenOn(bool on);

    double safeTop() const { return m_safeTop; }
    double safeBottom() const { return m_safeBottom; }
    double safeLeft() const { return m_safeLeft; }
    double safeRight() const { return m_safeRight; }

    bool puoTrasmettere() const;

    QString lastHost() const { return m_lastHost; }
    int lastPort() const { return m_lastPort; }

public slots:
    // Da richiamare quando lo schermo ruota o l'app torna in primo piano: i
    // margini cambiano di lato, e all'avvio possono non essere ancora pronti.
    Q_INVOKABLE void refreshSafeArea();

    // Voluto invocabile dal QML: Impostazioni -> IP:porta -> Connetti.
    void catConnect(const QString& host, int port);
    void catDisconnect();

    // GLI UNICI DUE COMANDI CHE ESCONO DA QUEST'APP. Fino alla 1.1.0 non ce
    // n'era nessuno e la scheda del negozio diceva "di sola lettura": ora
    // servono all'analizzatore d'antenna, che per misurare deve per forza
    // trasmettere. Restano confinati li' — nessun'altra parte dell'interfaccia
    // li chiama — e valgono solo se il gateway dichiara puoTrasmettere.
    Q_INVOKABLE void sintonizza(double hz);
    Q_INVOKABLE void premiPtt(bool giu);

signals:
    void keepScreenOnChanged();
    void alarmChanged();
    void catChanged();
    void rigCtlChanged();
    void txActiveChanged();
    void lastEndpointChanged();
    void safeAreaChanged();
    void radiosTrovateChanged();

private slots:
    // Un solo posto dove il contesto ricevuto diventa lo stato del quadrante.
    void onLinkState();
    void onLinkLinked();

private:
    void setPttState(bool active);
    void resetTxMeters();
    void applyKeepScreenOn();
    // Un colpo di vibrazione. Il resto del programma non deve sapere come si
    // fa su ciascun sistema.
    void vibra(int ms);
    void valutaAllarmeSwr();

    // catConnect/catDisconnect sono cio' che vuole l'UTENTE; queste tre sono
    // cio' che fa la MACCHINA per ottenerlo, e il ritentativo le riusa senza
    // passare da catConnect (che azzererebbe l'intenzione e riscriverebbe le
    // impostazioni a ogni giro).
    void chiudiSocket();
    void avviaConnessione();
    void programmaRitentativo();

    // Un secondo al primo tentativo, poi il doppio ogni volta fino a dieci:
    // se il PC e' spento davvero non ha senso bussare due volte al secondo
    // per ore, e se invece e' solo il WiFi che ha vacillato il primo
    // ritentativo arriva subito.
    static constexpr int kRitardoMin = 1000;
    static constexpr int kRitardoMax = 10000;
    // Un SYN che nessuno raccoglie resta appeso finche' decide il sistema
    // operativo: decine di secondi, durante i quali nessun ritentativo
    // partirebbe perche' formalmente il tentativo e' ancora in corso. Questa
    // e' la caduta di rete tipica — il telefono cambia access point, il PC
    // sparisce senza chiudere niente — quindi il tentativo si taglia da se'.
    static constexpr int kTimeoutConn = 6000;
    // Un TCP che cade non sempre si chiude: il telefono cambia access point e
    // il socket resta aperto e muto per minuti, con il quadrante fermo
    // sull'ultima lettura e nessun errore da nessuna parte. Dopo tre secondi
    // di silenzio a fronte di domande fatte, la linea si considera morta e si
    // ricomincia da capo — che e' esattamente cio' che l'utente farebbe a
    // mano, ma senza doverci pensare mentre trasmette.
    static constexpr int kSilenzioMax = 3000;

    DecoPortLink* m_link {nullptr};
    DecoPortDiscovery* m_scoperta {nullptr};
    // UN SOLO ciclo veloce, non due. La prima versione interrogava il PTT una
    // volta al secondo e solo DOPO averlo visto alto cominciava a chiedere i
    // livelli: fino a 1,35 s fra il momento in cui si premeva il tasto e il
    // momento in cui l'ago si muoveva. Su uno strumento che serve a guardare
    // la potenza MENTRE si trasmette, un ritardo simile lo rende inutile.
    // Il server risponde in circa 3 ms e legge da memoria senza toccare la
    // seriale, quindi chiedere tutto insieme e spesso non costa quasi nulla.
    QTimer m_ritenta;      // riconnessione dopo una caduta
    // Il giro precedente non ha ancora risposto: non se ne accavalla un altro.
    // A 80 ms su una rete che rallenta le domande si accumulerebbero, e le
    // risposte arriverebbero con un ritardo che cresce da solo — su un
    // misuratore vuol dire un ago che indica il passato.

    bool m_catConnected {false};
    QString m_catStatus;

    bool m_rigMetersOn {true};
    bool m_rigPtt {false};
    int m_rigAlc {0};
    double m_rigWatt {0.0};
    double m_rigRos {1.0};
    bool m_meterVeri {false};

    double m_rigFreqHz {0.0};
    int m_rigStrengthDb {0};
    bool m_strengthVeri {false};
    QString m_rigModel;

    double m_rigVd {0.0};      bool m_vdVeri {false};
    double m_rigId {0.0};      bool m_idVeri {false};
    double m_rigTemp {0.0};    bool m_tempVeri {false};
    double m_rigComp {0.0};    bool m_compVeri {false};
    double m_rigPwrSet {0.0};  bool m_pwrSetVeri {false};

    QString m_authKey;

    double m_swrAlarmSoglia {2.5};
    bool m_swrAlarmVibra {true};
    bool m_swrAlarmAttivo {false};
    // Quando ha vibrato l'ultima volta: un allarme che vibra dodici volte al
    // secondo non e' un allarme, e' un guasto.
    QElapsedTimer m_ultimaVibrazione;
    static constexpr int kIntervalloVibrazione = 4000;

    bool m_keepScreenOn {true};
    // Vero da quando l'utente ha chiesto di collegarsi a quando chiede di
    // staccare: e' la differenza fra una linea caduta, da riprendere da se',
    // e uno stacco voluto, che deve restare staccato.
    bool m_vuoleConnesso {false};
    int m_ritardoRitentativo {kRitardoMin};
    double m_safeTop {0.0};
    double m_safeBottom {0.0};
    double m_safeLeft {0.0};
    double m_safeRight {0.0};

    QString m_lastHost;
    int m_lastPort {5559};
    QSettings m_settings;
};

#endif
