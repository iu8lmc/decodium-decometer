# Scheda del Play Store — Decometer

Testi pronti da incollare nel Play Console. I limiti di caratteri sono quelli
del negozio: sono già rispettati, ma se si riscrive qualcosa vanno ricontati.

**La lingua predefinita della scheda è `en-US`**, non l'italiano: i testi
inglesi vanno nella scheda principale, questi italiani nella traduzione `it-IT`.
La descrizione lunga inglese è in `store/descrizione-lunga-en.txt`.

---

## Nome dell'app (max 30)

```
Decometer — misuratore RF
```
*(25 caratteri)*

## Descrizione breve (max 80)

```
Potenza, ROS e ALC della radio di Decodium 4, sul telefono in rete locale.
```
*(73 caratteri)*

## Descrizione completa (max 4000)

```
Decometer porta sul telefono il frontalino RF di Decodium 4: potenza diretta,
ROS, ALC e telemetria del finale, letti dal PC attraverso la rete di casa.

Serve quando la radio la tiene Decodium. Nessun altro programma può aprire la
stessa porta seriale, e chi opera dall'altra stanza — o dall'altra parte del
mondo — trasmette senza vedere né quanta potenza sta erogando né se l'antenna
risponde. Questa app apre quella finestra.

IL QUADRANTE
È lo stesso strumento del computer, non una versione ridotta: stessa
geometria, stessi colori, stesse formule, stessa balistica degli aghi. Chi lo
conosce sul PC lo ritrova identico in mano. I valori non si inventano mai: se
la radio non fornisce un misuratore compaiono due trattini e la riga di stato
dice perché. Uno strumento che mostra un numero verosimile ma falso è peggio
di uno che tace.

CINQUE PAGINE SULLO STESSO QUADRANTE
Potenza (diretta, riflessa, ROS), adattamento (perdita di ritorno, perdita di
disadattamento, potenza netta), pilotaggio (ALC, PEP, media), segnale (dBm,
S-meter, frequenza e banda) e finale: tensione, corrente, temperatura,
compressione e posizione della manopola, per le radio che le riportano.

L'ANALIZZATORE D'ANTENNA
Raccoglie coppie di frequenza e ROS mentre operi e ne ricava la curva della
tua antenna, banda per banda: frequenza di risonanza, larghezza di banda e —
soprattutto — la reattanza col suo segno, che dice se l'antenna va accorciata
o allungata.

Sulla resistenza dichiara due valori invece di uno, e non è indecisione: dal
solo ROS le due possibilità danno curve identiche, e sceglierne una sarebbe
inventare. La carta di Smith mostra il cerchio di ciò che è stato misurato e
gli archi di ciò che è compatibile.

C'è anche uno sweep che comanda la radio e trasmette per misurare più in
fretta. Parte solo se lo avvii tu, si ferma quando vuoi e rimette la radio
dov'era.

LE DECODIFICHE
Il traffico che Decodium già emette, distinto per modo: FT8, FT4, FT2 e gli
altri, con rapporto segnale/rumore, scarto di tempo e frequenza. I conteggi in
cima fanno anche da filtro, per guardare un modo per volta.

GLI SPOT DEL CLUSTER
Gli spot del cluster DX che il PC sta ricevendo, filtrabili per banda. Non è
una seconda linea verso il nodo: sono gli stessi spot che l'operatore ha
davanti sul computer.

COME SI COLLEGA
Sul PC, in Decodium 4, si accende il gateway DecoPort e si sceglie una
password. Il computer si annuncia da sé sulla rete: nell'app la radio compare
in un elenco, si tocca, si scrive la stessa password e da lì in poi si
ricollega da sola a ogni avvio. Non c'è nessun indirizzo IP da ricordare a
memoria.

Il collegamento è autenticato e non esiste una modalità in chiaro: senza la
password non si collega, il che su una porta che espone una radio non è un
dettaglio.

COSA NON FA
Non decodifica, non registra audio, non tocca la porta seriale, non chiede il
microfono. L'unico permesso è la rete locale. L'unica cosa che comanda la
radio è lo sweep dell'analizzatore, che parte soltanto quando lo chiedi tu.

RICHIEDE
Decodium 4 con il gateway DecoPort, in esecuzione sul PC, e il telefono sulla
stessa rete WiFi. Non passa da internet e non c'è niente da esporre verso
l'esterno.

Autore: Martino, IU8LMC — iu8lmc@gmail.com
```

---

## Impostazioni della scheda

| Voce | Valore |
|---|---|
| Categoria | Strumenti |
| Tag | Utilità, Radio |
| Email di contatto | iu8lmc@gmail.com |
| Sito web | https://decometer.ft2.it |
| Informativa privacy | `store/privacy-decometer.html` → `https://community.ft2.it/downloads/privacy-decometer.html` |
| Classificazione | adatta a tutti — nessun contenuto sensibile |
| App a pagamento | no, gratuita, senza acquisti né pubblicità |

## Sicurezza dei dati (Data safety)

Da dichiarare nel questionario del Play Console:

- **Raccolta dati:** nessuna.
- **Condivisione con terzi:** nessuna.
- **Dati trattati:** indirizzo e porta del PC, password del collegamento
  DecoPort, porte di decodifiche e spot, preferenze, e i campioni dell'antenna
  (coppie di frequenza e ROS). Tutto salvato sul dispositivo e mai trasmesso
  altrove.
- **Crittografia in transito:** non applicabile — il traffico non lascia la
  rete locale e non raggiunge alcun server. È comunque autenticato: i pacchetti
  sono firmati con una chiave derivata dalla password, che non viaggia mai.
- **Cancellazione dei dati:** disinstallando l'app; i campioni dell'antenna si
  cancellano anche dalla loro schermata. Il backup automatico è disattivato
  apposta, quindi non resta nulla neanche sul cloud.

## Una cosa da non dimenticare

Dalla versione con l'analizzatore **l'app può trasmettere**. Le versioni
precedenti della scheda dicevano "non trasmette": quella frase non c'è più, e
non deve tornarci per distrazione in una riscrittura futura. Le note per i
revisori la spiegano per esteso.

## Materiali grafici

| Cosa | File | Stato |
|---|---|---|
| Icona 512×512 | `store/icona-512.png` | pronta |
| Immagine in evidenza (IT) | `store/in-evidenza-1024x500-it.png` | pronta |
| Immagine in evidenza (EN) | `store/in-evidenza-1024x500-en.png` | pronta |
| Screenshot telefono | `store/schermate/*-play.png` | tre, da rifare quando cambia l'interfaccia |
| Screenshot tablet 7" e 10" | — | facoltativi, ma l'app dichiara il supporto iPad/tablet |

Gli screenshot vanno presi dal dispositivo vero e passati per
`store/prepara-schermate.py` prima di caricarli: le catture del telefono sono
2,23 a 1 e il negozio non accetta nulla di più allungato di 1,78 a 1.

**Sono da rifare**: mostrano l'interfaccia prima del vetro nero di sfondo,
della pagina del finale e dell'analizzatore d'antenna.

## Note per i revisori

In `store/note-revisione.md`, in italiano e in inglese, con una versione breve
entro i 500 caratteri che il Play Console impone. **Non è facoltativo**: senza,
chi esamina l'app la apre, trova schermate che dicono "non connesso" e la
respinge come non funzionante.
