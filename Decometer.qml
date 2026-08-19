// DECOMETER — RF Vector Meter, per il telefono.
//
// E' lo STESSO frontalino del programma da tavolo
// (qml/decodium/components/DecometerWindow.qml del repo Decodium 4): stessa
// geometria, stessi colori, stesse formule, stessa balistica degli aghi. Il
// disegno e' costruito su una tela fissa di 900×420 punti e poi scalato per
// entrare nello spazio disponibile, esattamente come fa il desktop: cosi' le
// proporzioni non si deformano e chi conosce lo strumento sul computer lo
// ritrova identico in mano.
//
// Cosa cambia rispetto al desktop, e perche':
//   - non c'e' finestra da spostare ne' da chiudere: qui e' un pannello, e il
//     tasto in alto a destra serve a portarlo a tutto schermo;
//   - non c'e' la scelta fra eccitatrice e amplificatore, perche' l'app del
//     telefono non parla con un amplificatore;
//   - le misure arrivano dal ponte del telefono (Hamlib attraverso Decolink)
//     invece che dal ponte del desktop. Sono le stesse grandezze e la stessa
//     provenienza.
//
// I VALORI NON SI INVENTANO, come sull'originale: se la radio non fornisce un
// misuratore, al suo posto compaiono due trattini e la riga di stato dice
// perche'. Uno strumento che mostra un numero verosimile ma falso e' peggio di
// uno che tace.

import QtQuick
import QtQuick.Controls

