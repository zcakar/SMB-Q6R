// One stage of the deadman switch indicator (RELEASED / ACTIVE / PANIC).
// Light theme; active stage fills with stage colour.

import QtQuick 2.12

Item {
    id: stage
    property string label: ""
    property color  stageColor: "#16a34a"
    property bool   active: false

    width: 230; height: 44

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: stage.active ? stage.stageColor : "#f9fafb"
        border.color: stage.active ? Qt.darker(stage.stageColor, 1.3) : "#d1d5db"
        border.width: 1
    }
    Rectangle {
        x: 14
        anchors.verticalCenter: parent.verticalCenter
        width: 16; height: 16; radius: 8
        color: stage.active ? "white" : "#e5e7eb"
        border.color: stage.active ? Qt.lighter(stage.stageColor, 1.4) : "#b0b4ba"
        border.width: 2
    }
    Text {
        x: 44
        anchors.verticalCenter: parent.verticalCenter
        text: stage.label
        font.pixelSize: 17; font.bold: true; font.letterSpacing: 2.0
        color: stage.active ? "white" : "#6b7280"
    }
}
