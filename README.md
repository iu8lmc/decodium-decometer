# Decometer — la stazione di Decodium 4 sul telefono

Tre finestre su quello che sta facendo il PC, in rete locale: il frontalino
**DECOMETER** con potenza diretta, ROS e ALC; il **traffico UDP delle
decodifiche**, distinte per modo; gli **spot del cluster DX** che Decodium sta
ricevendo.

Serve quando la radio la tiene Decodium: nessun altro programma può aprire la
stessa porta seriale, e chi opera dall'altra stanza — o dall'altra parte del
mondo — trasmetteva senza vedere né quanta potenza stesse erogando né se
l'antenna rispondesse. Questa app apre quella finestra, e nient'altro.

## Cosa non fa, e perché

Non decodifica, non trasmette, non tocca la seriale, non chiede il microfono.
È **sola lettura**: ascolta tre socket e disegna. Per questo sta in una
cartella separata da `decodium-mobile`: quell'app porta con sé tutto il motore di decodifica
(FFTW, Boost, Opus, il core C++ di Decodium), che qui non servirebbe a nulla
se non ad appesantire il pacchetto e la lista dei permessi.

## Come si collega

Sul PC, in **Decodium 4**, si attiva il server CAT condiviso (Impostazioni →
CAT). Il server sta in ascolto sulla porta **4533** e parla il protocollo
`rigctl` di Hamlib, quindi questa app non ha bisogno di sapere nulla della
radio: chiede i livelli per nome (`SWR`, `ALC`, `RFPOWER_METER_WATTS`) e li
riceve già calibrati, gli stessi numeri che Decodium mostra sul computer.

Nell'app si scrivono IP del PC e porta, e da lì in poi si ricollega da sola a
ogni avvio. Serve la **stessa rete WiFi**: non passa da internet, e non c'è
niente da esporre verso l'esterno.

Funziona anche contro un `rigctld` di Hamlib qualsiasi, non solo contro
Decodium — è lo stesso protocollo.

### Le decodifiche

Non c'è niente da aggiungere sul PC: Decodium, come ogni discendente di
WSJT-X, spedisce già ogni decodifica in un pacchetto UDP. Perché arrivino qui
basta scrivere l'indirizzo del telefono in **Impostazioni → Reporting → UDP
Server** (di serie `127.0.0.1:2237`), oppure un indirizzo multicast se devono
riceverle in più di uno — in quel caso l'app alza da sé il blocco multicast di
Android, senza il quale il telefono scarta i pacchetti di gruppo e resta in
ascolto di un silenzio che sembra un guasto.

Il formato è quello pubblico di WSJT-X, e si rispetta fino in fondo: la
versione di serializzazione dipende dallo schema dichiarato nel pacchetto
(1 → `Qt_5_0`, 2 → `Qt_5_2`, 3 → `Qt_5_4`). Leggerlo con quella sbagliata non
dà errore: dà numeri plausibili e falsi, che su uno strumento sono il peggio
possibile.

### Gli spot

Il telefono **non** apre una propria linea telnet verso il cluster: quella la
tiene già il PC, ed è quella giusta — un nodo non gradisce due sessioni dello
stesso nominativo, e gli spot che contano sono quelli che l'operatore sta
guardando sul computer, non un secondo flusso simile ma diverso.

Per questo gli spot li rivende Decodium: il servizio `DecodiumSpotShare`
(`Decodium-4.0/src/services/`) è il gemello della CAT condivisa — un
interruttore, una porta (**4534** di serie), nessun comando accettato in
entrata. Manda una riga JSON per messaggio:

```
{"tipo":"benvenuto","servizio":"decodium-spot-share","versione":1,"attesi":37}
{"tipo":"spot","dxCall":"JA1YYY","frequency":14074.0,"band":"20m",...}
{"tipo":"battito"}
```

Chi si collega riceve prima gli ultimi spot già raccolti — così non guarda una
lista vuota finché il nodo non si degna — e poi quelli nuovi. Il battito ogni
venti secondi serve a distinguere una linea viva e silenziosa da una caduta:
su TCP le due cose si somigliano per minuti.

