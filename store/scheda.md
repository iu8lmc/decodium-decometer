# Scheda del Play Store — Decometer

Testi pronti da incollare nel Play Console. I limiti di caratteri sono quelli
del negozio: sono già rispettati, ma se si riscrive qualcosa vanno ricontati.

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
ROS e ALC della radio, letti dal PC attraverso la rete di casa.

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

LE DECODIFICHE
Il traffico che Decodium già emette, distinto per modo: FT8, FT4, FT2 e gli
altri, con rapporto segnale/rumore, scarto di tempo e frequenza. I conteggi in
cima fanno anche da filtro, per guardare un modo per volta.

GLI SPOT DEL CLUSTER
Gli spot del cluster DX che il PC sta ricevendo, filtrabili per banda. Non è
una seconda linea verso il nodo: sono gli stessi spot che l'operatore ha
davanti sul computer.

COSA NON FA
Non decodifica, non trasmette, non tocca la porta seriale, non chiede il
microfono. È di sola lettura. L'unico permesso è la rete locale.

COME SI COLLEGA
Sul PC, in Decodium 4, si attiva il server CAT condiviso; nell'app si
scrivono indirizzo e porta, e da lì in poi si ricollega da sola a ogni avvio.
Se il WiFi cade, riprende da sé: un misuratore che resta spento dopo un
inciampo della rete non è un misuratore.

Serve la stessa rete WiFi del PC. Non passa da internet e non c'è niente da
esporre verso l'esterno.

RICHIEDE
Decodium 4 versione 1.0.565 o successiva, in esecuzione sul PC.
Funziona anche contro un rigctld di Hamlib qualsiasi: è lo stesso protocollo.

Autore: Martino, IU8LMC — iu8lmc@gmail.com
```

---

## Impostazioni della scheda

| Voce | Valore |
|---|---|
| Categoria | Strumenti |
| Tag | Utilità, Radio |
| Email di contatto | iu8lmc@gmail.com |
| Sito web | https://github.com/iu8lmc/decodium-decometer |
| Informativa privacy | *(vedi `privacy.md`: va pubblicata a un indirizzo raggiungibile)* |
| Classificazione | adatta a tutti — nessun contenuto sensibile |
| App a pagamento | no, gratuita, senza acquisti né pubblicità |

## Sicurezza dei dati (Data safety)

Da dichiarare nel questionario del Play Console:

- **Raccolta dati:** nessuna.
- **Condivisione con terzi:** nessuna.
- **Dati trattati:** solo l'indirizzo IP e la porta del PC scelti dall'utente,
  salvati sul dispositivo e mai trasmessi altrove.
- **Crittografia in transito:** non applicabile — il traffico non lascia la
  rete locale e non raggiunge alcun server.
- **Cancellazione dei dati:** disinstallando l'app; il backup automatico è
  disattivato apposta, quindi non resta nulla neanche sul cloud.

## Materiali grafici

| Cosa | File | Stato |
|---|---|---|
| Icona 512×512 | `store/icona-512.png` | pronta |
| Immagine in evidenza 1024×500 | `store/feature-1024x500.png` | pronta |
| Screenshot telefono (min 2, max 8) | — | **da fare sul telefono** |
| Screenshot tablet 7" e 10" | — | facoltativi, ma l'app dichiara il supporto iPad/tablet |

Gli screenshot vanno presi dal dispositivo vero, non dalla prova desktop: il
negozio li mostra come sono e una finestra di PC si riconosce. Ne bastano tre,
uno per schermata — quadrante, decodifiche, spot.
