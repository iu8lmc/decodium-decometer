#include "DecodeFeed.hpp"

#include <QDataStream>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QTime>
#include <QTimer>
#include <QUdpSocket>
#include <algorithm>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#endif

namespace {
// Numero magico del protocollo UDP di WSJT-X, che Decodium eredita: non
// cambia mai, ed e' l'unica cosa che distingue questi pacchetti da qualunque
// altra cosa transiti sulla porta.
constexpr quint32 kMagic = 0xadbccbda;

enum Tipo : quint32 {
    TipoHeartbeat = 0,
    TipoStatus = 1,
    TipoDecode = 2,
    TipoClear = 3,
    TipoQsoLogged = 5,
    TipoClose = 6,
    TipoWsprDecode = 10,
    TipoLoggedAdif = 12,
};

QString nomeTipo(quint32 t)
{
    switch (t) {
    case TipoHeartbeat:  return QStringLiteral("battito");
    case TipoStatus:     return QStringLiteral("stato");
    case TipoDecode:     return QStringLiteral("decodifica");
    case TipoClear:      return QStringLiteral("pulizia");
    case TipoQsoLogged:  return QStringLiteral("QSO loggato");
    case TipoClose:      return QStringLiteral("chiusura");
    case TipoWsprDecode: return QStringLiteral("WSPR");
    case TipoLoggedAdif: return QStringLiteral("ADIF");
    default:             return QStringLiteral("tipo %1").arg(t);
    }
}
}   // namespace

DecodeFeed::DecodeFeed(QObject* parent)
    : QAbstractListModel(parent)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_status = tr("In ascolto: no");
    m_port = m_settings.value(QStringLiteral("udpPort"), 2237).toInt();
    m_group = m_settings.value(QStringLiteral("udpGroup")).toString();

    // Si riapre l'app e si torna ad ascoltare da soli, come fanno le misure e
    // gli spot: una porta gia' scelta una volta non si richiede ogni volta.
    // Solo se qualcuno l'aveva davvero scelta — un ascolto mai chiesto non si
    // avvia da se'.
    if (m_settings.value(QStringLiteral("udpAttivo"), false).toBool()) {
        QTimer::singleShot(0, this, [this] { listen(m_port, m_group); });
    }
}

DecodeFeed::~DecodeFeed()
{
    stop();
}

int DecodeFeed::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_visibili.size());
}

QVariant DecodeFeed::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibili.size())
        return {};
    Decodifica const& d = m_tutte.at(m_visibili.at(index.row()));
    switch (role) {
    case RuoloOra:              return d.ora;
    case RuoloSnr:              return d.snr;
    case RuoloDt:               return d.dt;
    case RuoloDf:               return d.df;
    case RuoloModo:             return d.modo;
    case RuoloMessaggio:        return d.messaggio;
    case RuoloBassaConfidenza:  return d.bassaConfidenza;
    default:                    return {};
    }
}

QHash<int, QByteArray> DecodeFeed::roleNames() const
{
    return {
        {RuoloOra,             "ora"},
        {RuoloSnr,             "snr"},
        {RuoloDt,              "dt"},
        {RuoloDf,              "df"},
        {RuoloModo,            "modo"},
        {RuoloMessaggio,       "messaggio"},
        {RuoloBassaConfidenza, "bassaConfidenza"},
    };
}

bool DecodeFeed::listening() const
{
    return m_sock && m_sock->state() == QAbstractSocket::BoundState;
}

QVariantList DecodeFeed::modeStats() const
{
    QVariantList out;
    QList<QString> modi = m_perModo.keys();
    std::sort(modi.begin(), modi.end(), [this](const QString& a, const QString& b) {
        int const ca = m_perModo.value(a), cb = m_perModo.value(b);
        return ca != cb ? ca > cb : a < b;
    });
    out.reserve(modi.size());
    for (const QString& m : modi) {
        QVariantMap v;
        v.insert(QStringLiteral("mode"), m);
        v.insert(QStringLiteral("count"), m_perModo.value(m));
        out.append(v);
    }
    return out;
}

void DecodeFeed::setModeFilter(const QString& modo)
{
    if (m_filtro == modo) return;
    m_filtro = modo;
    emit modeFilterChanged();
    ricostruisciVisibili();
}

