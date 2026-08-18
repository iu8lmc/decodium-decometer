# Decometer — misuratore RF per Android e iOS

Il frontalino **DECOMETER** di Decodium 4 come app a sé, per il telefono:
potenza diretta, ROS e ALC della radio letti **dal PC**, in rete locale.

Serve quando la radio la tiene Decodium: nessun altro programma può aprire la
stessa porta seriale, e chi opera dall'altra stanza — o dall'altra parte del
mondo — trasmetteva senza vedere né quanta potenza stesse erogando né se
l'antenna rispondesse. Questa app apre quella finestra, e nient'altro.

## Cosa non fa, e perché

Non decodifica, non trasmette, non tocca la seriale, non chiede il microfono.
È **sola lettura**. Per questo sta in una cartella separata da
`decodium-mobile`: quell'app porta con sé tutto il motore di decodifica
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

I valori non si inventano: se la radio non fornisce un misuratore compaiono
due trattini e la riga di stato dice perché. A trasmettitore fermo i
misuratori di trasmissione non misurano niente, e il server risponde "non
disponibile" invece di zero — uno zero direbbe «nessuna potenza, ROS
perfetto», che somiglia a una stazione che va benissimo.

## Struttura

| File | Contenuto |
|---|---|
| `Main.qml` | schermata di collegamento + frontalino a tutto schermo |
| `Decometer.qml` | il frontalino, copia invariata dall'app completa |
| `MeterBridge.{hpp,cpp}` | client TCP rigctl: poll del PTT, tre livelli, nient'altro |
| `main.cpp` | avvio, registra `bridge` nel contesto QML |
| `android/` | manifest, gradle, icone |
| `ios/` | `Info.plist`, icona, `configure_ios.sh`, keep-screen-on nativo |

La superficie di proprietà di `MeterBridge` (`rigWatt`, `rigRos`, `rigAlc`,
`meterVeri`, `catConnected`, …) ricalca deliberatamente quella di `AppBridge`
dell'app completa: è ciò che permette di usare `Decometer.qml` così com'è.

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

Il protocollo è verificato end-to-end contro il server CAT di Decodium 4 e
contro un server di prova: connessione, poll del PTT, lettura dei tre livelli
in forma estesa, aggiornamento del quadrante. **Il layout su schermo di
telefono non è ancora stato verificato su un dispositivo reale**: la prova
desktop su Windows a scala 175% mostra un artefatto di composizione della
finestra (il contenuto viene disegnato in coordinate logiche su una superficie
di dimensione diversa) che non riguarda il layout in sé — le misure interne
sono corrette — ma che impedisce di giudicare l'aspetto da lì. Va guardato sul
telefono.