### Versione minima di Decodium

Serve **Decodium 4 v1.0.565** o successiva. Le versioni precedenti
rispondevano ai livelli nella forma "semplice" (solo il numero) anche quando
il client chiedeva quella estesa, e un client che si aspetta la forma estesa
scarta quel numero perché non può attribuirlo ad alcun misuratore: i quadranti
resterebbero vuoti pur essendo la radio letta correttamente. Dalla 1.0.565 il
server risponde nella forma richiesta; dalla 1.0.564 dichiara anche di avere
quei misuratori, senza di che un client Hamlib non li chiederebbe affatto.

## Il quadrante

`Decometer.qml` è **lo stesso file** dell'app completa
(`decodium-mobile/androidapp/Decometer.qml`), copiato senza una modifica:
stessa geometria su tela fissa 900×420, stessi colori, stesse formule, stessa
balistica degli aghi. Chi conosce lo strumento sul computer lo ritrova
identico in mano.

Quando quel file cambia nell'app completa, qui si **ricopia**, non si
modifica: due copie che divergono sono due strumenti diversi che dicono di
essere lo stesso.

C'è **una sola eccezione**, ed è marcata nel file come `INNESTO DELL'APP
STANDALONE`: i due tasti DECODE e CLUSTER sotto AUTO, con le proprietà che li
governano (`finestreDisponibili`, `decodeVivo`, `clusterVivo`) e i due segnali
che emettono. Sono **spenti di default**: nell'app completa il frontalino
resta identico a prima, non un tasto in più né un pixel diverso, perché lì le
altre finestre si aprono dal menu del programma. Qui il quadrante è tutta
l'applicazione, e da qualche parte si deve pur passare per le altre due
schermate. **Alla prossima ricopiatura l'innesto va riapplicato**, altrimenti
l'app perde la navigazione.

Quei tasti stanno lì, e non in una barra fissa, per una ragione precisa: il
frontalino è disegnato su tela fissa e scalato per intero, quindi qualunque
striscia sopra o sotto non gli toglie un margine — gli toglie *scala*, e il
misuratore si rimpicciolisce tutto insieme. Pagare uno strumento più piccolo
per tenere tre tasti sempre in vista non conviene, su una cosa che si guarda
mentre si trasmette.

I valori non si inventano: se la radio non fornisce un misuratore compaiono
due trattini e la riga di stato dice perché. A trasmettitore fermo i
misuratori di trasmissione non misurano niente, e il server risponde "non
disponibile" invece di zero — uno zero direbbe «nessuna potenza, ROS
perfetto», che somiglia a una stazione che va benissimo.

## Struttura

| File | Contenuto |
|---|---|
| `Main.qml` | le tre schermate, la navigazione, le impostazioni di rete |
| `Decometer.qml` | il frontalino, dall'app completa più l'innesto dei due tasti |
| `DecodeScreen.qml` | le decodifiche, con i conteggi per modo che fanno da filtro |
| `ClusterScreen.qml` | gli spot, con i conteggi per banda che fanno da filtro |
| `MeterBridge.{hpp,cpp}` | client TCP rigctl: poll del PTT, tre livelli, nient'altro |
| `DecodeFeed.{hpp,cpp}` | ricevitore UDP del protocollo WSJT-X |
| `SpotFeed.{hpp,cpp}` | client TCP del ritrasmettitore di spot |
| `main.cpp` | avvio, registra `bridge`, `decodeFeed` e `spotFeed` nel contesto QML |
| `android/` | manifest, gradle, icone |
| `ios/` | `Info.plist`, icona, `configure_ios.sh`, keep-screen-on nativo |

Le tre sorgenti non si conoscono fra loro ed è voluto: chi apre l'app per
guardare la potenza mentre trasmette non deve vedersi fermare il quadrante
perché il cluster non risponde. Ognuna dice da sé come sta — il puntino
accanto al nome del tasto — e ognuna, se la linea cade, se la riprende da
sola.

Si naviga dal quadrante: i due tasti sotto AUTO aprono le altre schermate, e
il `‹` in cima a ognuna riporta alle misure (su Android va anche il tasto di
sistema, che su iPhone non esiste).

La superficie di proprietà di `MeterBridge` (`rigWatt`, `rigRos`, `rigAlc`,
`meterVeri`, `catConnected`, …) ricalca deliberatamente quella di `AppBridge`
dell'app completa: è ciò che permette di usare `Decometer.qml` così com'è.

## Autore

**Martino, IU8LMC** — <iu8lmc@gmail.com>

L'indirizzo compare anche in fondo alle impostazioni dell'app, come
collegamento: chi trova un difetto sul telefono deve poterlo segnalare da
lì, senza andarlo a cercare altrove.

## Build

### Android
Servono JDK 17, Android SDK, NDK r26d e il kit Qt per Android.
```
configure_android.bat
apk_android.bat     REM APK di prova, da firmare e installare con adb
aab_android.bat     REM AAB per il Play Store
```
Il pacchetto è `com.ft2.decometer` — diverso da `com.ft2.decodium`, perché sul
negozio è un prodotto separato, con la propria scheda. Una volta pubblicato
**non si cambia più**.

### Play Store
```
aab_android.bat                   REM produce l'AAB, NON firmato
```
L'AAB esce da `build/android/android-build/build/outputs/bundle/release/`.
Il negozio non accetta più APK per le app nuove: vuole l'AAB, e da quello
ricava lui gli APK per ogni dispositivo.

La firma è un passo a parte e **non** sta in questo repository: la chiave di
pubblicazione va custodita fuori, perché perderla significa non poter più
aggiornare l'app.

```
jarsigner -keystore <chiave.jks> -signedjar Decometer-1.0.0.aab           android-build-release.aab <alias>
```

I materiali della scheda stanno in `store/`: testi pronti da incollare
(`scheda.md`), informativa privacy (`privacy.md`, da pubblicare a un indirizzo
raggiungibile — il negozio la pretende anche per le app che non raccolgono
nulla) e la grafica, rigenerabile con `python tools/make_store_art.py`. Gli
screenshot vanno presi dal telefono vero.

### iOS
```
cd ios && ./configure_ios.sh      # export APPLE_TEAM_ID=... per firmare
```
Nessuna libreria da precompilare: basta il kit Qt per iOS.

### Desktop (per provare in fretta)
```
cmake -S . -B build_desktop -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build_desktop
```

## Stato

I tre protocolli sono verificati end-to-end, contro Decodium 4 e contro
sorgenti di prova:

- **misure** — connessione, poll del PTT, lettura dei tre livelli in forma
  estesa, aggiornamento del quadrante; e la ripresa da sola dopo una caduta
  della linea (server tolto di mezzo per nove secondi, riconnessione 130 ms
  dopo il suo ritorno, polling ripartito);
- **decodifiche** — pacchetti WSJT-X in FT8, FT4 e FT2 letti con lo schema
  dichiarato, conteggi per modo esatti, `Status` interpretato (frequenza,
  modo, trasmettitore acceso);
- **spot** — benvenuto, spot già raccolti all'arrivo, spot nuovi in tempo
  reale, filtro per banda.

**Il layout su schermo di telefono non è ancora stato verificato su un
dispositivo reale.** L'artefatto che rendeva inutilizzabile la prova desktop su
Windows a scala 175% è invece risolto: il processo non dichiarava la
consapevolezza del DPI, così il sistema virtualizzava la finestra — i 480×900
punti chiesti finivano in 274×514 pixel veri e il disegno usciva dai bordi,
con l'aria di un layout sbagliato che sbagliato non era. `main.cpp` ora lo
dichiara all'avvio (rispettando `QT_QPA_PLATFORM` se impostata da fuori), e la
finestra si compone giusta. Su Android e iOS non si presentava, perché lì il
DPI lo governa il sistema.
