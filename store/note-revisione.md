# Note per la revisione — Google Play Console

Questo testo va incollato in **Play Console → Prova e pubblica → Accesso
all'app**.

Serve perché Decometer non è un'app che funziona da sola: è il quadrante di una
radio che sta altrove. Senza spiegarlo, chi la esamina la apre, trova tre
schermate che dicono "non connesso" e la respinge come non funzionante. È lo
stesso motivo per cui Decodium Mobile ha una nota analoga — con una differenza
importante: **qui non esiste una modalità dimostrativa**, e non è una
dimenticanza. Questo strumento non inventa mai un valore: mostrare numeri finti
per superare una revisione sarebbe esattamente ciò che promette di non fare.

---

## Testo da incollare (italiano)

> Decometer è il misuratore remoto di una stazione radioamatoriale. Non misura
> nulla da sé: legge potenza diretta, ROS e ALC da un computer collegato al
> ricetrasmettitore, sul quale gira il programma Decodium 4, e li mostra sul
> telefono attraverso la rete WiFi di casa.
>
> Per provarla servono quindi un PC Windows con Decodium 4 in esecuzione, una
> radio collegata a quel PC, e il telefono sulla stessa rete locale.
>
> COSA SI VEDE SENZA IL PC: l'app si apre e mostra l'interfaccia completa — il
> quadrante, la schermata delle decodifiche, quella degli spot e le
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
> il ROS e l'ALC si aggiornano mentre si trasmette, e le altre due schermate si
> riempiono con le decodifiche e con gli spot del cluster DX.
>
> L'app è di sola lettura: non comanda la radio, non trasmette, non registra
> audio. L'unico permesso di rilievo è l'accesso alla rete locale.

---

## Testo da incollare (inglese)

> Decometer is the remote meter of an amateur radio station. It measures
> nothing by itself: it reads forward power, SWR and ALC from a computer
> connected to the transceiver, running the Decodium 4 program, and displays
> them on the phone over the home WiFi network.
>
> Testing it therefore requires a Windows PC running Decodium 4, a radio
> connected to that PC, and the phone on the same local network.
>
> WHAT YOU SEE WITHOUT THE PC: the app opens and shows the complete interface —
> the meter face, the decodes screen, the DX cluster spots screen and the
> network settings. The meters stay at rest and the status line explicitly
> reads "CAT non connesso" (CAT not connected), because there is nothing to
> measure.
>
> There is no demo mode, and that is deliberate: this instrument exists to tell
> whether an antenna is working while you transmit, and a plausible but false
> reading would be worse than no reading at all. For the same reason, when the
> radio does not provide a measurement, two dashes appear instead of a zero.
>
> WHAT YOU SEE WITH THE PC CONNECTED: the needles follow the transmitted power
> in real time, SWR and ALC update while transmitting, and the other two
> screens fill with decodes and DX cluster spots.
>
> The app is read-only: it does not control the radio, does not transmit and
> does not record audio. The only meaningful permission is local network
> access.
