// ANALIZZATORE D'ANTENNA — quello che il ROS puo' dire, e nient'altro.
//
// Il ROS e' un numero solo e le incognite sono due: nessun algoritmo ricava R e
// X da una lettura sola, perche' l'informazione non c'e' — su una linea senza
// perdite il MODULO del coefficiente di riflessione e' identico al carico e al
// trasmettitore, e due antenne diversissime danno lo stesso ROS.
//
// Sta nella FORMA della curva ROS(f), invece. Questa schermata raccoglie punti
// — passivamente mentre trasmetti, o con uno sweep comandato — e ci adatta un
// modello a risonanza singola. Da li' escono risonanza, R, Q e la reattanza col
// suo segno.
//
// La distinzione fra MISURATO e STIMATO e' scritta ovunque, e non e' pudore:
// e' la stessa regola per cui il quadrante mostra due trattini invece di uno
// zero. Un numero verosimile ma falso, su uno strumento, e' peggio del silenzio.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: schermo

    signal tornaAlleMisure()

    readonly property color colInk:   "#E8ECEF"
    readonly property color colLabel: "#8A939C"
    readonly property color colMuted: "#5B6670"
    readonly property color colEdge:  "#262D34"
    readonly property color colPanel: "#14181D"
    readonly property color colCyan:  "#27C4D4"
    readonly property color colGreen: "#46D67C"
    readonly property color colAmber: "#FFB454"
    readonly property color colRosso: "#FF5C5C"

    component Tasto: Button {
        id: bt
        implicitHeight: 34
        padding: 10
        contentItem: Label {
            text: bt.text
            color: bt.enabled ? colInk : colMuted
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 5
            color: bt.down ? colEdge : colPanel
            border.color: bt.enabled ? colEdge : "#1A1F24"
            border.width: 1
        }
    }

    // Una riga "etichetta / valore", con la provenienza dichiarata: chi legge
    // deve poter distinguere a colpo d'occhio cosa e' stato misurato e cosa e'
    // stato dedotto.
    component Riga: RowLayout {
        property string nome
        property string valore
        property string nota
        property color tinta: schermo.colInk
        Layout.fillWidth: true
        spacing: 8
        Label {
            text: parent.nome
            color: schermo.colLabel
            font.pixelSize: 13
            Layout.preferredWidth: 96
        }
        Label {
            text: parent.valore
            color: parent.tinta
            font.pixelSize: 16
            font.bold: true
            font.family: "monospace"
        }
        Label {
            text: parent.nota
            color: schermo.colMuted
            font.pixelSize: 10
            Layout.fillWidth: true
        }
    }

    function mhz(hz) { return hz > 0 ? (hz / 1e6).toFixed(4) : "——" }

    Flickable {
        anchors.fill: parent
        anchors.margins: 10
        contentHeight: colonna.implicitHeight
        clip: true

        ColumnLayout {
            id: colonna
            width: parent.width
            spacing: 10

            // ------------------------------------------------------ testata
            RowLayout {
                Layout.fillWidth: true
                Tasto {
                    text: qsTr("‹ Misure")
                    onClicked: schermo.tornaAlleMisure()
                }
                Label {
                    text: qsTr("ANTENNA — %1").arg(antenna.banda.length ? antenna.banda : qsTr("banda ignota"))
                    color: schermo.colCyan
                    font.pixelSize: 13
                    font.bold: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("%1 punti raccolti su questa banda").arg(antenna.numCampioni)
                color: schermo.colMuted
                font.pixelSize: 11
            }

            // ------------------------------------------- la curva misurata
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 190
                color: schermo.colPanel
                border.color: schermo.colEdge
                border.width: 1
                radius: 6

                Canvas {
                    id: curva
                    anchors.fill: parent
                    anchors.margins: 8

                    // Ridisegna quando cambiano i punti o la stima. Non a ogni
                    // fotogramma: un grafico che si ridisegna sessanta volte al
                    // secondo per dati che cambiano una volta al minuto e'
                    // batteria buttata.
                    Connections {
                        target: antenna
                        function onCampioniChanged() { curva.requestPaint() }
                        function onStimaChanged() { curva.requestPaint() }
                    }

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var W = width, H = height
                        var punti = antenna.campioni
                        if (punti.length === 0) {
                            ctx.fillStyle = "#5B6670"
                            ctx.font = "12px sans-serif"
                            ctx.fillText(qsTr("nessun punto: trasmetti, o lancia uno sweep"), 8, H / 2)
                            return
                        }

                        var fmin = punti[0].hz, fmax = punti[0].hz, rmax = 3
                        for (var i = 0; i < punti.length; ++i) {
                            fmin = Math.min(fmin, punti[i].hz)
                            fmax = Math.max(fmax, punti[i].hz)
                            rmax = Math.max(rmax, punti[i].ros)
                        }
                        if (fmax - fmin < 1000) { fmin -= 20000; fmax += 20000 }
                        rmax = Math.min(rmax, 10)
                        var X = function (hz) { return (hz - fmin) / (fmax - fmin) * W }
                        var Y = function (r)  { return H - (Math.min(r, rmax) - 1) / (rmax - 1) * H }

                        // Griglia: le righe che contano sono ROS 1,5 / 2 / 3.
                        var righe = [1.5, 2, 3, 5]
                        ctx.font = "9px monospace"
                        for (var k = 0; k < righe.length; ++k) {
                            if (righe[k] > rmax) continue
                            var y = Y(righe[k])
                            ctx.strokeStyle = righe[k] === 2 ? "#2A3A42" : "#1C2329"
                            ctx.lineWidth = 1
                            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(W, y); ctx.stroke()
                            ctx.fillStyle = "#4A555E"
                            ctx.fillText(righe[k].toString(), 2, y - 2)
                        }

                        // La curva del modello, sotto ai punti: e' una stima e
                        // sta indietro. Tratteggiata, cosi' non si confonde con
                        // cio' che e' stato letto davvero.
                        if (antenna.valido) {
                            ctx.strokeStyle = "#27C4D4"
                            ctx.globalAlpha = 0.55
                            ctx.lineWidth = 1.5
                            ctx.beginPath()
                            var primo = true
                            for (var f = fmin; f <= fmax; f += (fmax - fmin) / 120) {
                                var rm = antenna.rosModello(f)
                                if (rm <= 0) continue
                                if (primo) { ctx.moveTo(X(f), Y(rm)); primo = false }
                                else ctx.lineTo(X(f), Y(rm))
                            }
                            ctx.stroke()
                            ctx.globalAlpha = 1.0
                        }

                        // I punti misurati: pieni, in primo piano.
                        for (i = 0; i < punti.length; ++i) {
                            var px = X(punti[i].hz), py = Y(punti[i].ros)
                            ctx.fillStyle = punti[i].ros <= 2 ? "#46D67C"
                                          : (punti[i].ros <= 3 ? "#FFB454" : "#FF5C5C")
                            ctx.beginPath(); ctx.arc(px, py, 2.5, 0, Math.PI * 2); ctx.fill()
                        }

                        // Dove sei adesso.
                        if (bridge.rigFreqHz > fmin && bridge.rigFreqHz < fmax) {
                            ctx.strokeStyle = "#E8ECEF"
                            ctx.globalAlpha = 0.35
                            ctx.beginPath()
                            ctx.moveTo(X(bridge.rigFreqHz), 0)
                            ctx.lineTo(X(bridge.rigFreqHz), H)
                            ctx.stroke()
                            ctx.globalAlpha = 1.0
                        }

                        ctx.fillStyle = "#4A555E"
                        ctx.fillText((fmin / 1e6).toFixed(3), 0, H - 2)
                        var tf = (fmax / 1e6).toFixed(3)
                        ctx.fillText(tf, W - tf.length * 5.5, H - 2)
                    }
                }
            }

            // ------------------------------------------------ i numeri
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: schermo.colEdge }

            Label {
                visible: !antenna.valido
                Layout.fillWidth: true
                text: antenna.perche
                color: schermo.colAmber
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            ColumnLayout {
                visible: antenna.valido
                Layout.fillWidth: true
                spacing: 4

                Riga {
                    nome: qsTr("Risonanza")
                    valore: schermo.mhz(antenna.freqRisonanzaHz) + " MHz"
                    nota: qsTr("misurata")
                    tinta: schermo.colGreen
                }
                Riga {
                    nome: qsTr("ROS minimo")
                    valore: antenna.rosMinimo.toFixed(2)
                    nota: qsTr("misurato")
                    tinta: schermo.colGreen
                }
                // LA REATTANZA PER PRIMA, perche' e' l'unica cosa vettoriale
                // che il ROS sappia davvero dire — ed e' anche quella che serve
                // a chi ha in mano le tronchesi.
                Riga {
                    nome: qsTr("X")
                    valore: (antenna.reattanza >= 0 ? "+" : "") + antenna.reattanza.toFixed(1) + " Ω"
                    nota: antenna.reattanza > 1 ? qsTr("induttiva — antenna lunga, accorcia")
                          : (antenna.reattanza < -1 ? qsTr("capacitiva — antenna corta, allunga")
                                                    : qsTr("in risonanza"))
                    tinta: schermo.colGreen
                }
                // E QUI LE DUE CANDIDATE. Non e' indecisione: la curva del ROS
                // e' identica per le due, e sceglierne una sarebbe inventare.
                Riga {
                    nome: qsTr("R")
                    valore: antenna.resistenzaBassa.toFixed(1) + "  oppure  "
                            + antenna.resistenzaAlta.toFixed(1) + " Ω"
                    nota: qsTr("due valori possibili, il ROS non li distingue")
                    tinta: schermo.colAmber
                }
                Riga {
                    nome: qsTr("Q")
                    valore: antenna.qBassa.toFixed(1) + "  /  " + antenna.qAlta.toFixed(1)
                    nota: qsTr("uno per ciascuna R")
                    tinta: schermo.colAmber
                }
                Riga {
                    nome: qsTr("Banda ROS≤2")
                    valore: antenna.larghezzaHz > 0
                            ? (antenna.larghezzaHz / 1e3).toFixed(0) + " kHz" : "——"
                    nota: qsTr("dal modello")
                    tinta: schermo.colCyan
                }
                Riga {
                    nome: qsTr("Scarto")
                    valore: antenna.scarto.toFixed(2)
                    nota: antenna.scarto > 0.4
                          ? qsTr("alto: l'antenna non e' una risonanza sola, i valori sopra valgono poco")
                          : qsTr("il modello segue i punti")
                    tinta: antenna.scarto > 0.4 ? schermo.colAmber : schermo.colMuted
                }
            }

            // ------------------------------------------- la carta di Smith
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: width * 0.92
                Layout.maximumHeight: 330
                color: schermo.colPanel
                border.color: schermo.colEdge
                border.width: 1
                radius: 6

                Canvas {
                    id: smith
                    anchors.fill: parent
                    anchors.margins: 10

                    Connections {
                        target: antenna
                        function onStimaChanged() { smith.requestPaint() }
                        function onCampioniChanged() { smith.requestPaint() }
                    }

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var R = Math.min(width, height) / 2 - 6
                        var cx = width / 2, cy = height / 2

                        // Coordinate della carta: G sta nel cerchio unitario, e
                        // l'asse immaginario e' rovesciato perche' sullo schermo
                        // la y cresce verso il basso.
                        var PX = function (gr) { return cx + gr * R }
                        var PY = function (gi) { return cy - gi * R }

                        // Cerchi a R costante e archi a X costante: la griglia.
                        ctx.lineWidth = 1
                        ctx.strokeStyle = "#1E262C"
                        var rr = [0.2, 0.5, 1, 2, 5]
                        for (var i = 0; i < rr.length; ++i) {
                            var r = rr[i]
                            var c = r / (1 + r), rad = 1 / (1 + r)
                            ctx.beginPath()
                            ctx.arc(PX(c), PY(0), rad * R, 0, Math.PI * 2)
                            ctx.stroke()
                        }
                        var xx = [0.2, 0.5, 1, 2, 5]
                        for (i = 0; i < xx.length; ++i) {
                            for (var s = -1; s <= 1; s += 2) {
                                var x = xx[i] * s
                                // Archi centrati in (1, 1/x) con raggio |1/x|,
                                // ritagliati dal cerchio unitario.
                                ctx.save()
                                ctx.beginPath()
                                ctx.arc(cx, cy, R, 0, Math.PI * 2)
                                ctx.clip()
                                ctx.beginPath()
                                ctx.arc(PX(1), PY(1 / x), Math.abs(1 / x) * R, 0, Math.PI * 2)
                                ctx.stroke()
                                ctx.restore()
                            }
                        }
                        // Bordo e asse reale.
                        ctx.strokeStyle = "#2E3A42"
                        ctx.beginPath(); ctx.arc(cx, cy, R, 0, Math.PI * 2); ctx.stroke()
                        ctx.beginPath(); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy); ctx.stroke()

                        // IL CERCHIO DEL ROS: questo e' misurato. Dice dove sta
                        // il carico senza dire dove esattamente — e' tutto cio'
                        // che un ROS puo' affermare, e va mostrato per primo.
                        var ros = bridge.meterVeri ? bridge.rigRos : 0
                        if (ros >= 1.0) {
                            var g = (ros - 1) / (ros + 1)
                            ctx.strokeStyle = "#FFB454"
                            ctx.lineWidth = 1.5
                            ctx.beginPath(); ctx.arc(cx, cy, g * R, 0, Math.PI * 2); ctx.stroke()
                        }

                        // L'arco del modello: la stima. Percorre la banda
                        // coperta dai punti, quindi si vede da dove a dove si
                        // sa qualcosa — e dove finisce, finisce.
                        var punti = antenna.campioni
                        if (antenna.valido && punti.length > 1) {
                            var fmin = punti[0].hz, fmax = punti[punti.length - 1].hz
                            // DUE archi, uno per ramo. Disegnarne uno solo
                            // sarebbe la bugia comoda: le due antenne danno la
                            // stessa curva di ROS, e la carta e' il posto dove
                            // si vede che sono davvero due luoghi diversi.
                            for (var ramo = 0; ramo < 2; ++ramo) {
                                ctx.strokeStyle = "#27C4D4"
                                ctx.globalAlpha = ramo === 0 ? 1.0 : 0.5
                                ctx.lineWidth = ramo === 0 ? 2 : 1.5
                                ctx.beginPath()
                                var primo = true
                                for (var f = fmin; f <= fmax; f += (fmax - fmin) / 160) {
                                    var gg = antenna.gammaModello(f, ramo)
                                    if (gg.length < 2) continue
                                    if (primo) { ctx.moveTo(PX(gg[0]), PY(gg[1])); primo = false }
                                    else ctx.lineTo(PX(gg[0]), PY(gg[1]))
                                }
                                ctx.stroke()

                                // Dove sei adesso, su ciascuno dei due.
                                var go = antenna.gammaModello(bridge.rigFreqHz, ramo)
                                if (go.length === 2) {
                                    ctx.fillStyle = "#E8ECEF"
                                    ctx.beginPath()
                                    ctx.arc(PX(go[0]), PY(go[1]), ramo === 0 ? 4 : 3, 0, Math.PI * 2)
                                    ctx.fill()
                                }
                            }
                            ctx.globalAlpha = 1.0
                        }

                        ctx.fillStyle = "#4A555E"
                        ctx.font = "9px monospace"
                        ctx.fillText("50 Ω", cx + 3, cy - 4)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Cerchio ambra: il ROS misurato — il carico sta lì sopra, e senza la fase " +
                           "non si può dire dove. I DUE archi ciano sono le due antenne compatibili " +
                           "con la stessa identica curva: il loro prodotto di resistenze fa sempre " +
                           "2500, e nessuna misura di solo ROS potrà mai sceglierne una. Piano " +
                           "dell'antenna: quello che vede la radio è ruotato dalla lunghezza del cavo.")
                color: schermo.colMuted
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            // ------------------------------------------------- lo sweep
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: schermo.colEdge }

            Label {
                text: qsTr("SWEEP COMANDATO")
                color: schermo.colRosso
                font.pixelSize: 12
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Questa è l'unica funzione dell'app che TRASMETTE: sposta la radio di " +
                           "frequenza e alza la portante a ogni passo. Usala solo su una banda dove " +
                           "puoi trasmettere, con la potenza già ridotta, e ascolta prima che sia " +
                           "libera. Si ferma da sola e rimette la radio dov'era.")
                color: schermo.colMuted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Label { text: qsTr("da"); color: schermo.colLabel; font.pixelSize: 12 }
                TextField {
                    id: daField
                    Layout.preferredWidth: 92
                    color: schermo.colInk
                    placeholderTextColor: schermo.colMuted
                    font.pixelSize: 14
                    placeholderText: "MHz"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    background: Rectangle {
                        radius: 4; color: "#0F1318"
                        border.color: schermo.colEdge; border.width: 1
                    }
                }
                Label { text: qsTr("a"); color: schermo.colLabel; font.pixelSize: 12 }
                TextField {
                    id: aField
                    Layout.preferredWidth: 92
                    color: schermo.colInk
                    placeholderTextColor: schermo.colMuted
                    font.pixelSize: 14
                    placeholderText: "MHz"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    background: Rectangle {
                        radius: 4; color: "#0F1318"
                        border.color: schermo.colEdge; border.width: 1
                    }
                }
                Label { text: qsTr("passi"); color: schermo.colLabel; font.pixelSize: 12 }
                TextField {
                    id: passiField
                    Layout.preferredWidth: 56
                    color: schermo.colInk
                    font.pixelSize: 14
                    text: "21"
                    inputMethodHints: Qt.ImhDigitsOnly
                    background: Rectangle {
                        radius: 4; color: "#0F1318"
                        border.color: schermo.colEdge; border.width: 1
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Tasto {
                    Layout.fillWidth: true
                    text: antenna.sweepInCorso
                          ? qsTr("FERMA (%1%)").arg(Math.round(antenna.sweepAvanzamento * 100))
                          : qsTr("Avvia sweep")
                    // Senza il consenso del gateway il tasto resta spento: e'
                    // Decodium a sapere se questa stazione puo' essere messa in
                    // aria da qui, non l'app.
                    enabled: antenna.sweepInCorso || bridge.puoTrasmettere
                    onClicked: {
                        if (antenna.sweepInCorso) { antenna.fermaSweep(); return }
                        var da = parseFloat(daField.text) * 1e6
                        var a = parseFloat(aField.text) * 1e6
                        var n = parseInt(passiField.text, 10) || 21
                        antenna.avviaSweep(da, a, n)
                    }
                }
                Tasto {
                    text: qsTr("Dimentica")
                    enabled: !antenna.sweepInCorso && antenna.numCampioni > 0
                    onClicked: antenna.dimentica()
                }
            }

            Label {
                Layout.fillWidth: true
                text: antenna.sweepStato.length ? antenna.sweepStato
                      : (bridge.puoTrasmettere ? qsTr("La radio si dichiara pronta a trasmettere.")
                                               : qsTr("La radio non si dichiara pronta: lo sweep resta spento."))
                color: antenna.sweepInCorso ? schermo.colRosso : schermo.colMuted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                Layout.bottomMargin: 16
                text: qsTr("Il modello vale per un'antenna a risonanza singola e per una linea senza " +
                           "perdite. Il cavo vero attenua: il ROS che la radio legge è più basso di " +
                           "quello all'antenna, quindi R risulta più vicino a 50 e Q più basso del " +
                           "vero. Più cavo, più ottimismo.")
                color: schermo.colMuted
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }
        }
    }
}
