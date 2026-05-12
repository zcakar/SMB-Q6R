// Indicator-LED tile (STOP / SERVO / ENABLE / spare). Light theme; the LED
// "bulb" inside is a small circle that glows in the indicator colour when on.
// Layout uses anchors with explicit margins so label, bulb and footer never
// overlap, regardless of the parent tile height.

import QtQuick 2.12

Item {
    id: tile

    property string label:    ""
    property string sub:      ""
    property color  ledColor: "#16a34a"
    property bool   on:       false

    signal toggleRequested()

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "#ffffff"
        border.color: tile.on ? tile.ledColor : "#d1d5db"
        border.width: tile.on ? 2 : 1
    }

    // Top: label (e.g. "STOP", "LED 1")
    Text {
        id: lbl
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 10
        text: tile.label
        font.pixelSize: 15; font.bold: true; font.letterSpacing: 1.0
        color: tile.on ? tile.ledColor : "#1f2937"
    }

    // Middle: bulb. Sized as a percentage of available height to stay
    // proportional regardless of tile size, but capped so it never collides
    // with the label or the footer.
    Rectangle {
        id: bulb
        property real maxDiameter: Math.min(40,
            parent.height - lbl.height - sub.height - 24)
        width: maxDiameter; height: maxDiameter; radius: maxDiameter / 2

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: lbl.bottom; anchors.topMargin: 6

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: tile.on ? Qt.lighter(tile.ledColor, 1.35) : "#e5e7eb" }
            GradientStop { position: 1.0; color: tile.on ? Qt.darker(tile.ledColor, 1.25)  : "#c8ccd2" }
        }
        border.color: tile.on ? Qt.darker(tile.ledColor, 1.5) : "#a1a4ac"
        border.width: 2

        Rectangle { // shine highlight
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top; anchors.topMargin: 5
            width: parent.width * 0.35; height: parent.height * 0.18
            radius: height / 2
            color: "white"
            opacity: tile.on ? 0.55 : 0.32
        }
    }

    // Bottom: sub-text + state
    Text {
        id: sub
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 8
        text: tile.sub + "  ·  " + (tile.on ? "ON" : "OFF")
        font.pixelSize: 11
        color: tile.on ? tile.ledColor : "#6b7280"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: tile.toggleRequested()
    }
}
