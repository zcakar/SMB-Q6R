// Mode-selector indicator (AUTO / MANUAL / STOP). Light theme. When
// active, the body fills with the mode colour and the LED dot turns white.

import QtQuick 2.12

Item {
    id: pill
    property string name: ""
    property color  activeColor: "#16a34a"
    property bool   active: false

    width: 230; height: 44

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: pill.active ? pill.activeColor : "#f9fafb"
        border.color: pill.active ? Qt.darker(pill.activeColor, 1.3) : "#d1d5db"
        border.width: 1
    }
    // LED dot
    Rectangle {
        x: 14
        anchors.verticalCenter: parent.verticalCenter
        width: 16; height: 16; radius: 8
        color: pill.active ? "white" : "#e5e7eb"
        border.color: pill.active ? Qt.lighter(pill.activeColor, 1.4) : "#b0b4ba"
        border.width: 2
    }
    // Label
    Text {
        x: 44
        anchors.verticalCenter: parent.verticalCenter
        text: pill.name
        font.pixelSize: 17; font.bold: true; font.letterSpacing: 2.0
        color: pill.active ? "white" : "#6b7280"
    }
}
