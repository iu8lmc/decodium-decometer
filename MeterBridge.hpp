#ifndef DECOMETER_METER_BRIDGE_HPP
#define DECOMETER_METER_BRIDGE_HPP

#include <QByteArray>
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

    bool m_catConnected {false};
    QString m_catStatus;

    bool m_rigMetersOn {true};
    bool m_rigPtt {false};
    int m_rigAlc {0};
    double m_rigWatt {0.0};
    double m_rigRos {1.0};
    bool m_meterVeri {false};
    QString m_livelloAtteso;      // nome del livello di cui si aspetta "Level Value:"

    bool m_keepScreenOn {true};
    QString m_lastHost;
    int m_lastPort {4533};
    QSettings m_settings;
};

#endif
