#include "AntennaProbe.hpp"

#include "MeterBridge.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

#include <cmath>

AntennaProbe::AntennaProbe(MeterBridge* bridge, QObject* parent)
    : QObject(parent)
    , m_bridge(bridge)
    , m_settings(QStringLiteral("Decodium"), QStringLiteral("Decometer"))
{
    m_ultimoCampione.start();
    m_livelloTono = m_settings.value(QStringLiteral("livelloTono"), 0.25).toDouble();

    if (m_bridge) {
        // La raccolta passiva non chiede niente a nessuno: guarda le misure che
        // gia' passano mentre l'operatore trasmette per conto suo. E' cosi' che
        // la curva si riempie senza che l'app debba mai premere il PTT.
        connect(m_bridge, &MeterBridge::rigCtlChanged, this, &AntennaProbe::osservaMisura);
        m_banda = MeterBridge::bandaDaHz(m_bridge->rigFreqHz());
    }
    carica();
    ricalcola();

    m_sweep.setSingleShot(true);
    connect(&m_sweep, &QTimer::timeout, this, &AntennaProbe::passoSweep);
}

AntennaProbe::~AntennaProbe()
{
    // Se l'oggetto muore con la portante alzata, la portante resta alzata.
    if (m_sweepAttivo && m_bridge)
        m_bridge->premiPtt(false);
    salva();
}

QVariantList AntennaProbe::campioni() const
{
    QVariantList out;
    out.reserve(m_punti.size());
    for (const Punto& p : m_punti) {
        QVariantMap m;
        m.insert(QStringLiteral("hz"), p.hz);
        m.insert(QStringLiteral("ros"), p.ros);
        out.append(m);
    }
    return out;
}

// ── raccolta ────────────────────────────────────────────────────────────────

void AntennaProbe::osservaMisura()
{
    if (!m_bridge) return;

    QString const banda = MeterBridge::bandaDaHz(m_bridge->rigFreqHz());
    if (!banda.isEmpty() && banda != m_banda)
        cambiaBanda(banda);

    // Un ROS vale solo se e' stato misurato mentre si trasmetteva. A riposo la
    // radio non ha niente da misurare e il valore fermo dell'ultima volta
    // sembrerebbe un campione nuovo.
    if (!m_bridge->rigPtt() || !m_bridge->meterVeri())
        return;
    if (m_ultimoCampione.elapsed() < kIntervalloCampione)
        return;

    double const hz = m_bridge->rigFreqHz();
    double const ros = m_bridge->rigRos();
    if (hz <= 0.0 || ros < 1.0)
        return;

    m_ultimoCampione.restart();
    aggiungiPunto(hz, ros);
}

void AntennaProbe::aggiungiPunto(double hz, double ros)
{
    // Stessa frequenza, entro la risoluzione: si tiene il ROS PEGGIORE. Non e'
    // pessimismo — un ROS piu' basso letto sulla stessa frequenza di solito
    // significa che il misuratore non era ancora salito, non che l'antenna e'
    // migliorata nel frattempo.
    for (Punto& p : m_punti) {
        if (std::fabs(p.hz - hz) < kRisoluzioneHz) {
            if (ros > p.ros) {
                p.ros = ros;
                emit campioniChanged();
                ricalcola();
                salva();
            }
            return;
        }
    }

    m_punti.append({hz, ros});
    std::sort(m_punti.begin(), m_punti.end(),
              [](const Punto& a, const Punto& b) { return a.hz < b.hz; });
    if (m_punti.size() > kMaxPunti)
        m_punti.remove(0, m_punti.size() - kMaxPunti);

    emit campioniChanged();
    ricalcola();
    salva();
}

void AntennaProbe::cambiaBanda(const QString& nuova)
{
    salva();
    m_banda = nuova;
    m_punti.clear();
    carica();
    ricalcola();
    emit campioniChanged();
}

void AntennaProbe::setLivelloTono(double v)
{
    double const nuovo = qBound(0.0, v, 0.9);
    if (qFuzzyCompare(nuovo + 1.0, m_livelloTono + 1.0)) return;
    m_livelloTono = nuovo;
    m_settings.setValue(QStringLiteral("livelloTono"), m_livelloTono);
    emit sweepChanged();
}

void AntennaProbe::dimentica()
{
    m_punti.clear();
    salva();
    ricalcola();
    emit campioniChanged();
}

