#include "SpotFeed.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <algorithm>

SpotFeed::SpotFeed(QObject* parent)
    : QAbstractListModel(parent)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_host = m_settings.value(QStringLiteral("spotHost")).toString();
    m_port = m_settings.value(QStringLiteral("spotPort"), 4534).toInt();
    m_status = tr("Spot: non connesso");

    m_ritenta.setSingleShot(true);
    connect(&m_ritenta, &QTimer::timeout, this, [this] {
        if (!m_vuoleConnesso) return;
        chiudiSocket();
        avviaConnessione();
    });

    // La guardia gira solo a linea aperta e non ha bisogno di essere fitta:
    // deve accorgersi di un silenzio lungo, non misurarlo.
    m_guardia.setInterval(10000);
    connect(&m_guardia, &QTimer::timeout, this, [this] {
        if (!m_ultimoDato.isValid() || m_ultimoDato.elapsed() < kSilenzioMax)
            return;
        m_status = tr("Spot: linea muta, riconnessione…");
        m_connected = false;
        emit statusChanged();
        chiudiSocket();
        programmaRitentativo();
    });

    // Chi ha gia' collegato una volta ritrova la lista viva riaprendo l'app,
    // senza ripassare dalle impostazioni.
    if (!m_host.isEmpty())
        QTimer::singleShot(0, this, [this] { connectTo(m_host, m_port); });
}

SpotFeed::~SpotFeed()
{
    disconnectFrom();
}

int SpotFeed::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_visibili.size());
}

QVariant SpotFeed::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibili.size())
        return {};
    Spot const& s = m_tutti.at(m_visibili.at(index.row()));
    switch (role) {
    case RuoloCall:       return s.call;
    case RuoloFrequenza:  return s.frequenza;
    case RuoloBanda:      return s.banda;
    case RuoloModo:       return s.modo;
    case RuoloSpotter:    return s.spotter;
    case RuoloCommento:   return s.commento;
    case RuoloOra:        return s.ora;
    default:              return {};
    }
}

QHash<int, QByteArray> SpotFeed::roleNames() const
{
    return {
        {RuoloCall,      "call"},
        {RuoloFrequenza, "frequenza"},
        {RuoloBanda,     "banda"},
        {RuoloModo,      "modo"},
        {RuoloSpotter,   "spotter"},
        {RuoloCommento,  "commento"},
        {RuoloOra,       "ora"},
    };
}

QVariantList SpotFeed::bandStats() const
{
    QVariantList out;
    QList<QString> bande = m_perBanda.keys();
    std::sort(bande.begin(), bande.end(), [this](const QString& a, const QString& b) {
        int const ca = m_perBanda.value(a), cb = m_perBanda.value(b);
        return ca != cb ? ca > cb : a < b;
    });
    out.reserve(bande.size());
    for (const QString& b : bande) {
        QVariantMap v;
        v.insert(QStringLiteral("band"), b);
        v.insert(QStringLiteral("count"), m_perBanda.value(b));
        out.append(v);
    }
    return out;
}

void SpotFeed::setBandFilter(const QString& banda)
{
    if (m_filtro == banda) return;
    m_filtro = banda;
    emit bandFilterChanged();
    ricostruisciVisibili();
}

void SpotFeed::connectTo(const QString& host, int port)
{
    disconnectFrom();
    if (host.isEmpty() || port <= 0 || port > 65535) {
        m_status = tr("Indirizzo non valido");
        emit statusChanged();
        return;
    }
    m_host = host;
    m_port = port;
    m_settings.setValue(QStringLiteral("spotHost"), host);
    m_settings.setValue(QStringLiteral("spotPort"), port);
    emit endpointChanged();

    m_vuoleConnesso = true;
    m_ritardoRitentativo = kRitardoMin;
    avviaConnessione();
}

void SpotFeed::avviaConnessione()
{
    m_sock = new QTcpSocket(this);
    connect(m_sock, &QTcpSocket::readyRead, this, &SpotFeed::onReadyRead);
    connect(m_sock, &QTcpSocket::connected, this, [this] {
        m_connected = true;
        m_ritardoRitentativo = kRitardoMin;
        m_status = tr("Spot da %1:%2").arg(m_host).arg(m_port);
        m_ultimoDato.start();
        m_guardia.start();
        emit statusChanged();
    });
    connect(m_sock, &QTcpSocket::disconnected, this, [this] {
        m_guardia.stop();
        if (m_connected) {
            m_connected = false;
            m_status = tr("Spot: linea caduta");
            emit statusChanged();
        }
        programmaRitentativo();
    });
    connect(m_sock, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_guardia.stop();
        m_connected = false;
        m_status = tr("Spot: %1").arg(m_sock ? m_sock->errorString() : QString());
        emit statusChanged();
        programmaRitentativo();
    });

    m_status = m_ritardoRitentativo > kRitardoMin
                   ? tr("Spot: riconnessione a %1:%2…").arg(m_host).arg(m_port)
                   : tr("Spot: connessione a %1:%2…").arg(m_host).arg(m_port);
    emit statusChanged();
    m_sock->connectToHost(m_host, quint16(m_port));
}

