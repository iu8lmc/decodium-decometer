# Note di rilascio 1.2.0 — quindici lingue

Da incollare nel Play Console, in **Versione di produzione → Note di versione**,
una per ogni lingua della scheda. Il limite è 500 caratteri per lingua: la più
lunga qui sta a 497.

Se il Console chiede il formato con i tag di lingua — quello che permette di
incollare tutte le traduzioni in una volta sola — il file già pronto è
**`store/note-rilascio-console.txt`**, e contiene le stesse quindici lingue coi
codici del negozio (`it-IT`, `en-US`, `de-DE`, `fr-FR`, `es-ES`, `ca`, `nl-NL`,
`da-DK`, `hu-HU`, `ro`, `lv`, `ru-RU`, `ja-JP`, `zh-CN`, `zh-TW`).

Fino alla 1.1.0 quel file ne conteneva **dodici**: catalano, rumeno e lettone
erano elencati qui ma non c'erano là dentro, e chi incollava si ritrovava tre
lingue senza note senza accorgersene. Ora ci sono tutte e quindici.

Le lingue sono le stesse di Decodium 4, così chi conosce il programma da tavolo
trova l'app nella lingua in cui già lo usa.

Sul termine tecnico: dove i radioamatori di quella lingua dicono SWR si è
lasciato SWR, dove dicono altro (ROS in italiano e rumeno, ROE in spagnolo e
catalano) si è usato il loro.

## Cosa dicono, in sostanza

Quattro cose, nell'ordine in cui contano per chi aggiorna:

1. **Il collegamento si fa da solo** — il PC si annuncia sulla rete e la radio
   compare in un elenco. Non c'è più un indirizzo IP da scrivere a mano.
2. **La pagina del finale** — tensione, corrente, compressione e manopola, per
   le radio che le riportano.
3. **L'analizzatore d'antenna** — ed è la novità vera, perché è anche la prima
   funzione dell'app che **trasmette**. Le note lo dicono esplicitamente in
   ogni lingua: non è una cosa da lasciare implicita in una nota di versione.
4. **Il vetro nero e le liste che tornano in cima** — i due difetti visibili
   segnalati sulla 1.1.0.

---

## it — Italiano

```
Si collega da solo: il PC si annuncia in rete, tocchi la radio nell'elenco e scrivi la password. Niente più indirizzi IP.

Nuova pagina del finale: tensione, corrente, compressione e posizione della manopola.

Nuovo analizzatore d'antenna: raccoglie ROS e frequenza mentre operi, trova la risonanza e dà la reattanza col segno, cioè se accorciare o allungare. Con carta di Smith e uno sweep che — solo se lo avvii tu — trasmette.

Sfondo a vetro nero sui tablet. Le liste tornano in cima da sole.
```
*(496 caratteri)*

## en — English

```
It connects on its own: the PC announces itself, you tap the radio in a list and type the password. No more IP addresses.

New power-amplifier page: drain voltage and current, compression, power knob.

New antenna analyser: it collects SWR and frequency while you operate, finds resonance and gives the reactance with its sign — whether to shorten or lengthen. With a Smith chart and a sweep that transmits, only if you start it.

Black-glass background on tablets. Lists scroll back on their own.
```
*(497 caratteri)*

## de — Deutsch

```
Verbindet sich selbst: Der PC meldet sich im Netz, Sie tippen das Funkgerät in der Liste an und geben das Kennwort ein. Keine IP-Adressen mehr.

Neue Endstufenseite: Spannung, Strom, Kompression, Leistungsregler.

Neuer Antennenanalysator: sammelt SWR und Frequenz im Betrieb, findet die Resonanz und nennt die Reaktanz mit Vorzeichen — kürzen oder verlängern. Mit Smith-Diagramm und einem Sweep, der nur auf Ihren Befehl sendet.

Schwarzglas-Hintergrund auf Tablets.
```
*(467 caratteri)*

## fr — Français

```
Il se connecte tout seul : le PC s'annonce sur le réseau, vous touchez la radio dans une liste et saisissez le mot de passe. Plus d'adresse IP à retenir.

Nouvelle page de l'ampli : tension, courant, compression, bouton de puissance.

Nouvel analyseur d'antenne : il collecte ROS et fréquence pendant que vous opérez, trouve la résonance et donne la réactance avec son signe — raccourcir ou allonger. Avec abaque de Smith et un balayage qui émet, seulement si vous le lancez.
```
*(475 caratteri)*

## es — Español

```
Se conecta solo: el PC se anuncia en la red, tocas la radio en una lista y escribes la contraseña. No más direcciones IP de memoria.

Nueva página del amplificador: tensión, corriente, compresión y mando de potencia.

Nuevo analizador de antena: recoge ROE y frecuencia mientras operas, halla la resonancia y da la reactancia con su signo, es decir si acortar o alargar. Con carta de Smith y un barrido que transmite, solo si lo inicias tú.
```
*(440 caratteri)*

## ca — Català

