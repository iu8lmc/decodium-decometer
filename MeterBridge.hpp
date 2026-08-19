#ifndef DECOMETER_METER_BRIDGE_HPP
#define DECOMETER_METER_BRIDGE_HPP

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>

class QTcpSocket;

// Ponte CAT minimo per Decometer standalone: SOLO lettura, SOLO in rete
// locale, SOLO i tre misuratori di trasmissione. Non decodifica, non
// trasmette, non tocca la seriale — parla in TCP col server CAT condiviso
// di Decodium 4 (porta 4533) o con un qualunque altro server compatibile
// col protocollo rigctl di Hamlib (rigctld, netrigctl).
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
    Q_PROPERTY(QString lastHost READ lastHost NOTIFY lastEndpointChanged)
    Q_PROPERTY(int lastPort READ lastPort NOTIFY lastEndpointChanged)

public:
    explicit MeterBridge(QObject* parent = nullptr);
    ~MeterBridge() override;

    bool catConnected() const { return m_catConnected; }
    QString catStatus() const { return m_catStatus; }
    QString rigModel() const { return QString(); }   // il server condiviso non lo dichiara

    bool rigMetersOn() const { return m_rigMetersOn; }
    void setRigMetersOn(bool on);
    bool rigPtt() const { return m_rigPtt; }
    bool txActive() const { return false; }
    int rigAlc() const { return m_rigAlc; }
    double rigWatt() const { return m_rigWatt; }
    double rigRos() const { return m_rigRos; }
    bool meterVeri() const { return m_meterVeri; }

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

    QString lastHost() const { return m_lastHost; }
    int lastPort() const { return m_lastPort; }

public slots:
    // Voluto invocabile dal QML: Impostazioni -> IP:porta -> Connetti.
    void catConnect(const QString& host, int port);
    void catDisconnect();

signals:
    void keepScreenOnChanged();
    void alarmChanged();
    void catChanged();
    void rigCtlChanged();
    void txActiveChanged();
    void lastEndpointChanged();

private slots:
    void onCatReadyRead();
    void onPoll();

private:
    void parseCatLines(const QByteArray& data);
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

    QTcpSocket* m_cat {nullptr};
    QByteArray m_catBuf;
    // UN SOLO ciclo veloce, non due. La prima versione interrogava il PTT una
    // volta al secondo e solo DOPO averlo visto alto cominciava a chiedere i
    // livelli: fino a 1,35 s fra il momento in cui si premeva il tasto e il
    // momento in cui l'ago si muoveva. Su uno strumento che serve a guardare
    // la potenza MENTRE si trasmette, un ritardo simile lo rende inutile.
    // Il server risponde in circa 3 ms e legge da memoria senza toccare la
    // seriale, quindi chiedere tutto insieme e spesso non costa quasi nulla.
    QTimer m_poll;
    QTimer m_ritenta;      // riconnessione dopo una caduta
    QTimer m_attesaConn;   // guardia sul singolo tentativo di connessione
    // Il giro precedente non ha ancora risposto: non se ne accavalla un altro.
    // A 80 ms su una rete che rallenta le domande si accumulerebbero, e le
    // risposte arriverebbero con un ritardo che cresce da solo — su un
    // misuratore vuol dire un ago che indica il passato.
    bool m_pollInVolo {false};
    QElapsedTimer m_ultimaRisposta;

    bool m_catConnected {false};
    QString m_catStatus;

    bool m_rigMetersOn {true};
    bool m_rigPtt {false};
    int m_rigAlc {0};
    double m_rigWatt {0.0};
    double m_rigRos {1.0};
    bool m_meterVeri {false};
    QString m_livelloAtteso;      // nome del livello di cui si aspetta "Level Value:"

    double m_rigFreqHz {0.0};
    int m_rigStrengthDb {0};
    bool m_strengthVeri {false};

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
    QString m_lastHost;
    int m_lastPort {4533};
    QSettings m_settings;
};

#endif
