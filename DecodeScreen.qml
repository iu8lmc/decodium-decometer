// Il traffico UDP delle decodifiche, come arriva da Decodium 4.
//
// Non e' una lista di QSO ne' un log: e' una finestra sul flusso, per vedere
// da lontano che cosa sta decodificando il PC e in quali modi. Per questo in
// cima ci sono i conteggi per modo — che sono anche il filtro — e per questo
// la riga di stato dice quanti pacchetti sono arrivati in tutto: quando non
// si vede niente, la prima domanda e' sempre se stia arrivando qualcosa.
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

    // Il colore del rapporto segnale/rumore dice a colpo d'occhio se il
    // segnale e' forte o al limite: gli stessi tre livelli del frontalino.
    function coloreSnr(v) {
        if (v >= 0) return colGreen
        if (v >= -15) return colCyan
        return colAmber
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // ------------------------------------------------------------ stato
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
                color: decodeFeed.listening ? colGreen : colMuted
                Layout.alignment: Qt.AlignVCenter
            }
            Label {
                Layout.fillWidth: true
                text: decodeFeed.status
                color: decodeFeed.listening ? colInk : colAmber
                font.pixelSize: 13
                elide: Text.ElideRight
            }
            Tasto {
                text: qsTr("Pulisci")
                enabled: decodeFeed.received > 0
                onClicked: decodeFeed.clear()
            }
        }

        // Che cosa sta facendo la radio dall'altra parte, quando lo dice.
        Label {
            Layout.fillWidth: true
            visible: decodeFeed.dialLabel.length > 0
            text: decodeFeed.transmitting
                  ? qsTr("%1 · in trasmissione").arg(decodeFeed.dialLabel)
                  : decodeFeed.dialLabel
            color: decodeFeed.transmitting ? colAmber : colLabel
            font.pixelSize: 13
            font.bold: decodeFeed.transmitting
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("%1 decodifiche · %2 pacchetti · ultimo: %3")
                    .arg(decodeFeed.received)
                    .arg(decodeFeed.packets)
                    .arg(decodeFeed.lastPacket.length ? decodeFeed.lastPacket : qsTr("nessuno"))
            color: colMuted
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        // ------------------------------------------------------- modi e filtro
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            contentWidth: fileModi.width
            flickableDirection: Flickable.HorizontalFlick
            clip: true

            Row {
                id: fileModi
                spacing: 6
                height: parent.height

                // "Tutti" e' il primo e resta sempre: e' il modo di tornare
                // indietro dopo aver ristretto la vista.
                Rectangle {
                    height: 30
                    width: etichettaTutti.width + 22
                    radius: 15
                    color: decodeFeed.modeFilter.length === 0 ? colCyan : colPanel
                    border.color: colEdge
                    border.width: 1
                    Label {
                        id: etichettaTutti
                        anchors.centerIn: parent
                        text: qsTr("tutti %1").arg(decodeFeed.received)
                        color: decodeFeed.modeFilter.length === 0 ? "#0B0E12" : colLabel
                        font.pixelSize: 12
                        font.bold: decodeFeed.modeFilter.length === 0
                    }
                    TapHandler { onTapped: decodeFeed.modeFilter = "" }
                }

                Repeater {
                    model: decodeFeed.modeStats
                    delegate: Rectangle {
                        required property var modelData
                        readonly property bool scelto: decodeFeed.modeFilter === modelData.mode
                        height: 30
                        width: etichettaModo.width + 22
                        radius: 15
                        color: scelto ? colCyan : colPanel
                        border.color: colEdge
                        border.width: 1
                        Label {
                            id: etichettaModo
                            anchors.centerIn: parent
                            text: modelData.mode + " " + modelData.count
                            color: scelto ? "#0B0E12" : colInk
                            font.pixelSize: 12
                            font.bold: scelto
                        }
                        // Ritoccare lo stesso modo lo toglie: senza questo,
                        // per tornare a vedere tutto bisognerebbe cercare il
                        // tasto "tutti" in fondo a una fila lunga.
                        TapHandler {
                            onTapped: decodeFeed.modeFilter = parent.scelto ? "" : modelData.mode
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: colEdge }

        // --------------------------------------------------------- decodifiche
        ListView {
            id: lista
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: decodeFeed
            spacing: 1
            // Le nuove entrano in cima: la lista resta ferma dov'e' se
            // l'utente ha scorso, e torna in cima appena e' gia' in cima.
            // Cosi' leggere una riga vecchia non viene interrotto dal flusso.
            verticalLayoutDirection: ListView.TopToBottom
            boundsBehavior: Flickable.StopAtBounds

            // Le nuove righe entrano in cima, e una ListView che riceve righe
            // SOPRA la parte visibile tiene fermo quello che si sta guardando:
            // alza contentY dell'altezza di cio' che ha inserito. E' il
            // comportamento giusto per chi ha scorso indietro a leggere una
            // riga vecchia — non gli si sposta il testo sotto gli occhi — ma
            // per chi sta in cima a guardare il flusso e' esattamente il
            // contrario di quel che serve: le righe si accumulano appena fuori
            // dallo schermo e la lista sembra ferma. Era il caso qui, dove il
            // commento prometteva il ritorno in cima e non lo faceva nessuno.
            //
            // Lo stato si legge PRIMA dell'inserimento: dopo, atYBeginning e'
            // gia' falso proprio a causa di quello spostamento, e la
            // condizione non sarebbe mai vera.
            property bool eraInCima: true
            Connections {
                target: decodeFeed
                function onRowsAboutToBeInserted() { lista.eraInCima = lista.atYBeginning }
                function onRowsInserted() { if (lista.eraInCima) lista.positionViewAtBeginning() }
                // Il filtro rifa' il modello da capo: li' si torna in cima
                // comunque, perche' la posizione di prima non vuol piu' dire
                // niente su un elenco diverso.
                function onModelReset() { lista.positionViewAtBeginning() }
            }

            delegate: Rectangle {
                required property string ora
                required property int snr
                required property double dt
                required property int df
                required property string modo
                required property string messaggio
                required property bool bassaConfidenza

                width: lista.width
                height: 46
                color: colPanel

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Label {
                        text: ora
                        color: colMuted
                        font.pixelSize: 12
                        font.family: "monospace"
                    }
                    Label {
                        text: (snr > 0 ? "+" : "") + snr
                        color: coloreSnr(snr)
                        font.pixelSize: 13
                        font.bold: true
                        font.family: "monospace"
                        Layout.preferredWidth: 32
                        horizontalAlignment: Text.AlignRight
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            Layout.fillWidth: true
                            text: messaggio
                            // Un decode a bassa confidenza puo' essere falso:
                            // si mostra, ma non con l'aria di essere certo
                            // quanto gli altri.
                            color: bassaConfidenza ? colMuted : colInk
                            font.pixelSize: 14
                            font.family: "monospace"
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("%1 · %2 Hz · DT %3").arg(modo).arg(df).arg(dt.toFixed(1))
                            color: colMuted
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // Nessuna riga: si dice perche', invece di lasciare un buco nero
            // che sembra un guasto.
            Label {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: lista.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: colMuted
                font.pixelSize: 13
                text: !decodeFeed.listening
                        ? qsTr("Non in ascolto. Imposta la porta nelle impostazioni di rete.")
                        : decodeFeed.modeFilter.length > 0
                          ? qsTr("Nessuna decodifica in %1 finora.").arg(decodeFeed.modeFilter)
                          : qsTr("In ascolto sulla porta %1.\n\nSul PC, in Decodium 4: Impostazioni → Reporting → UDP Server, scrivi l'indirizzo di questo telefono e la stessa porta.")
                            .arg(decodeFeed.port)
            }
        }
    }
}
