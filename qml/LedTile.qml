// Indicator-LED tile (STOP / SERVO / ENABLE / spare). Light theme; the LED
// "bulb" inside is a small circle that glows in the indicator colour when on.

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
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 10
        text: tile.label
        font.pixelSize: 14; font.bold: true; font.letterSpacing: 1.0
        color: tile.on ? tile.ledColor : "#374151"
    }
    // LED bulb
    Rectangle {
        id: bulb
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter:   parent.verticalCenter
        anchors.verticalCenterOffset: 4
        width: 46; height: 46; radius: 23
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: tile.on ? Qt.lighter(tile.ledColor, 1.35) : "#e5e7eb" }
            GradientStop { position: 1.0; color: tile.on ? Qt.darker(tile.ledColor, 1.25)  : "#c8ccd2" }
        }
        border.color: tile.on ? Qt.darker(tile.ledColor, 1.5) : "#a1a4ac"
        border.width: 2

        // White shine highlight on top of the bulb
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top; anchors.topMargin: 6
            width: 18; height: 8; radius: 4
            color: "white"
            opacity: tile.on ? 0.55 : 0.35
        }
    }
    Text {
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