// Ascolto su tutte le interfacce: il pacchetto arriva dal WiFi, e l'indirizzo
// scritto sul PC puo' essere quello del telefono, un broadcast o un gruppo
// multicast. ShareAddress perche' sulla stessa porta puo' esserci gia'
// qualcun altro in ascolto: qui non si toglie niente a nessuno.
void DecodeFeed::listen(int port, const QString& group)
{
    stop();
    if (port <= 0 || port > 65535) {
        m_status = tr("Porta non valida");
        emit statusChanged();
        return;
    }
    m_port = port;
    m_group = group.trimmed();
    m_settings.setValue(QStringLiteral("udpPort"), m_port);
    m_settings.setValue(QStringLiteral("udpGroup"), m_group);
    m_settings.setValue(QStringLiteral("udpAttivo"), true);
    emit endpointChanged();

    m_sock = new QUdpSocket(this);
    connect(m_sock, &QUdpSocket::readyRead, this, &DecodeFeed::onDatagrams);
    if (!m_sock->bind(QHostAddress::AnyIPv4, quint16(m_port),
                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_status = tr("Porta %1 occupata: %2").arg(m_port).arg(m_sock->errorString());
        m_sock->deleteLater();
        m_sock = nullptr;
        emit statusChanged();
        return;
    }

    applicaMulticast();

    m_status = m_group.isEmpty()
                   ? tr("In ascolto sulla porta %1").arg(m_port)
                   : tr("In ascolto su %1, porta %2").arg(m_group).arg(m_port);
    emit statusChanged();
}

// Il multicast su Android non arriva se nessuno tiene alzato il blocco
// apposito: il WiFi scarta i pacchetti di gruppo per non svegliare la radio,
// e l'app resta in ascolto di un silenzio che sembra un guasto.
void DecodeFeed::applicaMulticast()
{
    if (!m_sock || m_group.isEmpty()) return;

    QHostAddress const gruppo(m_group);
    if (gruppo.isNull() || !gruppo.isMulticast()) {
        m_status = tr("%1 non e' un gruppo multicast").arg(m_group);
        emit statusChanged();
        return;
    }

#if defined(Q_OS_ANDROID)
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
        QJniObject ctx = QNativeInterface::QAndroidApplication::context();
        if (!ctx.isValid()) return;
        QJniObject servizio = QJniObject::fromString(QStringLiteral("wifi"));
        QJniObject wifi = ctx.callObjectMethod("getSystemService",
                                               "(Ljava/lang/String;)Ljava/lang/Object;",
                                               servizio.object<jstring>());
        if (!wifi.isValid()) return;
        QJniObject tag = QJniObject::fromString(QStringLiteral("decometer-decode"));
        QJniObject lock = wifi.callObjectMethod(
            "createMulticastLock",
            "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;",
            tag.object<jstring>());
        if (lock.isValid())
            lock.callMethod<void>("acquire", "()V");
    });
#endif

    // Iscrizione su ogni interfaccia utile: su un telefono c'e' il WiFi, ma
    // ce ne puo' essere piu' d'una e indovinare quale sarebbe un modo per
    // sbagliare in silenzio.
    bool unaBuona = false;
    const auto interfacce = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& ni : interfacce) {
        if (!(ni.flags() & QNetworkInterface::IsUp)) continue;
        if (!(ni.flags() & QNetworkInterface::CanMulticast)) continue;
        if (m_sock->joinMulticastGroup(gruppo, ni))
            unaBuona = true;
    }
    if (!unaBuona && !m_sock->joinMulticastGroup(gruppo)) {
        m_status = tr("Gruppo %1 non raggiungibile: %2").arg(m_group, m_sock->errorString());
        emit statusChanged();
    }
}

void DecodeFeed::stop()
{
    if (!m_sock) return;
    if (!m_group.isEmpty()) {
        QHostAddress const gruppo(m_group);
        if (!gruppo.isNull() && gruppo.isMulticast())
            m_sock->leaveMulticastGroup(gruppo);
    }
    m_sock->close();
    m_sock->deleteLater();
    m_sock = nullptr;
    m_status = tr("In ascolto: no");
    emit statusChanged();
}

void DecodeFeed::clear()
{
    beginResetModel();
    m_tutte.clear();
    m_visibili.clear();
    endResetModel();
    m_perModo.clear();
    m_ricevute = 0;
    m_pacchetti = 0;
    m_ultimoPacchetto.clear();
    emit countsChanged();
    emit modesChanged();
}

void DecodeFeed::onDatagrams()
{
    while (m_sock && m_sock->hasPendingDatagrams())
        leggiPacchetto(m_sock->receiveDatagram().data());
}

