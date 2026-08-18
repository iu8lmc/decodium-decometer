// Decometer standalone — app minima: uno schermo di collegamento e il
// frontalino a tutto schermo. Il frontalino (Decometer.qml) e' lo STESSO file
// dell'app completa di Decodium 4 mobile, non toccato: stessa geometria,
// stessi colori, stesse formule. Qui cambia solo il contorno — non c'e' un
// pannello piu' piccolo da cui "ingrandire", quindi si parte gia' a schermo
// intero e il tasto che nell'app completa richiude il frontalino qui riporta
// alla schermata di collegamento, che e' il posto sensato dove tornare.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: win
    visible: true
    width: 480
    height: 900
    title: qsTr("Decometer")
    color: "#0B0E12"

    // true finche' non si e' mai tentato un collegamento: mostra la
    // schermata di rete invece del frontalino. Una volta connessi, un calo
    // della linea lo dice gia' da solo il frontalino ("NO CAT LINK", ambra):
    // il tasto in alto a destra (sempre visibile perche' ingrandito e'
    // sempre vero, qui) riporta a questa schermata per ricollegarsi altrove.
    property bool showSettings: bridge.lastHost.length === 0

    // ------------------------------------------------------------ impostazioni
    Item {
        id: settingsScreen
        anchors.fill: parent
        visible: win.showSettings
        z: 10

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 420)
            spacing: 18

            Label {
                text: qsTr("Decometer")
                color: "#E8ECEF"
                font.pixelSize: 28
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("Potenza, ROS e ALC dalla radio di Decodium 4, in rete locale.")
                color: "#8A939C"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle { height: 1; color: "#262D34"; Layout.fillWidth: true; Layout.topMargin: 8 }

            Label { text: qsTr("Indirizzo IP del PC"); color: "#8A939C"; font.pixelSize: 13 }
            TextField {
                id: hostField
                Layout.fillWidth: true
                placeholderText: qsTr("es. 192.168.1.50")
                text: bridge.lastHost
                color: "#E8ECEF"
                font.pixelSize: 18
                inputMethodHints: Qt.ImhPreferLatin
            }

            Label { text: qsTr("Porta (server CAT condiviso di Decodium 4)"); color: "#8A939C"; font.pixelSize: 13 }
            TextField {
                id: portField
                Layout.fillWidth: true
                text: bridge.lastPort > 0 ? String(bridge.lastPort) : "4533"
                color: "#E8ECEF"
                font.pixelSize: 18
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1; top: 65535 }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 4
                text: bridge.catStatus
                color: bridge.catConnected ? "#46D67C" : "#FFB454"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }

            Button {
                id: connecting
                Layout.fillWidth: true
                Layout.topMargin: 8
                text: qsTr("Connetti")
                enabled: hostField.text.trim().length > 0
                onClicked: {
                    bridge.catConnect(hostField.text.trim(), parseInt(portField.text, 10) || 4533)
                    win.showSettings = false
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: qsTr("Sul PC: apri Decodium 4, attiva il server CAT condiviso nelle Impostazioni e leggi qui l'IP della rete locale. Serve la stessa rete WiFi, non internet.")
                color: "#5B6670"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }

    // ---------------------------------------------------------------- misure
    Item {
        anchors.fill: parent
        visible: !win.showSettings
        z: 5

        Decometer {
            id: decometer
            anchors.fill: parent
            anchors.margins: 6
            ingrandito: true
            ingranditoDisponibile: false
            onChiudiIntero: win.showSettings = true
        }
    }
}
