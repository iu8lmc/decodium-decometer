# Pubblicazione sul Play Console — sequenza operativa

Tutto quello che serve è già in questa cartella. Qui sotto l'ordine in cui il
Play Console chiede le cose, con i valori pronti: dove c'è un blocco di testo,
si copia e si incolla.

---

## Prima di aprire il browser

**1. Firma il bundle**

```
firma_aab.bat
```

Produce `dist/playstore/Decometer-1.1.0.aab`. Se chiede la password e la
sbagli, non succede niente di male: il bundle firmato precedente resta al suo
posto (lo script firma su un file di lavoro).

**2. Cancella il vecchio bundle**

`dist/playstore/Decometer-1.0.0.aab` è firmato e valido ma dichiara la 1.0.0 e
non ha le correzioni del 19 agosto — fra cui quella della finestra storta su
Android. Caricarlo per sbaglio brucerebbe il codice di versione 1 con la build
sbagliata.

**3. Screenshot dal telefono**

Minimo due, massimo otto, dal dispositivo vero. Il taglio consigliato: uno per
schermata — quadrante, decodifiche, spot — più eventualmente la schermata del
segnale. Su Android si fanno con volume-giù + tasto laterale.

Poi vanno passati per lo script, **sempre**:

```
python store/prepara-schermate.py catture\*.png
```

Le catture del telefono sono 2,23 a 1 e il negozio non accetta nulla di più
allungato di 1,78 a 1: le rifiuta al caricamento, dopo che si è compilata
tutta la scheda. Lo script aggiunge due bande dello stesso fondo dell'app
invece di ritagliare, così non si perde quello che si voleva mostrare. Le
immagini pronte finiscono in `store/schermate/`.

**4. Pubblica l'informativa privacy**

Carica `store/privacy-decometer.html` sul tuo sito, dove sta gia' quella
dell'app sorella:

```
https://community.ft2.it/downloads/privacy-decometer.html
```

GitHub Pages **non e' una strada**: questo repository e' privato, e su un
repository privato Pages richiede un piano a pagamento. Provandolo, le pagine
hanno risposto per pochi minuti e poi sono state disattivate — un indirizzo
che muore dopo la pubblicazione lascerebbe la scheda del negozio con un
collegamento morto, che e' motivo di sospensione.

---

## Nel Play Console

### Crea l'app

| Campo | Valore |
|---|---|
| Nome | `Decometer — misuratore RF` |
| Lingua predefinita | Italiano |
| App o gioco | App |
| Gratuita o a pagamento | Gratuita |

Il nome del pacchetto — `com.ft2.decometer` — non si sceglie qui: lo prende dal
bundle al primo caricamento, e **da quel momento non si cambia più**.

### Scheda del negozio principale

Nome, descrizione breve e descrizione completa sono in `store/scheda.md`, già
entro i limiti di caratteri (30 / 80 / 4000). La versione inglese della
descrizione lunga è in `store/descrizione-lunga-en.txt`: conviene aggiungere
la scheda in inglese, perché il pubblico dei radioamatori non è italiano.

| Risorsa | File |
|---|---|
| Icona 512×512 | `store/icona-512.png` |
| Immagine in evidenza (IT) | `store/in-evidenza-1024x500-it.png` |
| Immagine in evidenza (EN) | `store/in-evidenza-1024x500-en.png` |
| Screenshot telefono | quelli preparati al punto 3 |

Categoria **Strumenti**, email di contatto `iu8lmc@gmail.com`, sito
`https://github.com/iu8lmc/decodium-decometer`.

### Sicurezza dei dati

Le risposte sono in `store/scheda.md`. In sintesi: **nessun dato raccolto,
nessuno condiviso**. L'indirizzo IP e la porta che l'utente inserisce restano
sul dispositivo e non vengono trasmessi a nessuno; il backup automatico è
disattivato apposta nel manifest, quindi non finiscono neanche sul cloud.

Alla domanda sulla crittografia in transito: non applicabile — il traffico non
lascia la rete locale e non raggiunge alcun server.

### Classificazione dei contenuti

Questionario IARC. Nessun contenuto sensibile di alcun tipo: l'esito atteso è
la fascia per tutti.

### Contenuti dell'app

- **Annunci:** nessuno.
- **Accesso all'app:** tutte le funzioni sono disponibili senza credenziali,
  ma il testo per i revisori va incollato lo stesso. Sta in
  `store/note-revisione.md`, in italiano e in inglese, ed è lo stesso
  accorgimento usato per Decodium Mobile: senza, chi esamina l'app la apre,
  trova tre schermate che dicono "non connesso" e la respinge come non
  funzionante.

- **Pubblico di destinazione:** 18+ o 13+, a scelta; non è rivolta ai bambini.

### Versione di produzione

Carica `dist/playstore/Decometer-1.1.0.aab`.

Note di versione suggerite:

```
Prima pubblicazione.

Il frontalino RF di Decodium 4 sul telefono: potenza diretta, ROS e ALC letti
dal PC in rete locale, con lo stesso quadrante del programma da tavolo.

Oltre alle misure: il traffico delle decodifiche distinto per modo, gli spot
del cluster DX, S-meter e frequenza, allarme di ROS con vibrazione.
```

---

## Dopo il primo caricamento

Il **codice di versione** è ora 2. Non si riusa e non si torna indietro: va
alzato in `CMakeLists.txt` a ogni pacchetto caricato, anche quando il nome
della versione resta uguale.

E una cosa da sapere prima di premere pubblica: le finestre **Cluster** e
**S-meter** funzionano solo con un Decodium 4 che le supporti — il
ritrasmettitore di spot e il polling di `RIG_LEVEL_STRENGTH` sono modifiche
recenti. Se Decometer arriva sul negozio prima di quel Decodium, chi lo
installa vede due schermate su tre dire "non collegato".
