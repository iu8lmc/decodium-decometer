# Dati da incollare nel Play Console

Foglio di riferimento: tutti i valori identificativi in un posto solo, così non
vanno cercati in tre file diversi mentre si compila la scheda. Sono verificati
sul bundle firmato, non scritti a memoria.

---

## Identità dell'app

| Campo | Valore |
|---|---|
| **Nome dell'app** (max 30) | `Decometer — misuratore RF` |
| Nome del pacchetto | `com.ft2.decometer` |
| Versione | `1.1.0` |
| Codice di versione | `2` |
| SDK di destinazione | 36 (Android 16) |
| SDK minimo | 24 (Android 7.0) |
| Architetture | arm64-v8a, armeabi-v7a, x86_64 |

Il nome del pacchetto **non si digita**: lo prende dal bundle al primo
caricamento, e da quel momento non si cambia più. Se il Console lo chiede
prima, significa che si sta creando l'app a mano: va scritto identico a come
sta qui, minuscole comprese.

Se `Decometer — misuratore RF` viene rifiutato per il trattino lungo, usare
`Decometer - misuratore RF` (trattino semplice) o `Decometer RF Meter`.

## Contatti e collegamenti

| Campo | Valore |
|---|---|
| Email di contatto | `iu8lmc@gmail.com` |
| Sito web | `https://iu8lmc.github.io/decodium-decometer/` |
| Informativa sulla privacy | `https://iu8lmc.github.io/decodium-decometer/privacy.html` |
| Codice sorgente | `https://github.com/iu8lmc/decodium-decometer` |
| Sviluppatore | Martino, IU8LMC |

## Classificazione

| Campo | Valore |
|---|---|
| Categoria | Strumenti |
| Tipo | App (non gioco) |
| Prezzo | Gratuita |
| Acquisti in-app | No |
| Annunci | No |
| Lingua predefinita | Italiano (aggiungere anche Inglese) |

## Descrizione breve (max 80)

```
Potenza, ROS e ALC della radio di Decodium 4, sul telefono in rete locale.
```
*(73 caratteri)*

Versione inglese:

```
Forward power, SWR and ALC of your radio, from the PC over local WiFi.
```
*(69 caratteri)*

## Il file da caricare

```
dist\playstore\Decometer-1.1.0.aab
```

50.220.656 byte, firmato con `decodium-upload.jks` — `jar verified`.

**Non caricare** `Decometer-1.1.0-da-firmare.aab`: è lo stesso pacchetto senza
firma, e il Console lo rifiuta.

---

## Dove sta il resto

| Cosa | File |
|---|---|
| Descrizione completa (IT) | `store/scheda.md` |
| Descrizione completa (EN) | `store/descrizione-lunga-en.txt` |
| Note per i revisori (IT + EN) | `store/note-revisione.md` |
| Risposte su sicurezza dei dati | `store/scheda.md` |
| Icona e immagini in evidenza | `store/icona-512.png`, `store/in-evidenza-1024x500-{it,en}.png` |
| Screenshot | `store/schermate/*-play.png` |
| Procedura completa | `store/console.md` |
