#ifndef DECOMETER_ANTENNA_PROBE_HPP
#define DECOMETER_ANTENNA_PROBE_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class MeterBridge;

// ANALIZZATORE D'ANTENNA — quello che si puo' davvero ricavare dal CAT.
//
// Il ROS da' il MODULO del coefficiente di riflessione e nient'altro. Su una
// linea senza perdite quel modulo e' lo stesso al carico e al trasmettitore:
// la linea ruota la fase, non la comprime. Quindi da una singola lettura non
// esiste algoritmo che tiri fuori R e X — non manca il codice, manca proprio
// l'informazione, e due antenne diversissime danno lo stesso ROS.
//
// Dove l'informazione c'e' e' nella FORMA della curva ROS(f). Un'antenna vicina
// alla risonanza si comporta come un RLC serie:
//
//     X(f) = R0 * Q * (f/f0 - f0/f)          Z = R0 + jX
//     G    = (Z - 50) / (Z + 50)             ROS = (1+|G|)/(1-|G|)
//
// Tre incognite — f0, R0, Q — e una curva misurata su cui adattarle. Il minimo
// del ROS da' f0 e la larghezza della campana da' la coppia (R0, Q).
//
// LA COPPIA, non i due valori separatamente, e questa e' la parte che va capita
// prima di fidarsi dei numeri. La trasformazione
//
//     R0 -> 2500/R0        Q -> Q * R0^2 / 2500
//
// lascia il ROS IDENTICO a ogni frequenza. Non simile: identico, e la verifica
// numerica lo conferma con scarto 0,001 su dati puliti. Quindi un'antenna da 25
// ohm con Q 12 e una da 100 ohm con Q 6 producono la stessa identica curva, e
// nessun adattamento potra' mai distinguerle. Si dichiarano ENTRAMBE, e il loro
// prodotto e' sempre 2500 — cioe' 50 al quadrato, il che e' anche il modo piu'
// rapido di ricordarsi che sono l'una l'inverso normalizzato dell'altra.
//
// Quello che invece si determina, ed e' il motivo per cui vale la pena fare
// tutto questo, e' la REATTANZA: X = R0 * Q * delta e' invariante sotto quella
// stessa trasformazione. Modulo e segno, senza ambiguita'. Per chi sta
// accorciando o allungando un'antenna e' esattamente il numero che serve.
//
// Cosa resta STIMA e non misura, e va detto in interfaccia:
//   - il modello e' a risonanza singola: un'antenna multibanda o una trappola
//     lo violano, e lo scarto del fit lo denuncia;
//   - la linea si assume senza perdite. Non lo e': l'attenuazione del cavo
//     abbassa il ROS visto dalla radio, quindi R risulta piu' vicino a 50 e Q
//     piu' basso del vero. Piu' cavo, piu' bugia — e non la si puo' correggere
//     senza sapere quanto cavo c'e';
//   - il segno di X viene dal lato della risonanza, che vale per una risonanza
//     serie. Su un carico che risuona in parallelo e' rovesciato.
//
// La carta di Smith mostra il luogo al PIANO DELL'ANTENNA, non a quello della
// radio: la fase che vedrebbe il trasmettitore e' ruotata dalla lunghezza della
// linea, che non sappiamo. Per regolare un'antenna serve il piano dell'antenna,
// quindi e' anche il piano giusto.
class AntennaProbe : public QObject
{
    Q_OBJECT

    // I campioni raccolti nella banda corrente: {hz, ros}. E' cio' che si
    // disegna, ed e' l'unica cosa qui dentro che sia stata misurata.
    Q_PROPERTY(QVariantList campioni READ campioni NOTIFY campioniChanged)
    Q_PROPERTY(QString banda READ banda NOTIFY campioniChanged)
    Q_PROPERTY(int numCampioni READ numCampioni NOTIFY campioniChanged)

    // Il risultato dell'adattamento. "valido" e' falso finche' i campioni non
    // bastano o non coprono abbastanza banda: un fit su tre punti tutti vicini
    // darebbe numeri, e sarebbero inventati.
    Q_PROPERTY(bool valido READ valido NOTIFY stimaChanged)
    Q_PROPERTY(QString perche READ perche NOTIFY stimaChanged)
    Q_PROPERTY(double freqRisonanzaHz READ freqRisonanzaHz NOTIFY stimaChanged)
    Q_PROPERTY(double rosMinimo READ rosMinimo NOTIFY stimaChanged)
    // Le due candidate per R, che il ROS non sa separare. Bassa*Alta = 2500.
    Q_PROPERTY(double resistenzaBassa READ resistenzaBassa NOTIFY stimaChanged)
    Q_PROPERTY(double resistenzaAlta READ resistenzaAlta NOTIFY stimaChanged)
    Q_PROPERTY(double qBassa READ qBassa NOTIFY stimaChanged)
    Q_PROPERTY(double qAlta READ qAlta NOTIFY stimaChanged)
    // Questa invece e' determinata, segno compreso.
    Q_PROPERTY(double reattanza READ reattanza NOTIFY stimaChanged)
    Q_PROPERTY(double larghezzaHz READ larghezzaHz NOTIFY stimaChanged)
    // Quanto il modello si scosta dai punti, in ROS. Sopra qualche decimo
    // l'antenna non e' una risonanza sola e i numeri sopra vanno guardati con
    // sospetto: meglio dirlo che nasconderlo.
    Q_PROPERTY(double scarto READ scarto NOTIFY stimaChanged)