Item {
    id: dm

    // Chi ospita decide se il frontalino puo' essere ingrandito e cosa
    // succede quando lo si chiude.
    property bool ingranditoDisponibile: true
    property bool ingrandito: false
    // Vero dopo il primo ingrandimento: serve solo a togliere di mezzo il
    // suggerimento, che a quel punto ha finito il suo lavoro.
    property bool giaIngrandito: false
    signal apriIntero()
    signal chiudiIntero()

    // INNESTO DELL'APP STANDALONE (Decometer, telefono). Nell'app completa
    // resta spento e il frontalino e' identico a prima: nessun tasto in piu',
    // nessun pixel diverso. Serve perche' li' le altre finestre si aprono dal
    // menu del programma, mentre qui il quadrante e' tutta l'applicazione e
    // da qualche parte bisogna pur passare per le altre due schermate.
    // Stanno sotto AUTO, nella colonna dei comandi, dove il pollice arriva
    // gia' per cambiare portata.
    property bool finestreDisponibili: false
    property bool decodeVivo: false
    property bool clusterVivo: false
    signal apriDecode()
    signal apriCluster()

    readonly property int faceWidth: 900
    readonly property int faceHeight: 420

    // ---------------------------------------------------------------- colori
    readonly property color colInk:   "#E8ECEF"
    readonly property color colCyan:  "#27C4D4"
    readonly property color colGreen: "#46D67C"
    readonly property color colAmber: "#FFB454"
    readonly property color colRed:   "#FF4A4A"
    readonly property color colMuted: "#5B6670"
    readonly property color colLabel: "#8A939C"
    readonly property color colDim:   "#4E5A63"
    readonly property color colEdge:  "#262D34"
    readonly property color colPanel: "#14181D"

    // ---------------------------------------------------------------- misure
    // Sorgenti reali, nessun valore inventato.
    readonly property bool catUp:    bridge.catConnected
    readonly property bool telemetry: bridge.rigMetersOn
    readonly property bool appRfOn:  bridge.txActive || bridge.rigPtt
    readonly property real rawFwd:   bridge.meterVeri && bridge.rigWatt > 0 ? bridge.rigWatt : 0
    readonly property real rawSwr:   bridge.meterVeri && bridge.rigRos >= 1 ? bridge.rigRos : 1
    readonly property bool txOn:     appRfOn || rawFwd > 0.05
    readonly property bool swrValid: catUp && bridge.meterVeri && bridge.rigRos >= 1
    readonly property bool pwrValid: catUp && bridge.meterVeri && rawFwd > 0
    // L'ALC del Yaesu arriva sulla scala del frontalino, 0–255: qui si mostra
    // in percentuale, che e' come lo legge il desktop.
    readonly property real rawAlc:   bridge.rigAlc * 100 / 255
    readonly property bool alcValid: catUp && telemetry && bridge.rigAlc > 0
    readonly property bool rfActive: txOn || rawFwd > 0.05

    function fmtW(v) { return v >= 100 ? v.toFixed(1) : v.toFixed(2) }

    // coefficiente di riflessione: rho = (ROS-1)/(ROS+1)
    readonly property real rho: rawSwr > 1 ? (rawSwr - 1) / (rawSwr + 1) : 0
    readonly property real returnLossDb: rho > 0.0005 ? -20 * Math.log(rho) / Math.LN10 : 99
    readonly property real mismatchLossDb: rho > 0.0005 ? -10 * Math.log(1 - rho * rho) / Math.LN10 : 0
    readonly property real netW: Math.max(0, vFwd - vRef)
    // L'impedenza vera vorrebbe un sensore vettoriale: dal solo ROS si sa
    // soltanto fra quali estremi puo' stare la parte resistiva.
    readonly property real rMin: 50 / Math.max(1, rawSwr)
    readonly property real rMax: 50 * Math.max(1, rawSwr)

    readonly property string statusLine: {
        if (!catUp)            return qsTr("NO CAT LINK")
        if (!telemetry)        return qsTr("TELEMETRY OFF — ENABLE METERS")
        if (rfActive && !pwrValid && !swrValid) return qsTr("RIG REPORTS NO METER")
        var n = bridge.rigModel && bridge.rigModel.length ? bridge.rigModel : qsTr("connected")
        return "CAT: " + n.toUpperCase()
    }
    readonly property color statusColor: (!catUp || !telemetry) ? colAmber : colDim

    // stato balistico
    property real vFwd: 0
    property real vRef: 0
    property real vSwr: 1
    property real pkFwdV: 0;  property real pkFwdT: 0
    property real pkRefV: 0;  property real pkRefT: 0
    property real pkSwrV: 0;  property real pkSwrT: 0
    property real pepW: 0
    property real avgW: 0
    property real txSeconds: 0

    // portate: 5 / 50 / 500 / 5000 W
    property int  rangeIdx: 1
    property bool autoRange: true
    property real overT: 0
    readonly property var fsArr: [5, 50, 500, 5000]
    readonly property var fsMult: ["×0.1", "×1", "×10", "×100"]
    readonly property var fsWatt: ["5 W", "50 W", "500 W", "5 kW"]
    function effFs() { return fsArr[rangeIdx] }

    property int  screenIdx: 0
    readonly property int screenCount: 3
    property real clock: 0

    // ------------------------------------------------------------ balistica
    // Attacco istantaneo, rilascio esponenziale: e' il comportamento di una
    // bobina vera, e senza di esso l'ago salta venticinque volte al secondo e
    // non si legge. Le costanti sono quelle del desktop.
    readonly property bool settling: vFwd > 0.01 || vRef > 0.001
                                     || pkFwdV > 0.01 || pkRefV > 0.001 || pkSwrV > 0.002

    Timer {
        interval: (dm.rfActive || dm.settling) ? 40 : 200
        running: dm.visible
        repeat: true
        onTriggered: dm.tick(interval / 1000)
    }

    function tick(dt) {
        clock += dt
        var fwdT = (rfActive && pwrValid) ? rawFwd : 0
        var swrT = (rfActive && swrValid) ? rawSwr : vSwr

        function rel(cur, tgt, tau) {
            return tgt >= cur ? tgt : cur + (tgt - cur) * (1 - Math.exp(-dt / tau))
        }
        var refT = fwdT * rho * rho

        vFwd = rel(vFwd, fwdT, 0.5)
        vRef = rel(vRef, refT, 0.5)
        if (rfActive && swrValid) vSwr = rel(vSwr, swrT, 0.4)

        // ritenuta di picco 3 s, poi discesa con tau 0.9 s
        function peak(v, pv, pt) {
            if (v >= pv - 1e-9) return [v, clock]
            if (clock - pt > 3) return [pv + (0 - pv) * (1 - Math.exp(-dt / 0.9)), pt]
            return [pv, pt]
        }
        var p
        p = peak(vFwd, pkFwdV, pkFwdT); pkFwdV = p[0]; pkFwdT = p[1]
        p = peak(vRef, pkRefV, pkRefT); pkRefV = p[0]; pkRefT = p[1]
        var sF = Math.max(0, (vSwr - 1) / (vSwr + 1))
        p = peak(sF, pkSwrV, pkSwrT); pkSwrV = p[0]; pkSwrT = p[1]

        if (rfActive) {
            txSeconds += dt
            if (vFwd > pepW) pepW = vFwd
            avgW = avgW + (vFwd - avgW) * (1 - Math.exp(-dt / 3.0))
        }

        if (autoRange && vFwd > 0.95 * fsArr[rangeIdx] && rangeIdx < 3) {
            overT += dt
            if (overT > 0.5) { rangeIdx++; overT = 0 }
        } else if (autoRange && rangeIdx > 0 && pkFwdV < 0.35 * fsArr[rangeIdx - 1]) {
            overT -= dt
            if (overT < -2.5) { rangeIdx--; overT = 0 }
        } else {
            overT = 0
        }
        gauge.requestPaint()
    }

    component Readout: Item {
        property string tag: ""
        property string value: ""
        property string unit: ""
        property color tint: "#E8ECEF"
        property int valueSize: 17
        width: parent ? parent.width : 0
        height: valueSize + 4
        Text {
            id: tagText
            anchors.left: parent.left
            anchors.baseline: valText.baseline
            text: parent.tag
            width: 46
            font.pixelSize: 13; font.bold: true; font.family: "monospace"
            color: parent.tint
        }
        Text {
            id: unitText
            anchors.right: parent.right
            anchors.baseline: valText.baseline
            text: parent.unit
            font.pixelSize: 10; font.family: "monospace"
            color: parent.tint
        }
        Text {
            id: valText
            anchors.left: tagText.right
            anchors.right: unitText.left
            anchors.rightMargin: 8
            horizontalAlignment: Text.AlignRight
            text: parent.value
            font.pixelSize: parent.valueSize; font.bold: true; font.family: "monospace"
            color: parent.tint
        }
    }

    // DUE TOCCHI PER INGRANDIRE. Nella colonna del waterfall il frontalino gira
    // a un quarto di scala: i chip delle portate misurano quindici punti veri e
    // il tasto d'ingrandimento nemmeno dieci — sotto la soglia di quello che un
    // dito puo' centrare. Finche' e' piccolo, quindi, l'intera superficie fa una
    // cosa sola: due tocchi e va a tutto schermo, dove i comandi tornano grandi
    // abbastanza da servire a qualcosa.
    //
    // Sta SOPRA il frontalino, e copre i comandi apposta: in quello stato erano
    // comunque inutilizzabili, e coprirli evita anche di cambiare portata per
    // sbaglio sfiorando lo strumento.
    MouseArea {
        anchors.fill: parent
        z: 50
        enabled: !dm.ingrandito && dm.ingranditoDisponibile
        onDoubleClicked: dm.apriIntero()
    }

    // Un gesto che non si vede non si scopre: la scritta lo dice, e sparisce
    // dopo il primo ingrandimento perche' a quel punto lo si e' imparato.
    Rectangle {
        visible: !dm.ingrandito && dm.ingranditoDisponibile && !dm.giaIngrandito
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 4 }
        width: etichetta.width + 14; height: etichetta.height + 6
        radius: 4; z: 51
        color: Qt.rgba(0, 0, 0, 0.55)
        Text {
            id: etichetta
            anchors.centerIn: parent
            text: qsTr("due tocchi per ingrandire")
            color: dm.colLabel; font.pixelSize: 9; font.letterSpacing: 0.5
        }
    }

    // ---------------------------------------------------------- il frontalino
    Item {
        id: faceHolder
        anchors.fill: parent
        clip: true
        readonly property real fit: Math.min(width / dm.faceWidth, height / dm.faceHeight)

        Item {
            id: face
            width: dm.faceWidth
            height: dm.faceHeight
            anchors.centerIn: parent
            scale: faceHolder.fit

            Rectangle {
                anchors.fill: parent
                radius: 10
                border.color: "#23292F"
                border.width: 1
                gradient: Gradient {
                    GradientStop { position: 0.0;  color: "#171B21" }
                    GradientStop { position: 0.55; color: "#12161B" }
                    GradientStop { position: 1.0;  color: "#101418" }
                }
            }

            // Doppio tocco per tornare dalla schermata intera. Sta qui, PRIMA
            // dei comandi, cosi' chip e frecce restano sopra e continuano a
            // ricevere i tocchi singoli: il doppio tocco vale solo sulle zone
            // libere del frontalino.
            MouseArea {
                anchors.fill: parent
                enabled: dm.ingrandito
                onDoubleClicked: dm.chiudiIntero()
            }

            // ------------------------------------------------- scale ad arco
            Canvas {
                id: gauge
                x: 14; y: 6
                width: 640; height: 264
                renderStrategy: Canvas.Cooperative

                onPaint: {
                    var g = getContext("2d")
                    g.clearRect(0, 0, 640, 264)

                    // Il centro degli archi sta MOLTO sotto la tela: e' quello
                    // che rende le scale quasi rettilinee, come su uno
                    // strumento a bobina largo e basso.
                    var cx = 320, cy = 640
                    var A0 = -Math.PI * 2 / 3, A1 = -Math.PI / 3
                    var dim = dm.txOn ? 1 : 0.3
                    var GREEN = dm.colGreen, AMBER = dm.colAmber, RED = dm.colRed

                    function ang(f) { return A0 + (A1 - A0) * f }
                    function tick(R, f, len) {
                        var a = ang(f)
                        g.beginPath()
                        g.moveTo(cx + R * Math.cos(a), cy + R * Math.sin(a))
                        g.lineTo(cx + (R + len) * Math.cos(a), cy + (R + len) * Math.sin(a))
                        g.stroke()
                    }
                    function lbl(R, f, s) {
                        var a = ang(f)
                        g.fillText(s, cx + R * Math.cos(a), cy + R * Math.sin(a) + 3)
                    }

                    g.textAlign = "center"
                    g.font = "9px monospace"
                    g.strokeStyle = "#3A424A"
                    g.lineWidth = 1
                    g.fillStyle = "#6A737C"

                    var fs = dm.effFs()
                    var stepW = fs / 10
                    for (var i = 0; i <= 10; i++) {
                        tick(601, i / 10, 5)
                        var v = stepW * i
                        lbl(616, i / 10, v >= 1000 ? (v / 1000) + "k" : String(Math.round(v * 100) / 100))
                    }
                    for (var j = 0; j <= 5; j++) {
                        tick(536, j / 5, 5)
                        var vr = fs * 0.2 / 5 * j
                        lbl(550, j / 5, vr >= 1000 ? (vr / 1000) + "k" : String(Math.round(vr * 100) / 100))
                    }
                    var swrL = [[1, "1.0"], [1.25, "1.25"], [1.5, "1.5"], [2, "2"], [3, "3"], [5, "5"], [1e9, "∞"]]
                    for (var k = 0; k < swrL.length; k++) {
                        var s = swrL[k][0]
                        var f = Math.min(1, (s - 1) / (s + 1))
                        tick(476, f, 5)
                        lbl(490, f, swrL[k][1])
                    }
                    g.font = "8px monospace"
                    g.fillStyle = "#525C64"
                    var alcT = [[0.25, "25"], [0.5, "50"], [0.75, "75"]]
                    for (var q = 0; q < alcT.length; q++) {
                        tick(452, alcT[q][0], -4)
                        lbl(443, alcT[q][0], alcT[q][1])
                    }

                    g.font = "bold 9px sans-serif"
                    g.fillStyle = "#7A848D"
                    g.textAlign = "left"
                    g.fillText("FWD", 18, 88)
                    g.fillText("REF", 52, 150)
                    g.fillText("SWR", 87, 212)
                    g.font = "bold 8px sans-serif"
                    g.fillStyle = "#525C64"
                    g.fillText("ALC %", 126, 247)

                    function arc(R, len, n, litF, peakF, colFn) {
                        var step = (A1 - A0) / n
                        for (var i2 = 0; i2 < n; i2++) {
                            var ff = (i2 + 0.5) / n
                            var a2 = A0 + step * (i2 + 0.5)
                            var ca = Math.cos(a2), sa = Math.sin(a2)
                            g.beginPath()
                            g.moveTo(cx + R * ca, cy + R * sa)
                            g.lineTo(cx + (R + len) * ca, cy + (R + len) * sa)
                            if (ff <= litF) {
                                g.strokeStyle = colFn(ff)
                                g.lineWidth = 5
                                g.globalAlpha = dim
                                g.stroke()
                                if (dim === 1) {
                                    g.globalAlpha = 0.22
                                    g.lineWidth = 9
                                    g.stroke()
                                }
                            } else {
                                g.strokeStyle = dm.colInk
                                g.globalAlpha = 0.08
                                g.lineWidth = 5
                                g.stroke()
                            }
                            g.globalAlpha = 1
                        }
                        if (peakF > 0.012) {
                            var ip = Math.min(n - 1, Math.floor(peakF * n))
                            var ap = A0 + step * (ip + 0.5)
                            var cp = Math.cos(ap), sp = Math.sin(ap)
                            g.beginPath()
                            g.moveTo(cx + R * cp, cy + R * sp)
                            g.lineTo(cx + (R + len) * cp, cy + (R + len) * sp)
                            g.strokeStyle = colFn(Math.min(1, peakF))
                            g.lineWidth = 5
                            g.globalAlpha = Math.max(dim, 0.95)
                            g.stroke()
                            g.globalAlpha = 0.4
                            g.lineWidth = 11
                            g.stroke()
                            g.globalAlpha = 1
                        }
                    }

                    function pwCol(f) { return f < 0.7 ? GREEN : (f < 0.9 ? AMBER : RED) }
                    function swCol(f) { return f < 0.2 ? GREEN : (f < 0.5 ? AMBER : RED) }
                    function cl(v) { return Math.max(0, Math.min(1, v)) }

                    var sFnow = Math.max(0, (dm.vSwr - 1) / (dm.vSwr + 1))
                    arc(580, 20, 64, cl(dm.vFwd / fs), cl(dm.pkFwdV / fs), pwCol)
                    arc(520, 15, 46, cl(dm.vRef / (fs * 0.2)), cl(dm.pkRefV / (fs * 0.2)), pwCol)
                    arc(460, 15, 34,
                        dm.swrValid ? cl(sFnow) : 0,
                        dm.swrValid ? cl(dm.pkSwrV) : 0, swCol)
                    if (dm.alcValid) {
                        arc(430, 10, 24, cl(dm.rawAlc / 100), 0,
                            function (f) { return f < 0.6 ? GREEN : (f < 0.85 ? AMBER : RED) })
                    }
                }
            }

            Rectangle {
                x: 660; y: 20; width: 1; height: 380
                gradient: Gradient {
                    GradientStop { position: 0.0;  color: "transparent" }
                    GradientStop { position: 0.2;  color: "#262D34" }
                    GradientStop { position: 0.8;  color: "#262D34" }
                    GradientStop { position: 1.0;  color: "transparent" }
                }
            }

            // ------------------------------------------------------- portate
            Row {
                x: 170; y: 250; width: 430; height: 30
                spacing: 8
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("RANGE")
                    font.pixelSize: 9; font.bold: true; font.letterSpacing: 2
                    color: dm.colMuted
                    rightPadding: 4
                }
                Repeater {
                    model: 4
                    delegate: Rectangle {
                        id: rangeChip
                        required property int index
                        readonly property bool on: index === dm.rangeIdx
                        width: 62; height: 30; radius: 5
                        color: on ? Qt.rgba(0.153, 0.769, 0.831, 0.14) : dm.colPanel
                        border.width: 1
                        border.color: on ? dm.colCyan : dm.colEdge
                        Column {
                            anchors.centerIn: parent
                            spacing: -1
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: dm.fsMult[rangeChip.index]
                                font.pixelSize: 11; font.bold: true; font.family: "monospace"
                                color: rangeChip.on ? dm.colCyan : dm.colLabel
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: dm.fsWatt[rangeChip.index]
                                font.pixelSize: 8; font.family: "monospace"
                                color: dm.colMuted
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { dm.autoRange = false; dm.rangeIdx = rangeChip.index }
                        }
                    }
                }
            }

            // ------------------------------------------------------- display
            Rectangle {
                x: 170; y: 296; width: 430; height: 106
                radius: 6
                color: "#000000"
                border.width: 1
                border.color: dm.swrValid && dm.vSwr >= 3 ? dm.colRed : "#1E252C"

                Column {
                    anchors.fill: parent
                    anchors.topMargin: 7
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    anchors.bottomMargin: 8
                    spacing: 5
                    opacity: (dm.txOn || !dm.catUp || !dm.telemetry) ? 1 : 0.45

                    Item {
                        width: parent.width; height: 11
                        Text {
                            anchors.left: parent.left
                            text: dm.statusLine
                            font.pixelSize: 9; font.letterSpacing: 1; font.family: "monospace"
                            color: dm.statusColor
                        }
                        Text {
                            anchors.right: parent.right
                            text: dm.txOn ? (dm.screenIdx === 2 ? "TX-AVG" : "TX-PK") : "RX"
                            font.pixelSize: 9; font.letterSpacing: 1; font.family: "monospace"
                            color: dm.txOn ? dm.colCyan : dm.colDim
                        }
                    }

                    Row {
                        width: parent.width
                        height: 72
                        spacing: 18

                        Column {
                            width: parent.width - 148
                            spacing: 2

                            // schermata 1 — potenza
                            Readout {
                                visible: dm.screenIdx === 0
                                tag: "FWD"; tint: dm.colCyan
                                value: dm.pwrValid ? dm.fmtW(dm.pkFwdV) : "——"
                                unit: "W"
                            }
                            Readout {
                                visible: dm.screenIdx === 0
                                tag: "REF"
                                value: dm.pwrValid && dm.swrValid ? dm.pkRefV.toFixed(3) : "——"
                                unit: "W"
                            }
                            Readout {
                                visible: dm.screenIdx === 0
                                tag: "SWR"; tint: dm.colAmber; valueSize: 22
                                value: dm.swrValid ? dm.vSwr.toFixed(2) : "——"
                            }

                            // schermata 2 — adattamento
                            Readout {
                                visible: dm.screenIdx === 1
                                tag: "RL"; tint: dm.colCyan
                                value: dm.swrValid ? (dm.returnLossDb >= 99 ? "> 60" : dm.returnLossDb.toFixed(1)) : "——"
                                unit: "dB"
                            }
                            Readout {
                                visible: dm.screenIdx === 1
                                tag: "ML"
                                value: dm.swrValid ? dm.mismatchLossDb.toFixed(2) : "——"
                                unit: "dB"
                            }
                            Readout {
                                visible: dm.screenIdx === 1
                                tag: "NET"; tint: dm.colGreen; valueSize: 22
                                value: dm.pwrValid ? dm.fmtW(dm.netW) : "——"
                                unit: "W"
                            }

                            // schermata 3 — pilotaggio
                            Readout {
                                visible: dm.screenIdx === 2
                                tag: "ALC"; tint: dm.colAmber
                                value: dm.alcValid ? Math.round(dm.rawAlc) + "" : "——"
                                unit: "%"
                            }
                            Readout {
                                visible: dm.screenIdx === 2
                                tag: "PEP"; tint: dm.colCyan
                                value: dm.pwrValid ? dm.fmtW(dm.pepW) : "——"
                                unit: "W"
                            }
                            Readout {
                                visible: dm.screenIdx === 2
                                tag: "AVG"; valueSize: 22
                                value: dm.pwrValid ? dm.fmtW(dm.avgW) : "——"
                                unit: "W"
                            }
                        }

                        // Impedenza: si dichiara cosa e' noto e cosa no. Dal solo
                        // ROS la parte reattiva non si ricava, e fingere di
                        // saperla sarebbe la bugia piu' facile da raccontare.
                        Item {
                            width: 130; height: parent.height
                            Rectangle { width: 1; height: parent.height; color: "#1A2228" }
                            Column {
                                x: 14
                                spacing: 3
                                Text {
                                    text: dm.screenIdx === 2 ? qsTr("TX TIME") : qsTr("IMPEDANCE")
                                    font.pixelSize: 9; font.letterSpacing: 1; font.family: "monospace"
                                    color: dm.colDim
                                    bottomPadding: 3
                                }
                                Text {
                                    visible: dm.screenIdx !== 2
                                    text: dm.swrValid ? "|Γ| " + dm.rho.toFixed(3) : "|Γ| —"
                                    font.pixelSize: 13; font.family: "monospace"
                                    color: "#9FB3BC"
                                }
                                Text {
                                    visible: dm.screenIdx !== 2
                                    text: dm.swrValid ? "R " + dm.rMin.toFixed(1) + "–" + dm.rMax.toFixed(1) : "R —"
                                    font.pixelSize: 13; font.family: "monospace"
                                    color: "#9FB3BC"
                                }
                                Text {
                                    visible: dm.screenIdx !== 2
                                    text: qsTr("X: needs vector sensor")
                                    width: 116
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 8
                                    color: dm.colDim
                                }
                                Text {
                                    visible: dm.screenIdx === 2
                                    text: {
                                        var s = Math.floor(dm.txSeconds)
                                        return Math.floor(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + (s % 60)
                                    }
                                    font.pixelSize: 17; font.bold: true; font.family: "monospace"
                                    color: "#9FB3BC"
                                }
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------- spie
            Column {
                x: 684; y: 34
                spacing: 15
                Repeater {
                    model: [
                        { key: "swr",  label: qsTr("SWR ALARM") },
                        { key: "alc",  label: qsTr("ALC CLIP") },
                        { key: "pwr",  label: qsTr("PWR SENSE") },
                        { key: "cat",  label: qsTr("CAT LINK") }
                    ]
                    delegate: Row {
                        id: ledRow
                        required property var modelData
                        spacing: 11
                        readonly property bool lit: {
                            switch (ledRow.modelData.key) {
                            case "swr": return dm.swrValid && dm.vSwr >= 3
                                               && (Math.floor(dm.clock * 2.4) % 2 === 0)
                            case "alc": return dm.alcValid && dm.rawAlc >= 85
                            case "pwr": return dm.pwrValid
                            case "cat": return dm.catUp
                            }
                            return false
                        }
                        readonly property color hue: (ledRow.modelData.key === "swr" || ledRow.modelData.key === "alc")
                                                     ? dm.colRed : dm.colGreen
                        Rectangle {
                            width: 10; height: 10; radius: 5
                            anchors.verticalCenter: parent.verticalCenter
                            color: ledRow.lit ? ledRow.hue : "#20262C"
                            border.width: 1
                            border.color: Qt.rgba(0, 0, 0, 0.6)
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: ledRow.modelData.label
                            font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.8
                            color: dm.colLabel
                        }
                    }
                }
            }

            // ------------------------------------------------ schermate e auto
            Column {
                x: 684; y: 196; width: 190
                spacing: 10
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "◀   " + qsTr("SCREEN") + "   ▶"
                    font.pixelSize: 9; font.bold: true; font.letterSpacing: 2
                    color: dm.colMuted
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10
                    Repeater {
                        model: ["◀", "▶"]
                        delegate: Rectangle {
                            id: navChip
                            required property int index
                            required property string modelData
                            width: 74; height: 32; radius: 5
                            border.width: 1
                            border.color: "#2A3138"
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#1C2127" }
                                GradientStop { position: 1.0; color: "#14181D" }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: navChip.modelData
                                font.pixelSize: 11; font.bold: true
                                color: dm.colLabel
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var n = dm.screenCount
                                    dm.screenIdx = (dm.screenIdx + (navChip.index === 0 ? n - 1 : 1)) % n
                                }
                            }
                        }
                    }
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 158; height: 34; radius: 5
                    color: dm.autoRange ? Qt.rgba(0.153, 0.769, 0.831, 0.14) : "#181D22"
                    border.width: 1
                    border.color: dm.autoRange ? dm.colCyan : "#2A3138"
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("AUTO")
                        font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                        color: dm.autoRange ? dm.colCyan : dm.colLabel
                    }
                    MouseArea { anchors.fill: parent; onClicked: dm.autoRange = !dm.autoRange }
                }

                // Le altre due finestre dell'app del telefono. Stesso taglio
                // dei comandi accanto, con un puntino che dice se quella
                // sorgente sta ricevendo: chi guarda il quadrante sa gia' da
                // qui se c'e' qualcosa da vedere di la'.
                Repeater {
                    model: dm.finestreDisponibili
                           ? [{ testo: qsTr("DECODE"),  vivo: dm.decodeVivo,  quale: 0 },
                              { testo: qsTr("CLUSTER"), vivo: dm.clusterVivo, quale: 1 }]
                           : []
                    delegate: Rectangle {
                        id: tastoFinestra
                        required property var modelData
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 158; height: 30; radius: 5
                        color: "#181D22"
                        border.width: 1
                        border.color: "#2A3138"

                        Row {
                            anchors.centerIn: parent
                            spacing: 7
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: tastoFinestra.modelData.testo
                                font.pixelSize: 10; font.bold: true; font.letterSpacing: 2
                                color: dm.colLabel
                            }
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 6; height: 6; radius: 3
                                color: tastoFinestra.modelData.vivo ? dm.colGreen : "#2A3138"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (tastoFinestra.modelData.quale === 0) dm.apriDecode()
                                else dm.apriCluster()
                            }
                        }
                    }
                }
            }

            // ------------------------------------------------------- marchio
            Column {
                x: 22
                y: dm.faceHeight - 58
                width: 136
                spacing: 3
                clip: true
                Text {
                    width: parent.width
                    textFormat: Text.StyledText
                    text: "DEC<font color=\"#27C4D4\">Ø</font>METER"
                    font.pixelSize: 12; font.bold: true; font.letterSpacing: 1.5
                    color: dm.colLabel
                }
                Text {
                    width: parent.width
                    text: qsTr("RF VECTOR METER") + "\n1.8–500 MHz"
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    font.pixelSize: 8; font.letterSpacing: 1.0
                    color: dm.colMuted
                }
            }

            Text {
                x: dm.faceWidth - 92
                y: dm.faceHeight - 24
                text: "DECODIUM"
                font.pixelSize: 8; font.bold: true; font.letterSpacing: 2.5
                color: dm.colDim
            }

            // Ingrandimento / ritorno: al posto della chiusura del desktop, che
            // qui non ha senso perche' non c'e' una finestra da chiudere.
            //
            // Il tasto resta, ma NON e' la via principale: rimpicciolito nella
            // colonna del waterfall il frontalino sta a un quarto di scala, e un
            // quadratino di ventiquattro punti diventa sei — meno di un
            // millimetro, impossibile da centrare. La via vera e' il doppio
            // tocco, qui sotto.
            Rectangle {
                x: dm.faceWidth - 40
                y: 8
                width: 30; height: 30; radius: 5
                visible: dm.ingranditoDisponibile || dm.ingrandito
                color: "transparent"
                border.width: 1
                border.color: "#2A3138"
                Text {
                    anchors.centerIn: parent
                    text: dm.ingrandito ? "⤡" : "⛶"
                    font.pixelSize: 14; font.bold: true
                    color: dm.colLabel
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: dm.ingrandito ? dm.chiudiIntero() : dm.apriIntero()
                }
            }
        }
    }
}