void SpotFeed::chiudiSocket()
{
    m_guardia.stop();
    m_ultimoDato.invalidate();
    if (m_sock) {
        // Segnali staccati prima dell'abort: una chiusura decisa da noi non
        // deve rientrare dai gestori e programmare un ritentativo suo.
        m_sock->disconnect(this);
        m_sock->abort();
        m_sock->deleteLater();
        m_sock = nullptr;
    }
    m_buf.clear();
}

void SpotFeed::programmaRitentativo()
{
    if (!m_vuoleConnesso || m_host.isEmpty()) return;
    if (m_ritenta.isActive()) return;
    m_ritenta.start(m_ritardoRitentativo);
    m_ritardoRitentativo = qMin(m_ritardoRitentativo * 2, kRitardoMax);
}

void SpotFeed::disconnectFrom()
{
    bool const stavaProvando = m_vuoleConnesso;
    m_vuoleConnesso = false;
    m_ritenta.stop();
    m_ritardoRitentativo = kRitardoMin;
    chiudiSocket();
    if (m_connected || stavaProvando) {
        m_connected = false;
        m_status = tr("Spot: non connesso");
        emit statusChanged();
    }
}

void SpotFeed::onReadyRead()
{
    if (!m_sock) return;
    m_ultimoDato.restart();
    m_buf += m_sock->readAll();

    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        QByteArray const riga = m_buf.left(nl).trimmed();
        m_buf.remove(0, nl + 1);
        if (!riga.isEmpty())
            leggiRiga(riga);
    }

    // Una riga senza fine che cresce all'infinito vorrebbe dire che dall'altra
    // parte non c'e' il servizio che crediamo. Meglio ripartire che riempire
    // la memoria del telefono.
    if (m_buf.size() > (1 << 20))
        m_buf.clear();
}

void SpotFeed::leggiRiga(const QByteArray& riga)
{
    QJsonParseError err {};
    QJsonDocument const doc = QJsonDocument::fromJson(riga, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject const o = doc.object();
    QString const tipo = o.value(QStringLiteral("tipo")).toString();
    if (tipo == QLatin1String("battito"))
        return;   // serviva solo ad aggiornare m_ultimoDato, gia' fatto
    if (tipo == QLatin1String("benvenuto")) {
        int const attesi = o.value(QStringLiteral("attesi")).toInt();
        m_status = attesi > 0 ? tr("Spot da %1:%2 — %3 in arrivo").arg(m_host).arg(m_port).arg(attesi)
                              : tr("Spot da %1:%2").arg(m_host).arg(m_port);
        emit statusChanged();
        return;
    }
    if (tipo != QLatin1String("spot"))
        return;

    Spot s;
    s.call = o.value(QStringLiteral("dxCall")).toString();
    s.frequenza = o.value(QStringLiteral("frequency")).toDouble();
    s.banda = o.value(QStringLiteral("band")).toString();
    s.modo = o.value(QStringLiteral("mode")).toString();
    s.spotter = o.value(QStringLiteral("spotter")).toString();
    s.commento = o.value(QStringLiteral("comment")).toString();
    s.ora = o.value(QStringLiteral("time")).toString();
    if (s.call.isEmpty() || s.frequenza <= 0.0)
        return;   // uno spot senza nominativo o senza frequenza non e' uno spot
    if (s.banda.isEmpty())
        s.banda = QStringLiteral("?");
    aggiungi(s);
}

void SpotFeed::aggiungi(const Spot& s)
{
    m_perBanda[s.banda] += 1;

    m_tutti.prepend(s);
    if (m_tutti.size() > kMaxRighe) {
        m_tutti.remove(kMaxRighe, m_tutti.size() - kMaxRighe);
        ricostruisciVisibili();
    } else {
        for (int& i : m_visibili)
            ++i;
        if (passaIlFiltro(s)) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_visibili.prepend(0);
            endInsertRows();
        }
    }

    emit countChanged();
    emit bandsChanged();
}

bool SpotFeed::passaIlFiltro(const Spot& s) const
{
    return m_filtro.isEmpty() || s.banda.compare(m_filtro, Qt::CaseInsensitive) == 0;
}

void SpotFeed::ricostruisciVisibili()
{
    beginResetModel();
    m_visibili.clear();
    m_visibili.reserve(m_tutti.size());
    for (int i = 0; i < m_tutti.size(); ++i) {
        if (passaIlFiltro(m_tutti.at(i)))
            m_visibili.append(i);
    }
    endResetModel();
    emit countChanged();
}

void SpotFeed::clear()
{
    beginResetModel();
    m_tutti.clear();
    m_visibili.clear();
    endResetModel();
    m_perBanda.clear();
    emit countChanged();
    emit bandsChanged();
}