    // Lo sweep comandato.
    Q_PROPERTY(bool sweepInCorso READ sweepInCorso NOTIFY sweepChanged)
    Q_PROPERTY(double sweepAvanzamento READ sweepAvanzamento NOTIFY sweepChanged)
    Q_PROPERTY(QString sweepStato READ sweepStato NOTIFY sweepChanged)

public:
    explicit AntennaProbe(MeterBridge* bridge, QObject* parent = nullptr);
    ~AntennaProbe() override;

    QVariantList campioni() const;
    QString banda() const { return m_banda; }
    int numCampioni() const { return m_punti.size(); }

    bool valido() const { return m_valido; }
    QString perche() const { return m_perche; }
    double freqRisonanzaHz() const { return m_f0; }
    double rosMinimo() const { return m_rosMin; }
    double resistenzaBassa() const { return qMin(m_r0, 2500.0 / m_r0); }
    double resistenzaAlta() const { return qMax(m_r0, 2500.0 / m_r0); }
    // Il Q che accompagna ciascuna delle due: quello della R piu' bassa e'
    // sempre il piu' alto, perche' l'uno e' l'altro scalato di (R/50)^2.
    double qBassa() const;
    double qAlta() const;
    double reattanza() const { return m_x; }
    double larghezzaHz() const { return m_larghezza; }
    double scarto() const { return m_scarto; }

    bool sweepInCorso() const { return m_sweepAttivo; }
    double sweepAvanzamento() const { return m_sweepTotale > 0
                                          ? double(m_sweepFatti) / m_sweepTotale : 0.0; }
    QString sweepStato() const { return m_sweepStato; }

    // Il coefficiente di riflessione previsto dal modello a una frequenza, per
    // la carta di Smith. Ritorna {re, im} normalizzati; lista vuota se non c'e'
    // una stima valida — cosi' il disegno non puo' inventare un punto.
    // ramo 0 = la R piu' bassa, ramo 1 = la piu' alta. Sulla carta di Smith si
    // disegnano tutt'e due, perche' tutt'e due sono compatibili con la misura.
    Q_INVOKABLE QVariantList gammaModello(double hz, int ramo = 0) const;
    // Il ROS previsto dal modello: serve a disegnare la curva continua sopra i
    // punti misurati, cosi' si vede a occhio quanto il modello li segue.
    Q_INVOKABLE double rosModello(double hz) const;

public slots:
    // Cancella i campioni della banda corrente. Serve dopo aver toccato
    // l'antenna: i punti di prima descrivono un'altra antenna.
    Q_INVOKABLE void dimentica();

    // SWEEP COMANDATO. Trasmette davvero: alza il PTT su ogni passo, legge, lo
    // abbassa. Va chiamato solo da un gesto esplicito dell'utente.
    Q_INVOKABLE void avviaSweep(double daHz, double aHz, int passi);
    Q_INVOKABLE void fermaSweep();

signals:
    void campioniChanged();
    void stimaChanged();
    void sweepChanged();

private slots:
    // Un campione passivo: arriva mentre l'operatore trasmette per conto suo.
    void osservaMisura();
    void passoSweep();

private:
    struct Punto { double hz; double ros; };

    void aggiungiPunto(double hz, double ros);
    void ricalcola();
    void carica();
    void salva();
    void cambiaBanda(const QString& nuova);
    void concludiSweep(const QString& motivo);
    static double rosDaModello(double hz, double f0, double r0, double q);

    MeterBridge* m_bridge {nullptr};
    QSettings m_settings;

    QString m_banda;
    QVector<Punto> m_punti;
    // Un campione ogni tanto, non dodici al secondo: la stessa portante ripetuta
    // non aggiunge informazione, ingrossa il file e appiattisce il fit sul punto
    // dove si e' trasmesso di piu'.
    QElapsedTimer m_ultimoCampione;
    static constexpr int kIntervalloCampione = 900;   // ms
    // Due punti piu' vicini di cosi' sono lo stesso punto: si tiene il ROS
    // peggiore, che e' quello che conta quando si giudica un'antenna.
    static constexpr double kRisoluzioneHz = 2000.0;
    static constexpr int kMaxPunti = 400;

    bool m_valido {false};
    QString m_perche;
    double m_f0 {0.0};
    double m_rosMin {0.0};
    double m_r0 {0.0};
    double m_x {0.0};
    double m_q {0.0};
    double m_larghezza {0.0};
    double m_scarto {0.0};

    // ---- sweep
    QTimer m_sweep;
    bool m_sweepAttivo {false};
    int m_sweepFase {0};
    int m_sweepFatti {0};
    int m_sweepTotale {0};
    double m_sweepDa {0.0};
    double m_sweepA {0.0};
    double m_sweepFreqIniziale {0.0};
    QString m_sweepStato;
    // Il PTT non resta alzato piu' di cosi', mai: se qualcosa va storto la
    // portante deve cadere da sola. La libreria DecoPort ha gia' una scadenza
    // sua lato gateway, ma una guardia che dipende dalla rete non e' una
    // guardia — questa sta sul telefono, dove sta il dito.
    static constexpr int kMsSintonia = 350;
    static constexpr int kMsPortante = 450;
    static constexpr int kMsPausa    = 250;
    static constexpr int kMaxPassi   = 41;
};

#endif // DECOMETER_ANTENNA_PROBE_HPP
