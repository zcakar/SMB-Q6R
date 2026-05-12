// Re-usable named LED indicator tile. Touch to toggle the underlying
// /dev/leds port via the DiagnosticsModel. Declared in its own file
// because Qt 5.12 lacks the Qt 6 `component` keyword for inline
// component definitions.

import QtQuick 2.12

Rectangle {
    id: tile

    // Public API
    property string label:    ""
    property string sub:      ""
    property int    ledPort:  0
    property color  ledColor: "#4dff77"
    property bool   on:       false

    signal toggleRequested()

    width: 130; height: 150; radius: 12
    color: tile.on ? Qt.darker(tile.ledColor, 2.0) : "#172033"
    border.color: tile.on ? tile.ledColor : "#2c3a55"
    border.width: 2

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 8
        text: tile.label
        font.pixelSize: 16; font.bold: true
        color: tile.on ? tile.ledColor : "#E8EEF7"
    }
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 4
        width: 56; height: 56; radius: 28
        color: tile.on ? tile.ledColor : "#1a2236"
        border.color: tile.on ? Qt.lighter(tile.ledColor, 1.4) : "#2c3a55"
        border.width: 3
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 6
        text: tile.sub + "  " + (tile.on ? "ON" : "OFF")
        font.pixelSize: 11
        color: tile.on ? tile.ledColor : "#8a99b3"
    }
    MouseArea {
        anchors.fill: parent
        onClicked: tile.toggleRequested()
    }
}
