// Gli spot del cluster DX che sta guardando Decodium sul PC, rimandati qui.
//
// Stessa forma della schermata delle decodifiche — stato in alto, conteggi
// che fanno anche da filtro, lista sotto — perche' sono due finestre sulla
// stessa cosa: quel che il PC sta ricevendo mentre l'operatore e' altrove.
// Qui il filtro e' per banda, che e' il taglio che serve piu' spesso: si
// cerca chi chiama dove si sta trasmettendo.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: schermo

    // L'ospite decide dove si torna: la schermata non sa di far parte di una
    // pila e non deve saperlo.
    signal tornaAlleMisure()

    // I tasti di sistema arrivano col fondo chiaro del tema predefinito: sul
    // nero del quadrante il "torna indietro" diventava un disco bianco che
    // copriva la propria freccia, e "Pulisci" spariva scuro su scuro. Si
    // disegnano qui, con gli stessi colori di tutto il resto.
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
            border.color: colEdge
            border.width: 1
        }
    }

    readonly property color colInk:   "#E8ECEF"
    readonly property color colLabel: "#8A939C"
    readonly property color colMuted: "#5B6670"
    readonly property color colEdge:  "#262D34"
    readonly property color colPanel: "#14181D"
    readonly property color colCyan:  "#27C4D4"
    readonly property color colGreen: "#46D67C"
    readonly property color colAmber: "#FFB454"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Il ritorno al quadrante: senza barra, questa e' l'unica via
            // indietro che si vede: il tasto di sistema su Android c'e', ma
            // su iPhone non esiste e nessuno lo cerca a schermo.
            Tasto {
                text: "‹"
                implicitWidth: 40
                onClicked: schermo.tornaAlleMisure()
            }
            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 5
                color: spotFeed.connected ? colGreen : colMuted
                Layout.alignment: Qt.AlignVCenter
            }
            Label {
                Layout.fillWidth: true
                text: spotFeed.status
                color: spotFeed.connected ? colInk : colAmber
                font.pixelSize: 13
                elide: Text.ElideRight
            }
            Tasto {
                text: qsTr("Pulisci")
                enabled: spotFeed.count > 0
                onClicked: spotFeed.clear()
            }
        }

        // ------------------------------------------------------ bande e filtro
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            contentWidth: fileBande.width
            flickableDirection: Flickable.HorizontalFlick
            clip: true

            Row {
                id: fileBande
                spacing: 6
                height: parent.height

                Rectangle {
                    height: 30
                    width: etichettaTutte.width + 22
                    radius: 15
                    color: spotFeed.bandFilter.length === 0 ? colCyan : colPanel
                    border.color: colEdge
                    border.width: 1
                    Label {
                        id: etichettaTutte
                        anchors.centerIn: parent
                        text: qsTr("tutte")
                        color: spotFeed.bandFilter.length === 0 ? "#0B0E12" : colLabel
                        font.pixelSize: 12
                        font.bold: spotFeed.bandFilter.length === 0
                    }
                    TapHandler { onTapped: spotFeed.bandFilter = "" }
                }

                Repeater {
                    model: spotFeed.bandStats
                    delegate: Rectangle {
                        required property var modelData
                        readonly property bool scelta: spotFeed.bandFilter === modelData.band
                        height: 30
                        width: etichettaBanda.width + 22
                        radius: 15
                        color: scelta ? colCyan : colPanel
                        border.color: colEdge
                        border.width: 1
                        Label {
                            id: etichettaBanda
                            anchors.centerIn: parent
                            text: modelData.band + " " + modelData.count
                            color: scelta ? "#0B0E12" : colInk
                            font.pixelSize: 12
                            font.bold: scelta
                        }
                        TapHandler {
                            onTapped: spotFeed.bandFilter = parent.scelta ? "" : modelData.band
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: colEdge }

        // ---------------------------------------------------------------- spot
        ListView {
            id: lista
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: spotFeed
            spacing: 1
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property string call
                required property double frequenza
                required property string banda
                required property string modo
                required property string spotter
                required property string commento
                required property string ora

                width: lista.width
                height: 52
                color: colPanel

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        spacing: 0
                        Layout.preferredWidth: 96
                        Label {
                            text: call
                            color: colInk
                            font.pixelSize: 15
                            font.bold: true
                            font.family: "monospace"
                        }
                        Label {
                            text: ora
                            color: colMuted
                            font.pixelSize: 11
                            font.family: "monospace"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            Layout.fillWidth: true
                            // In kHz come li manda il cluster: e' il numero
                            // che si scrive nella radio, non una conversione
                            // da rifare a mente.
                            text: qsTr("%1 kHz · %2").arg(frequenza.toFixed(1)).arg(banda)
                            color: colCyan
                            font.pixelSize: 13
                            font.family: "monospace"
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: commento.length ? commento : modo
                            color: colLabel
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Label {
                        text: spotter
                        color: colMuted
                        font.pixelSize: 11
                        font.family: "monospace"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: lista.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: colMuted
                font.pixelSize: 13
                text: !spotFeed.connected
                        ? qsTr("Non collegato al PC. Imposta indirizzo e porta nelle impostazioni di rete.")
                        : spotFeed.bandFilter.length > 0
                          ? qsTr("Nessuno spot in %1 finora.").arg(spotFeed.bandFilter)
                          : qsTr("Collegato. Gli spot compaiono qui appena il cluster li manda a Decodium.\n\nSul PC serve il cluster collegato e la condivisione degli spot accesa.")
            }
        }
    }
}