```
Es connecta sol: el PC s'anuncia a la xarxa, toques la ràdio en una llista i escrius la contrasenya. Ja no cal recordar adreces IP.

Nova pàgina de l'amplificador: tensió, corrent, compressió i el comandament de potència.

Nou analitzador d'antena: recull ROE i freqüència mentre operes, troba la ressonància i dóna la reactància amb el seu signe — si cal escurçar o allargar. Amb carta de Smith i un escombrat que transmet, només si l'inicies tu.
```
*(447 caratteri)*

## nl — Nederlands

```
Hij verbindt vanzelf: de pc meldt zich op het netwerk, u tikt de set in een lijst aan en typt het wachtwoord. Geen IP-adressen meer.

Nieuwe eindtrappagina: spanning, stroom, compressie en de vermogensknop.

Nieuwe antenneanalysator: verzamelt SWR en frequentie terwijl u werkt, vindt de resonantie en geeft de reactantie met teken — inkorten of verlengen. Met een smithkaart en een sweep die zendt, alleen als u hem start.
```
*(423 caratteri)*

## da — Dansk

```
Den forbinder selv: pc'en melder sig på nettet, du trykker på radioen i en liste og skriver adgangskoden. Ingen IP-adresser mere.

Ny slutttrinsside: spænding, strøm, kompression og effektknappen.

Ny antenneanalysator: samler SWR og frekvens mens du kører, finder resonansen og angiver reaktansen med fortegn — afkort eller forlæng. Med Smith-diagram og et sweep, der sender, kun hvis du starter det.
```
*(401 caratteri)*

## hu — Magyar

```
Magától kapcsolódik: a PC bejelentkezik a hálózaton, a listában megérinti a rádiót és beírja a jelszót. Nincs több IP-cím.

Új végfok-oldal: feszültség, áram, kompresszió és a teljesítményszabályzó.

Új antennaanalizátor: üzem közben gyűjti az SWR-t és a frekvenciát, megtalálja a rezonanciát, és előjelesen megadja a reaktanciát — rövidíteni vagy hosszabbítani kell. Smith-diagrammal és adással járó pásztázással, amely csak az Ön parancsára indul.
```
*(449 caratteri)*

## ro — Română

```
Se conectează singur: PC-ul se anunță în rețea, atingi stația în listă și scrii parola. Nu mai sunt adrese IP de reținut.

Pagină nouă pentru etajul final: tensiune, curent, compresie și butonul de putere.

Analizor de antenă nou: adună ROS și frecvența în timp ce lucrezi, găsește rezonanța și dă reactanța cu semnul ei — dacă să scurtezi sau să lungești. Cu diagramă Smith și o baleiere care emite, doar dacă o pornești tu.
```
*(425 caratteri)*

## lv — Latviešu

```
Tas savienojas pats: dators paziņo par sevi tīklā, jūs pieskaraties stacijai sarakstā un ievadāt paroli. IP adreses vairs nav jāatceras.

Jauna gala pakāpes lapa: spriegums, strāva, kompresija un jaudas poga.

Jauns antenas analizators: darba laikā vāc SWR un frekvenci, atrod rezonansi un norāda reaktivitāti ar zīmi — vai saīsināt, vai pagarināt. Ar Smita diagrammu un izvērsi, kas raida tikai tad, ja jūs to sākat.
```
*(417 caratteri)*

## ru — Русский

```
Подключается сам: компьютер объявляет себя в сети, вы касаетесь трансивера в списке и вводите пароль. Больше не нужно помнить IP-адрес.

Новая страница усилителя: напряжение, ток, компрессия и регулятор мощности.

Новый антенный анализатор: собирает КСВ и частоту во время работы, находит резонанс и даёт реактивность со знаком — укоротить или удлинить. С диаграммой Смита и разверткой, которая передаёт только по вашей команде.
```
*(428 caratteri)*

## ja — 日本語

```
自動で接続します。PC がネットワークに自らを知らせ、一覧から無線機を選んでパスワードを入力するだけ。IP アドレスを覚える必要はありません。

終段の新しいページ：電圧、電流、コンプレッション、パワーつまみ。

新しいアンテナ・アナライザー：運用中に SWR と周波数を集め、共振点を求め、リアクタンスを符号つきで示します（短くするか長くするか）。スミス図表と、自分で始めたときだけ送信するスイープ付き。
```
*(203 caratteri)*

## zh — 简体中文

```
自动连接：电脑在网络中自我通告，您在列表中点选电台并输入密码，无需再记 IP 地址。

新增末级页面：电压、电流、压缩和功率旋钮。

新增天线分析仪：在您操作时采集驻波比与频率，找出谐振点，并给出带符号的电抗——告诉您该剪短还是加长。附史密斯圆图，以及仅在您启动时才发射的扫频。

平板上采用黑玻璃背景。列表会自动回到顶部。
```
*(162 caratteri)*

## zh_TW — 繁體中文

```
自動連線：電腦在網路中自我通告，您在清單中點選電台並輸入密碼，不必再記 IP 位址。

新增末級頁面：電壓、電流、壓縮與功率旋鈕。

新增天線分析儀：在您操作時採集駐波比與頻率，找出諧振點，並給出帶符號的電抗——告訴您該剪短還是加長。附史密斯圓圖，以及僅在您啟動時才發射的掃頻。

平板上採用黑玻璃背景。清單會自動回到頂端。
```
*(162 caratteri)*
