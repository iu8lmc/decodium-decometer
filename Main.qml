// Decometer standalone. Tre finestre sulla stazione che sta al PC, e una
// schermata di rete per collegarle:
//
//   MISURE   il frontalino RF (Decometer.qml), lo STESSO file dell'app
//            completa di Decodium 4 mobile, non toccato;
//   DECODE   il traffico UDP delle decodifiche, per modo;
//   CLUSTER  gli spot del cluster DX che Decodium sta ricevendo.
//
// Le tre sorgenti sono indipendenti: una puo' funzionare mentre le altre
// tacciono, e ognuna dice da se' come sta. E' voluto — chi apre l'app per
// guardare la potenza mentre trasmette non deve vedersi bloccare il quadrante
// perche' il cluster non risponde.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: win
    visible: true
    // Misura di un telefono, per la prova sul PC.
    //
    // SUL TELEFONO NON SI TOCCANO, e non e' pignoleria: Qt 6 su Android sa
    // fare finestre che non occupano tutto lo schermo, quindi una dimensione
    // che ci sta dentro viene rispettata davvero. Chiedendo 480x900 non
    // succedeva niente — piu' grandi dello schermo, il sistema le riportava a
    // schermo intero — ma un limite calcolato sullo schermo disponibile
    // (320x742 su un telefono da 360x802 punti) ci sta comodamente, e l'app
    // finiva disegnata in un angolo con due bande vuote. Su iOS lo stesso
    // codice si vedeva perfetto, perche' li' le dimensioni richieste vengono
    // ignorate: e' cosi' che un difetto puo' presentarsi su un telefono e non
    // sull'altro.
    //
    // Il Binding con "when" lascia la proprieta' INTATTA dove non serve,
    // invece di assegnarle un altro valore: sul telefono la dimensione resta
    // quella che decide il sistema, che e' l'unica che sa qual e'.
    Binding on width {
        when: !win.suTelefono
        value: Math.min(480, Screen.desktopAvailableWidth - 40)
    }
    Binding on height {
        when: !win.suTelefono
        value: Math.min(900, Screen.desktopAvailableHeight - 60)
    }
    title: qsTr("Decometer")
    color: "#0B0E12"

    readonly property bool suTelefono: Qt.platform.os === "android"
                                       || Qt.platform.os === "ios"

    readonly property color colInk:   "#E8ECEF"
    readonly property color colLabel: "#8A939C"
    readonly property color colMuted: "#5B6670"
    readonly property color colEdge:  "#262D34"
    readonly property color colPanel: "#14181D"
    readonly property color colCyan:  "#27C4D4"
    readonly property color colGreen: "#46D67C"
    readonly property color colAmber: "#FFB454"

    // true finche' non si e' mai tentato un collegamento: mostra la schermata
    // di rete invece delle misure. Una volta collegati, un calo della linea lo
    // dice gia' da se' ogni schermata.
    property bool showSettings: bridge.lastHost.length === 0

    // Quale delle tre schermate si guarda: 0 misure, 1 decodifiche, 2 spot.
    // Non c'e' piu' una barra a dirlo — dal quadrante si entra con i due tasti
    // sotto AUTO e si torna col tasto in cima alle altre due. Una barra fissa
    // costava altezza al frontalino, che e' disegnato su tela fissa e quindi
    // si rimpicciolisce tutto insieme: pagare un misuratore piu' piccolo per
    // tre tasti sempre in vista non conviene, su uno strumento che si guarda
    // mentre si trasmette.
    property int schermo: 0

    // ---------------------------------------------------------- impostazioni
    Item {
        id: settingsScreen
        anchors.fill: parent
        visible: win.showSettings
        z: 10

        Flickable {
            anchors.fill: parent
            anchors.margins: 20
            contentHeight: colonna.implicitHeight
            clip: true

            ColumnLayout {
                id: colonna
                width: parent.width
                spacing: 14

                Label {
                    text: qsTr("Decometer")
                    color: colInk
                    font.pixelSize: 28
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("Misure, decodifiche e spot dalla stazione di Decodium 4, in rete locale.")
                    color: colLabel
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                // -------------------------------------------------- misure RF
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 6 }
                Label { text: qsTr("MISURE — server CAT condiviso"); color: colCyan; font.pixelSize: 12; font.bold: true }

                Label { text: qsTr("Indirizzo IP del PC"); color: colLabel; font.pixelSize: 13 }
                TextField {
                    id: hostField
                    Layout.fillWidth: true
                    placeholderText: qsTr("es. 192.168.1.50")
                    text: bridge.lastHost
                    color: colInk
                    font.pixelSize: 18
                    inputMethodHints: Qt.ImhPreferLatin
                }

                Label { text: qsTr("Porta (CAT condivisa di Decodium 4)"); color: colLabel; font.pixelSize: 13 }
                TextField {
                    id: portField
                    Layout.fillWidth: true
                    text: bridge.lastPort > 0 ? String(bridge.lastPort) : "4533"
                    color: colInk
                    font.pixelSize: 18
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 65535 }
                }

                Label {
                    Layout.fillWidth: true
                    text: bridge.catStatus
                    color: bridge.catConnected ? colGreen : colAmber
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                // ------------------------------------------------ decodifiche
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 6 }
                Label { text: qsTr("DECODE — traffico UDP"); color: colCyan; font.pixelSize: 12; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Decodium manda gia' le decodifiche in UDP. Sul PC: Impostazioni → Reporting → UDP Server, con l'indirizzo di questo telefono e la porta qui sotto.")
                    color: colMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Label { text: qsTr("Porta in ascolto"); color: colLabel; font.pixelSize: 13 }
                TextField {
                    id: udpPortField
                    Layout.fillWidth: true
                    text: String(decodeFeed.port)
                    color: colInk
                    font.pixelSize: 18
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 65535 }
                }

                Label { text: qsTr("Gruppo multicast (solo se il PC manda in multicast)"); color: colLabel; font.pixelSize: 13 }
                TextField {
                    id: udpGroupField
                    Layout.fillWidth: true
                    placeholderText: qsTr("vuoto = normale")
                    text: decodeFeed.group
                    color: colInk
                    font.pixelSize: 18
                    inputMethodHints: Qt.ImhPreferLatin
                }

                Label {
                    Layout.fillWidth: true
                    text: decodeFeed.status
                    color: decodeFeed.listening ? colGreen : colAmber
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                // ----------------------------------------------------- cluster
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 6 }
                Label { text: qsTr("CLUSTER — spot condivisi"); color: colCyan; font.pixelSize: 12; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Gli spot li rivende Decodium: sul PC va accesa la condivisione degli spot. L'indirizzo e' lo stesso delle misure, la porta no.")
                    color: colMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Label { text: qsTr("Porta degli spot"); color: colLabel; font.pixelSize: 13 }
                TextField {
                    id: spotPortField
                    Layout.fillWidth: true
                    text: spotFeed.port > 0 ? String(spotFeed.port) : "4534"
                    color: colInk
                    font.pixelSize: 18
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 65535 }
                }

                Label {
                    Layout.fillWidth: true
                    text: spotFeed.status
                    color: spotFeed.connected ? colGreen : colAmber
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                // ------------------------------------------- allarme di ROS
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 6 }
                Label { text: qsTr("ALLARME ROS"); color: colCyan; font.pixelSize: 12; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Il telefono vibra quando il ROS supera la soglia mentre trasmetti. Serve proprio quando non stai guardando lo schermo.")
                    color: colMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: qsTr("Soglia"); color: colLabel; font.pixelSize: 13 }
                    Repeater {
                        model: [2.0, 2.5, 3.0, 5.0]
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool scelta: Math.abs(bridge.swrAlarmSoglia - modelData) < 0.01
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 34
                            radius: 5
                            color: scelta ? colCyan : colPanel
                            border.color: colEdge
                            border.width: 1
                            Label {
                                anchors.centerIn: parent
                                text: modelData.toFixed(1)
                                color: parent.scelta ? "#0B0E12" : colInk
                                font.pixelSize: 14
                                font.bold: parent.scelta
                            }
                            TapHandler { onTapped: bridge.swrAlarmSoglia = modelData }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
                Switch {
                    Layout.fillWidth: true
                    text: qsTr("Vibrazione all'allarme")
                    checked: bridge.swrAlarmVibra
                    onToggled: bridge.swrAlarmVibra = checked
                }

                // ---------------------------------------------------- schermo
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 6 }
                Switch {
                    id: schermoAcceso
                    Layout.fillWidth: true
                    text: qsTr("Schermo sempre acceso")
                    checked: bridge.keepScreenOn
                    onToggled: bridge.keepScreenOn = checked
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Un misuratore che si spegne da solo mentre si trasmette non e' un misuratore. Vale solo con l'app in primo piano.")
                    color: colMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                // ---------------------------------------------------- comandi
                Button {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    text: qsTr("Collega tutto")
                    enabled: hostField.text.trim().length > 0
                    onClicked: {
                        var ip = hostField.text.trim()
                        bridge.catConnect(ip, parseInt(portField.text, 10) || 4533)
                        decodeFeed.listen(parseInt(udpPortField.text, 10) || 2237,
                                          udpGroupField.text.trim())
                        spotFeed.connectTo(ip, parseInt(spotPortField.text, 10) || 4534)
                        win.showSettings = false
                    }
                }
                Button {
                    Layout.fillWidth: true
                    visible: bridge.lastHost.length > 0
                    text: qsTr("Torna alle misure")
                    onClicked: win.showSettings = false
                }

                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    text: qsTr("Serve la stessa rete WiFi del PC, non internet. Le tre sorgenti sono indipendenti: se una non risponde, le altre continuano.")
                    color: colMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                // ---------------------------------------------------- contatti
                // In fondo alle impostazioni, dove si cerca chi ha fatto una
                // cosa quando la si vuole segnalare: l'indirizzo e' un
                // collegamento vero, cosi' dal telefono si scrive senza
                // ricopiarlo a mano.
                Rectangle { Layout.preferredHeight: 1; color: colEdge; Layout.fillWidth: true; Layout.topMargin: 10 }
                Label { text: qsTr("CONTATTI"); color: colCyan; font.pixelSize: 12; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Decometer 1.0 — Martino, IU8LMC")
                    color: colInk
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 20
                    text: "<a href=\"mailto:iu8lmc@gmail.com\">iu8lmc@gmail.com</a>"
                    textFormat: Text.RichText
                    linkColor: colCyan
                    font.pixelSize: 13
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }
        }
    }

    // -------------------------------------------------------------- schermate
    ColumnLayout {
        anchors.fill: parent
        visible: !win.showSettings
        spacing: 0
        z: 5

        StackLayout {
            id: pile
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: win.schermo

            // --- misure --------------------------------------------------
            Item {
                Decometer {
                    id: decometer
                    anchors.fill: parent
                    anchors.margins: 6
                    ingrandito: true
                    ingranditoDisponibile: false
                    onChiudiIntero: win.showSettings = true
                    // I due tasti dentro il quadrante, sotto AUTO: qui e'
                    // l'unica applicazione, quindi di la' si passa da qui.
                    finestreDisponibili: true
                    decodeVivo: decodeFeed.listening
                    clusterVivo: spotFeed.connected
                    onApriDecode: win.schermo = 1
                    onApriCluster: win.schermo = 2
                }

                // Via d'uscita quando il collegamento non c'e'. Compare SOLO
                // quando serve davvero e sparisce appena la radio risponde,
                // per non rubare spazio allo strumento mentre si trasmette.
                Button {
                    visible: !bridge.catConnected
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20
                    text: qsTr("Impostazioni di rete")
                    onClicked: win.showSettings = true
                }
            }

            // --- decodifiche ---------------------------------------------
            DecodeScreen { onTornaAlleMisure: win.schermo = 0 }

            // --- cluster --------------------------------------------------
            ClusterScreen { onTornaAlleMisure: win.schermo = 0 }
        }

    }

    // Il tasto Indietro di Android: dalle schermate secondarie riporta alle
    // misure, dalle misure esce. Senza questo uscirebbe sempre, e chi guarda
    // il cluster si troverebbe fuori dall'app per un gesto abituale.
    Shortcut {
        sequences: [StandardKey.Back, StandardKey.Cancel]
        onActivated: {
            if (win.showSettings && bridge.lastHost.length > 0)
                win.showSettings = false
            else if (win.schermo !== 0)
                win.schermo = 0
            else
                Qt.quit()
        }
    }
}
