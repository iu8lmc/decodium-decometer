#ifndef DECOMETER_SPOT_FEED_HPP
#define DECOMETER_SPOT_FEED_HPP

#include <QAbstractListModel>
#include <QByteArray>
#include <QElapsedTimer>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QVector>

class QTcpSocket;

// Gli spot del cluster DX, presi da Decodium invece che dal nodo.
//
// Il telefono non apre una propria linea telnet verso il cluster: quella la
// tiene gia' il PC, ed e' quella giusta — un nodo non gradisce due sessioni
// dello stesso nominativo, e gli spot che conta sono quelli che l'operatore
// sta guardando sul computer, non un secondo flusso simile ma diverso. Per
// questo Decodium 4 li rivende in rete locale (DecodiumSpotShare, porta 4534
// di serie) e qui si leggono e basta.
//
// Il formato e' una riga JSON per messaggio:
//   {"tipo":"benvenuto","servizio":"decodium-spot-share","versione":1,"attesi":37}
//   {"tipo":"spot","dxCall":"JA1YYY","frequency":14074.0,...}
//   {"tipo":"battito"}
// Alla connessione arrivano gli ultimi spot gia' raccolti, poi quelli nuovi
// appena il nodo li manda.
//
// Come il ponte CAT: sola lettura, rete locale, e riconnessione da se' quando
// la linea cade — che su un telefono in giro per casa succede di continuo.
class SpotFeed : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString host READ host NOTIFY endpointChanged)
    Q_PROPERTY(int port READ port NOTIFY endpointChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // Filtro per banda: sul telefono si guarda una banda per volta, ed e' il
    // taglio che serve piu' spesso quando si cerca chi chiama dove si sta
    // trasmettendo.
    Q_PROPERTY(QString bandFilter READ bandFilter WRITE setBandFilter NOTIFY bandFilterChanged)
    Q_PROPERTY(QVariantList bandStats READ bandStats NOTIFY bandsChanged)

public:
    enum Ruoli {
        RuoloCall = Qt::UserRole + 1,
        RuoloFrequenza,
        RuoloBanda,
        RuoloModo,
        RuoloSpotter,
        RuoloCommento,
        RuoloOra,
    };
    Q_ENUM(Ruoli)

    explicit SpotFeed(QObject* parent = nullptr);
    ~SpotFeed() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool connected() const { return m_connected; }
    QString status() const { return m_status; }
    QString host() const { return m_host; }
    int port() const { return m_port; }
    int count() const { return int(m_visibili.size()); }

    QString bandFilter() const { return m_filtro; }
    void setBandFilter(const QString& banda);
    QVariantList bandStats() const;

public slots:
    void connectTo(const QString& host, int port);
    void disconnectFrom();
    void clear();

signals:
    void statusChanged();
    void endpointChanged();
    void countChanged();
    void bandsChanged();
    void bandFilterChanged();

private slots:
    void onReadyRead();

private:
    struct Spot {
        QString call;
        double frequenza {0.0};   // kHz, come li manda il cluster
        QString banda;
        QString modo;
        QString spotter;
        QString commento;
        QString ora;
    };

    void avviaConnessione();
    void chiudiSocket();
    void programmaRitentativo();
    void leggiRiga(const QByteArray& riga);
    void aggiungi(const Spot& s);
    void ricostruisciVisibili();
    bool passaIlFiltro(const Spot& s) const;

    QTcpSocket* m_sock {nullptr};
    QByteArray m_buf;
    QTimer m_ritenta;
    // Il server manda un battito ogni venti secondi: se non arriva piu' nulla
    // per un minuto la linea e' morta anche se il socket dice il contrario, e
    // si ricomincia. Senza il battito questa distinzione non esisterebbe — un
    // cluster tranquillo e un cavo staccato si somigliano troppo.
    QTimer m_guardia;
    QElapsedTimer m_ultimoDato;

    bool m_connected {false};
    bool m_vuoleConnesso {false};
    int m_ritardoRitentativo {1000};
    QString m_status;
    QString m_host;
    int m_port {4534};

    QVector<Spot> m_tutti;
    QVector<int> m_visibili;
    QString m_filtro;
    QHash<QString, int> m_perBanda;

    static constexpr int kMaxRighe = 300;
    static constexpr int kRitardoMin = 1000;
    static constexpr int kRitardoMax = 10000;
    static constexpr int kSilenzioMax = 60000;

    QSettings m_settings;
};

#endif