void DecodeFeed::leggiPacchetto(const QByteArray& dato)
{
    QDataStream in(dato);
    quint32 magic = 0;
    quint32 schema = 0;
    in >> magic;
    if (magic != kMagic)
        return;   // non e' roba di WSJT-X: non e' un errore, e' altro traffico
    in >> schema;

    // La versione di serializzazione la dichiara il pacchetto stesso. Leggere
    // con quella sbagliata non fallisce: restituisce numeri credibili e
    // sbagliati, che su una schermata di diagnostica sono il peggio possibile.
    if (schema <= 1)      in.setVersion(QDataStream::Qt_5_0);
    else if (schema <= 2) in.setVersion(QDataStream::Qt_5_2);
    else                  in.setVersion(QDataStream::Qt_5_4);

    quint32 tipo = 0;
    QByteArray id;
    in >> tipo >> id;
    if (in.status() != QDataStream::Ok)
        return;

    ++m_pacchetti;
    m_ultimoPacchetto = nomeTipo(tipo);

    if (tipo == TipoDecode) {
        bool nuova = false;
        QTime ora;
        qint32 snr = 0;
        double dt = 0.0;
        quint32 df = 0;
        QByteArray modo, messaggio;
        bool bassa = false;
        in >> nuova >> ora >> snr >> dt >> df >> modo >> messaggio >> bassa;
        if (in.status() != QDataStream::Ok) {
            emit countsChanged();
            return;
        }

        Decodifica d;
        d.ora = ora.toString(QStringLiteral("HH:mm:ss"));
        d.snr = snr;
        d.dt = dt;
        d.df = int(df);
        d.modo = QString::fromUtf8(modo).trimmed();
        d.messaggio = QString::fromUtf8(messaggio).trimmed();
        d.bassaConfidenza = bassa;
        if (d.modo.isEmpty())
            d.modo = QStringLiteral("?");
        aggiungi(d);
        return;
    }

    if (tipo == TipoStatus) {
        quint64 dial = 0;
        QByteArray modo, dxCall, report, txModo;
        bool txAbilitato = false;
        bool trasmette = false;
        in >> dial >> modo >> dxCall >> report >> txModo >> txAbilitato >> trasmette;
        if (in.status() == QDataStream::Ok) {
            // I campi successivi (decoding, rx df, tx df, nominativo, locatore
            // …) qui non servono e non si leggono: quel che serve e' su quale
            // frequenza e in che modo sta lavorando il PC.
            QString const m = QString::fromUtf8(modo).trimmed();
            QString const mhz = QString::number(dial / 1e6, 'f', 6);
            QString const nuovo = m.isEmpty() ? tr("%1 MHz").arg(mhz)
                                              : tr("%1 MHz · %2").arg(mhz, m);
            if (nuovo != m_dialLabel || trasmette != m_transmitting) {
                m_dialLabel = nuovo;
                m_transmitting = trasmette;
                emit statusChanged();
            }
        }
    }

    emit countsChanged();
}

void DecodeFeed::aggiungi(const Decodifica& d)
{
    ++m_ricevute;
    m_perModo[d.modo] += 1;

    // Le nuove in cima. Il tetto si applica in coda, dove ci sono le piu'
    // vecchie: quando si taglia, gli indici gia' calcolati per la lista
    // visibile non valgono piu', e la si rifa'.
    m_tutte.prepend(d);
    if (m_tutte.size() > kMaxRighe) {
        m_tutte.remove(kMaxRighe, m_tutte.size() - kMaxRighe);
        ricostruisciVisibili();
    } else {
        // Tutti gli indici scalano di uno: si aggiornano prima di inserire il
        // nuovo, altrimenti punterebbero alla riga sbagliata.
        for (int& i : m_visibili)
            ++i;
        if (passaIlFiltro(d)) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_visibili.prepend(0);
            endInsertRows();
        }
    }

    emit countsChanged();
    // Sempre: cambia l'elenco dei modi quando ne compare uno nuovo, e il
    // conteggio accanto al modo tutte le altre volte.
    emit modesChanged();
}

bool DecodeFeed::passaIlFiltro(const Decodifica& d) const
{
    return m_filtro.isEmpty() || d.modo.compare(m_filtro, Qt::CaseInsensitive) == 0;
}

void DecodeFeed::ricostruisciVisibili()
{
    beginResetModel();
    m_visibili.clear();
    m_visibili.reserve(m_tutte.size());
    for (int i = 0; i < m_tutte.size(); ++i) {
        if (passaIlFiltro(m_tutte.at(i)))
            m_visibili.append(i);
    }
    endResetModel();
    emit countsChanged();
}
