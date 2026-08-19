#ifndef DECOMETER_DECODE_FEED_HPP
#define DECOMETER_DECODE_FEED_HPP

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class QUdpSocket;

// Ascolta il traffico UDP che Decodium 4 emette gia' di suo e ne mostra le
// decodifiche, distinte per modo.
//
// Non c'e' niente da aggiungere al PC: Decodium, come ogni discendente di
// WSJT-X, spedisce ogni decodifica in un pacchetto UDP verso l'indirizzo
// scritto in Impostazioni -> Reporting (UDPServer, di serie 127.0.0.1:2237).
// Perche' arrivino qui basta scriverci l'indirizzo del telefono, oppure un
// gruppo multicast se li devono ricevere in piu' di uno.
//
// Il formato e' quello pubblico di WSJT-X, non un'invenzione di Decodium:
//   quint32 0xadbccbda | quint32 schema | quint32 tipo | utf8 id | corpo
// e il corpo di una decodifica (tipo 2) e'
//   bool nuova | QTime ora | qint32 snr | double dt | quint32 df
//   | utf8 modo | utf8 messaggio | bool bassa confidenza | bool fuori onda
// La versione di QDataStream dipende dallo schema dichiarato nel pacchetto:
// 1 -> Qt_5_0, 2 -> Qt_5_2, 3 -> Qt_5_4. Leggerlo con la versione sbagliata
// non da' errore, da' numeri plausibili e falsi — per questo si rispetta.
//
// SOLA LETTURA, come tutto il resto dell'app: si ascolta una porta, non si
// risponde a nessuno e non si chiede niente al PC.
class DecodeFeed : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool listening READ listening NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int port READ port NOTIFY endpointChanged)
    Q_PROPERTY(QString group READ group NOTIFY endpointChanged)

    // Quante decodifiche sono passate da quando si ascolta, e quante ne
    // mostra la lista adesso (che dipende dal filtro).
    Q_PROPERTY(int received READ received NOTIFY countsChanged)
    Q_PROPERTY(int shown READ shown NOTIFY countsChanged)
    // Pacchetti di qualunque tipo: dice che il PC sta parlando anche quando
    // non c'e' una sola decodifica, che e' la meta' dei casi in cui uno apre
    // questa schermata per capire perche' non vede niente.
    Q_PROPERTY(int packets READ packets NOTIFY countsChanged)
    Q_PROPERTY(QString lastPacket READ lastPacket NOTIFY countsChanged)

    // Un elemento per modo visto, {mode, count}, dal piu' frequente al meno:
    // e' il "nei vari modi" della schermata, e serve anche da filtro.
    Q_PROPERTY(QVariantList modeStats READ modeStats NOTIFY modesChanged)
    Q_PROPERTY(QString modeFilter READ modeFilter WRITE setModeFilter NOTIFY modeFilterChanged)

    // Dallo Status di Decodium: che radio guarda e se sta trasmettendo.
    Q_PROPERTY(QString dialLabel READ dialLabel NOTIFY statusChanged)
    Q_PROPERTY(bool transmitting READ transmitting NOTIFY statusChanged)

public:
    enum Ruoli {
        RuoloOra = Qt::UserRole + 1,
        RuoloSnr,
        RuoloDt,
        RuoloDf,
        RuoloModo,
        RuoloMessaggio,
        RuoloBassaConfidenza,
    };
    Q_ENUM(Ruoli)

    explicit DecodeFeed(QObject* parent = nullptr);
    ~DecodeFeed() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool listening() const;
    QString status() const { return m_status; }
    int port() const { return m_port; }
    QString group() const { return m_group; }

    int received() const { return m_ricevute; }
    int shown() const { return int(m_visibili.size()); }
    int packets() const { return m_pacchetti; }
    QString lastPacket() const { return m_ultimoPacchetto; }

    QVariantList modeStats() const;
    QString modeFilter() const { return m_filtro; }
    void setModeFilter(const QString& modo);

    QString dialLabel() const { return m_dialLabel; }
    bool transmitting() const { return m_transmitting; }

public slots:
    // group vuoto = si ascolta e basta (unicast o broadcast); altrimenti ci si
    // iscrive anche al gruppo multicast indicato.
    void listen(int port, const QString& group);
    void stop();
    void clear();

signals:
    void statusChanged();
    void endpointChanged();
    void countsChanged();
    void modesChanged();
    void modeFilterChanged();

private slots:
    void onDatagrams();

private:
    struct Decodifica {
        QString ora;
        int snr {0};
        double dt {0.0};
        int df {0};
        QString modo;
        QString messaggio;
        bool bassaConfidenza {false};
    };

    void leggiPacchetto(const QByteArray& dato);
    void aggiungi(const Decodifica& d);
    void ricostruisciVisibili();
    bool passaIlFiltro(const Decodifica& d) const;
    void applicaMulticast();

    QUdpSocket* m_sock {nullptr};
    QString m_status;
    int m_port {2237};
    QString m_group;

    // Tutte le decodifiche tenute, e gli indici di quelle che il filtro
    // lascia passare. Il filtro sta qui e non nel QML perche' una lista di
    // migliaia di righe filtrata a ogni ridisegno e' il modo piu' semplice
    // per rendere scattosa una schermata che deve solo scorrere.
    QVector<Decodifica> m_tutte;
    QVector<int> m_visibili;
    QString m_filtro;               // vuoto = tutti i modi

    // Le piu' recenti in cima e un tetto al numero: e' una finestra sul
    // traffico, non un archivio. Un telefono che tiene tutto finisce per
    // scorrere una lista che nessuno guardera' mai.
    static constexpr int kMaxRighe = 500;

    QHash<QString, int> m_perModo;
    int m_ricevute {0};
    int m_pacchetti {0};
    QString m_ultimoPacchetto;

    QString m_dialLabel;
    bool m_transmitting {false};

    QSettings m_settings;
};

#endif
