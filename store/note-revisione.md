# Note per la revisione — Google Play Console e App Store

Questo testo va incollato in **Play Console → Prova e pubblica → Accesso
all'app**, e nel campo equivalente di App Store Connect.

Serve perché Decometer non è un'app che funziona da sola: è il quadrante di una
radio che sta altrove. Senza spiegarlo, chi la esamina la apre, trova schermate
che dicono "non connesso" e la respinge come non funzionante. È lo stesso
accorgimento usato per Decodium Mobile — con una differenza importante: **qui
non esiste una modalità dimostrativa**, e non è una dimenticanza. Questo
strumento non inventa mai un valore: mostrare numeri finti per superare una
revisione sarebbe esattamente ciò che promette di non fare.

**Dalla versione con l'analizzatore d'antenna l'app può comandare la radio.** Il
campo limite di caratteri del Play Console è 500, quindi il testo qui sotto è
già dentro quel limite; la spiegazione lunga sta più in basso, per App Store
Connect, che è più generoso.

---

## Testo breve, entro i 500 caratteri (Play Console)

> Decometer is the remote meter of an amateur radio station. It measures nothing
> by itself: over WiFi it reads power, SWR, ALC and PA telemetry from a Windows
> PC running Decodium 4, connected to a transceiver.
>
> Without that PC the app opens and every screen is reachable; the meters stay
> at rest and the status line reads "CAT non connesso". There is no demo mode, by
> design: the app never shows an invented reading.
>
> One function transmits: the antenna sweep, started only by the user.

*(496 caratteri)*

---

## Testo lungo (italiano)

> Decometer è il misuratore remoto di una stazione radioamatoriale. Non misura
> nulla da sé: legge potenza diretta, ROS, ALC, S-meter e telemetria del finale
> da un computer collegato al ricetrasmettitore, sul quale gira il programma
> Decodium 4, e li mostra sul telefono attraverso la rete WiFi di casa.
>
> Per provarla servono quindi un PC Windows con Decodium 4 in esecuzione, una
> radio collegata a quel PC, e il telefono sulla stessa rete locale.
>
> COSA SI VEDE SENZA IL PC: l'app si apre e mostra l'interfaccia completa — il
> quadrante, le decodifiche, gli spot, l'analizzatore d'antenna e le
> impostazioni di rete. I misuratori restano fermi e la riga di stato dice
> esplicitamente "CAT non connesso", perché non c'è niente da misurare.
>
> Non esiste una modalità dimostrativa, ed è una scelta: questo strumento serve
> a decidere se un'antenna sta funzionando mentre si trasmette, e un numero
> verosimile ma falso sarebbe peggio di nessun numero. Per lo stesso motivo,
> quando la radio non fornisce una misura, al suo posto compaiono due trattini
> e non uno zero.
>
> COSA SI VEDE CON IL PC COLLEGATO: gli aghi seguono la potenza in tempo reale,
> ROS e ALC si aggiornano mentre si trasmette, la pagina del finale mostra
> tensione, corrente e temperatura quando la radio le riporta, e le altre
> schermate si riempiono con le decodifiche e con gli spot del cluster DX.
>
> UNA FUNZIONE TRASMETTE, e va detto chiaramente. L'analizzatore d'antenna ha
> uno "sweep" che cambia la frequenza della radio e ne alza la portante per
> misurare il ROS a frequenze diverse. Parte SOLO da un comando esplicito
> dell'utente nella sua schermata, si può fermare a metà, rimette la radio dove
> era, e la portante ha una scadenza automatica: se il collegamento cade, scende
> da sé. Nessun'altra parte dell'app manda comandi alla radio. L'analizzatore
> funziona anche senza trasmettere, limitandosi a osservare le misure che passano
> mentre è l'operatore a trasmettere.
>
> Il collegamento è autenticato con una password condivisa fra il PC e il
> telefono: senza quella, l'app non si collega affatto. Non esiste modalità in
> chiaro. Un revisore senza il PC vedrà quindi la schermata di rete con l'elenco
> delle radio vuoto, che è il comportamento corretto.
>
> L'app non decodifica, non registra audio, non tocca la porta seriale e non
> chiede il microfono. L'unico permesso di rilievo è l'accesso alla rete locale.

---

## Testo lungo (inglese)

> Decometer is the remote meter of an amateur radio station. It measures nothing
> by itself: it reads forward power, SWR, ALC, S-meter and power-amplifier
> telemetry from a computer connected to the transceiver, running the Decodium 4
> program, and displays them on the phone over the home WiFi network.
>
> Testing it therefore requires a Windows PC running Decodium 4, a radio
> connected to that PC, and the phone on the same local network.
>
> WHAT YOU SEE WITHOUT THE PC: the app opens and shows the complete interface —
> the meter face, the decodes screen, the DX cluster spots screen, the antenna
> analyser and the network settings. The meters stay at rest and the status line
> explicitly reads "CAT non connesso" (CAT not connected), because there is
> nothing to measure.
>
> There is no demo mode, and that is deliberate: this instrument exists to tell
> whether an antenna is working while you transmit, and a plausible but false
> reading would be worse than no reading at all. For the same reason, when the
> radio does not provide a measurement, two dashes appear instead of a zero.
>
> WHAT YOU SEE WITH THE PC CONNECTED: the needles follow the transmitted power
> in real time, SWR and ALC update while transmitting, the PA page shows drain
> voltage, current and temperature where the radio reports them, and the other
> screens fill with decodes and DX cluster spots.
>
> ONE FUNCTION TRANSMITS, and it should be stated plainly. The antenna analyser
> has a sweep that changes the radio's frequency and keys the carrier in order to
> measure SWR at several frequencies. It starts ONLY from an explicit user action
> on its own screen, it can be stopped mid-way, it restores the original
> frequency, and the carrier has an automatic timeout: if the link drops, it
> falls by itself. No other part of the app sends commands to the radio. The
> analyser also works without transmitting, by observing the readings that pass
> while the operator transmits.
>
> The link is authenticated with a password shared between the PC and the phone:
> without it the app does not connect at all. There is no cleartext mode. A
> reviewer without the PC will therefore see the network screen with an empty
> list of radios, which is the correct behaviour.
>
> The app does not decode, does not record audio, does not touch the serial port
> and does not ask for the microphone. The only meaningful permission is local
> network access.