// ── persistenza ─────────────────────────────────────────────────────────────
//
// Una chiave per banda. I punti di 40 metri non dicono niente su quelli di 20,
// e tenerli insieme farebbe adattare una risonanza sola a due antenne diverse.

void AntennaProbe::carica()
{
    if (m_banda.isEmpty()) return;
    QString const chiave = QStringLiteral("antenna/") + m_banda;
    QByteArray const raw = m_settings.value(chiave).toByteArray();
    if (raw.isEmpty()) return;

    QJsonArray const arr = QJsonDocument::fromJson(raw).array();
    m_punti.clear();
    m_punti.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        QJsonObject const o = v.toObject();
        double const hz = o.value(QStringLiteral("hz")).toDouble();
        double const ros = o.value(QStringLiteral("ros")).toDouble();
        if (hz > 0.0 && ros >= 1.0)
            m_punti.append({hz, ros});
    }
}

void AntennaProbe::salva()
{
    if (m_banda.isEmpty()) return;
    QJsonArray arr;
    for (const Punto& p : m_punti) {
        QJsonObject o;
        o.insert(QStringLiteral("hz"), p.hz);
        o.insert(QStringLiteral("ros"), p.ros);
        arr.append(o);
    }
    m_settings.setValue(QStringLiteral("antenna/") + m_banda,
                        QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// ── il modello ──────────────────────────────────────────────────────────────

double AntennaProbe::rosDaModello(double hz, double f0, double r0, double q)
{
    if (hz <= 0.0 || f0 <= 0.0 || r0 <= 0.0) return 1.0;
    double const x = r0 * q * (hz / f0 - f0 / hz);
    double const num = (r0 - 50.0) * (r0 - 50.0) + x * x;
    double const den = (r0 + 50.0) * (r0 + 50.0) + x * x;
    if (den <= 0.0) return 1.0;
    double g = std::sqrt(num / den);
    if (g > 0.999999) g = 0.999999;
    return (1.0 + g) / (1.0 - g);
}

double AntennaProbe::rosModello(double hz) const
{
    if (!m_valido) return 0.0;
    return rosDaModello(hz, m_f0, m_r0, m_q);
}

double AntennaProbe::qBassa() const
{
    // Il Q che va con la resistenza piu' bassa. Se il fit ha gia' scelto quella,
    // e' il suo; altrimenti e' quello dell'altra scalato dall'inversione.
    double const rb = qMin(m_r0, 2500.0 / m_r0);
    return (rb == m_r0) ? m_q : m_q * (m_r0 * m_r0) / 2500.0;
}

double AntennaProbe::qAlta() const
{
    double const ra = qMax(m_r0, 2500.0 / m_r0);
    return (ra == m_r0) ? m_q : m_q * (m_r0 * m_r0) / 2500.0;
}

QVariantList AntennaProbe::gammaModello(double hz, int ramo) const
{
    QVariantList out;
    if (!m_valido || hz <= 0.0) return out;   // niente stima, niente punto

    // I due rami danno la STESSA reattanza — e' il punto di tutta la faccenda —
    // ma coefficienti di riflessione diversi, perche' la parte reale cambia.
    double const r = (ramo == 0) ? resistenzaBassa() : resistenzaAlta();
    double const x = m_x;
    // G = (Z - 50) / (Z + 50), con Z = R + jX
    double const ar = r - 50.0, ai = x;
    double const br = r + 50.0, bi = x;
    double const den = br * br + bi * bi;
    if (den <= 0.0) return out;
    out.append((ar * br + ai * bi) / den);
    out.append((ai * br - ar * bi) / den);
    return out;
}

void AntennaProbe::ricalcola()
{
    bool const eraValido = m_valido;
    m_valido = false;
    m_scarto = 0.0;

    if (m_punti.size() < 5) {
        m_perche = tr("Servono almeno cinque punti: trasmetti su frequenze diverse "
                      "della banda, oppure lancia uno sweep.");
        if (eraValido) emit stimaChanged();
        else emit stimaChanged();
        return;
    }

    double const hzMin = m_punti.first().hz;
    double const hzMax = m_punti.last().hz;
    double const centro = 0.5 * (hzMin + hzMax);
    // Quanta banda serve davvero: provato in laboratorio su dati sintetici,
    // con l'1%% il Q usciva sbagliato di tre volte pur con uno scarto piccolo —
    // il fianco della campana non e' abbastanza pendente da vincolarlo. Col
    // 2,5%% torna. Uno scarto basso su pochi punti vicini non e' una conferma:
    // e' un modello che passa per tre punti allineati.
    if (centro <= 0.0 || (hzMax - hzMin) / centro < 0.025) {
        m_perche = tr("I punti coprono troppa poca banda: sotto il 2,5%% la "
                      "larghezza della risonanza non e' determinabile. Trasmetti "
                      "piu' lontano dal centro, o allarga lo sweep.");
        emit stimaChanged();
        return;
    }

    // Il minimo misurato, come punto di partenza per f0.
    int iMin = 0;
    for (int i = 1; i < m_punti.size(); ++i)
        if (m_punti.at(i).ros < m_punti.at(iMin).ros) iMin = i;
    m_rosMin = m_punti.at(iMin).ros;
    double f0Iniziale = m_punti.at(iMin).hz;

    // Ricerca a griglia su f0, R0 e Q, poi un secondo giro piu' fitto attorno al
    // vincitore. Le due candidate per R0 — 50*ROSmin e 50/ROSmin — entrano
    // ENTRAMBE nella griglia: e' la pendenza dei fianchi a scegliere, non noi.
    double migliorErr = 1e30;
    double bf0 = f0Iniziale, br0 = 50.0, bq = 10.0;

    auto errore = [this](double f0, double r0, double q) {
        double somma = 0.0;
        for (const Punto& p : m_punti) {
            double const d = rosDaModello(p.hz, f0, r0, q) - p.ros;
            somma += d * d;
        }
        return std::sqrt(somma / m_punti.size());
    };

    double const rA = 50.0 * m_rosMin;
    double const rB = 50.0 / m_rosMin;
    double const span = hzMax - hzMin;

    for (int gi = 0; gi < 2; ++gi) {
        double const f0da = (gi == 0) ? hzMin : bf0 - span * 0.05;
        double const f0a  = (gi == 0) ? hzMax : bf0 + span * 0.05;
        for (int i = 0; i <= 40; ++i) {
            double const f0 = f0da + (f0a - f0da) * i / 40.0;
            if (f0 <= 0.0) continue;
            for (int k = 0; k < 2; ++k) {
                double const base = (gi == 0) ? (k == 0 ? rA : rB) : br0;
                for (int j = -6; j <= 6; ++j) {
                    double const r0 = base * std::pow(1.12, j * (gi == 0 ? 1.0 : 0.25));
                    if (r0 < 1.0 || r0 > 2000.0) continue;
                    for (int m = 0; m <= 30; ++m) {
                        double const q = (gi == 0)
                                             ? 1.0 * std::pow(1.25, m)
                                             : bq * std::pow(1.05, m - 15);
                        if (q < 0.5 || q > 400.0) continue;
                        double const e = errore(f0, r0, q);
                        if (e < migliorErr) {
                            migliorErr = e; bf0 = f0; br0 = r0; bq = q;
                        }
                    }
                }
            }
        }
    }

    m_f0 = bf0;
    m_r0 = br0;
    m_q = bq;
    m_scarto = migliorErr;
    m_rosMin = rosDaModello(bf0, bf0, br0, bq);   // il ROS del modello in risonanza

    // La reattanza alla frequenza su cui si sta operando: e' il numero che
    // serve a chi sta accorciando o allungando un filo. Il segno viene dal lato
    // della risonanza, e vale per una risonanza serie.
    double const hzOra = m_bridge ? m_bridge->rigFreqHz() : m_f0;
    double const hzX = hzOra > 0.0 ? hzOra : m_f0;
    m_x = m_r0 * m_q * (hzX / m_f0 - m_f0 / hzX);

    // Larghezza di banda a ROS 2, cercata sul modello attorno a f0.
    double lo = m_f0, hi = m_f0;
    for (double f = m_f0; f > m_f0 * 0.5; f -= m_f0 * 0.0005) {
        if (rosDaModello(f, m_f0, m_r0, m_q) > 2.0) { lo = f; break; }
    }
    for (double f = m_f0; f < m_f0 * 1.5; f += m_f0 * 0.0005) {
        if (rosDaModello(f, m_f0, m_r0, m_q) > 2.0) { hi = f; break; }
    }
    m_larghezza = (hi > lo) ? (hi - lo) : 0.0;

    m_valido = true;
    m_perche.clear();
    emit stimaChanged();
}

// ── sweep comandato ─────────────────────────────────────────────────────────
//
// Questa e' l'unica parte dell'app che TRASMETTE. Alza la portante, legge,
// l'abbassa, si sposta. Sta tutta in una macchina a stati su un timer, non in
// un ciclo con attese: un ciclo bloccherebbe l'interfaccia e, soprattutto, non
// si potrebbe fermare a meta' — che su una cosa che tiene alzato un PTT e' la
// caratteristica che conta di piu'.

void AntennaProbe::avviaSweep(double daHz, double aHz, int passi)
{
    if (m_sweepAttivo) return;
    if (!m_bridge) return;

    if (!m_bridge->puoTrasmettere()) {
        m_sweepStato = tr("La radio non e' pronta a trasmettere");
        emit sweepChanged();
        return;
    }
    if (daHz <= 0.0 || aHz <= daHz) {
        m_sweepStato = tr("Intervallo non valido");
        emit sweepChanged();
        return;
    }

    m_sweepTotale = qBound(3, passi, kMaxPassi);
    m_sweepDa = daHz;
    m_sweepA = aHz;
    m_sweepFatti = 0;
    m_sweepFase = 0;
    m_sweepFreqIniziale = m_bridge->rigFreqHz();
    m_sweepAttivo = true;
    m_sweepStato = tr("Sweep in corso");
    emit sweepChanged();

    m_sweep.start(0);
}

void AntennaProbe::fermaSweep()
{
    if (!m_sweepAttivo) return;
    concludiSweep(tr("Sweep interrotto"));
}

void AntennaProbe::concludiSweep(const QString& motivo)
{
    m_sweep.stop();
    if (m_bridge) {
        // Prima si abbassa la portante, POI si torna dove si era: in ordine
        // inverso si cambierebbe frequenza mentre si trasmette, che e' il modo
        // piu' rapido per far arrabbiare un accordatore e chi ascolta.
        m_bridge->premiPtt(false);
        if (m_sweepFreqIniziale > 0.0)
            m_bridge->sintonizza(m_sweepFreqIniziale);
    }
    m_sweepAttivo = false;
    m_sweepStato = motivo;
    emit sweepChanged();
}

void AntennaProbe::passoSweep()
{
    if (!m_sweepAttivo || !m_bridge) return;

    // Se la radio smette di dichiararsi pronta a meta' sweep — cade il CAT,
    // qualcuno preme il PTT sul microfono — si smette e basta.
    if (!m_bridge->catConnected()) {
        concludiSweep(tr("Collegamento perso: sweep interrotto"));
        return;
    }

    switch (m_sweepFase) {
    case 0: {   // sintonizza
        if (m_sweepFatti >= m_sweepTotale) {
            concludiSweep(tr("Sweep completato: %1 punti").arg(m_sweepFatti));
            return;
        }
        double const hz = m_sweepDa
                          + (m_sweepA - m_sweepDa) * m_sweepFatti / double(m_sweepTotale - 1);
        m_bridge->sintonizza(hz);
        m_sweepFase = 1;
        m_sweep.start(kMsSintonia);
        break;
    }
    case 1: {   // portante su, col tono che la fa esistere
        // L'audio si manda PRIMA di alzare il PTT: il gateway lo trattiene fino
        // all'istante in cui va suonato, quindi spedirlo in anticipo e' cio' che
        // deve succedere. Alzare il PTT e poi cercare l'audio darebbe una
        // portante muta all'inizio, proprio dove i misuratori si assestano.
        m_bridge->inviaTono(kFreqTono, kMsPortante, m_livelloTono);
        m_bridge->premiPtt(true);
        m_sweepFase = 2;
        // Si aspetta l'anticipo richiesto dal gateway PIU' la durata del tono:
        // leggere prima significherebbe leggere la portante mentre e' ancora
        // silenzio, cioe' zero watt, cioe' niente.
        m_sweep.start(m_bridge->ritardoAudioMs() + kMsPortante);
        break;
    }
    case 2: {   // leggi e giu'
        double const hz = m_bridge->rigFreqHz();
        double const ros = m_bridge->rigRos();
        bool const buono = m_bridge->meterVeri() && hz > 0.0 && ros >= 1.0;
        m_bridge->premiPtt(false);
        if (buono)
            aggiungiPunto(hz, ros);
        ++m_sweepFatti;
        emit sweepChanged();
        m_sweepFase = 0;
        m_sweep.start(kMsPausa);
        break;
    }
    default:
        concludiSweep(tr("Sweep interrotto"));
        break;
    }
}
