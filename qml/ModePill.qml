// Pill-shaped mode indicator (AUTO / MANUAL / STOP). Glows when active.

import QtQuick 2.12

Item {
    id: pill
    property string name: ""
    property color  pillColor: "#4dff77"
    property bool   active: false
    width: 240; height: 50

    Rectangle {
        anchors.fill: parent
        radius: 25
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: pill.active ? Qt.lighter(pill.pillColor, 1.05) : "#1f2a42" }
            GradientStop { position: 1.0; color: pill.active ? Qt.darker(pill.pillColor, 1.3) : "#121828" }
        }
        border.color: pill.active ? Qt.lighter(pill.pillColor, 1.4) : "#2c3a55"
        border.width: pill.active ? 2 : 1
    }
    Rectangle {  // outer glow
        visible: pill.active
        anchors.fill: parent
        anchors.margins: -3
        radius: 28
        color: "transparent"
        border.color: pill.pillColor; border.width: 2
        opacity: 0.35
    }
    Rectangle {  // inner dot
        anchors.left: parent.left; anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        width: 18; height: 18; radius: 9
        color: pill.active ? "white" : "#2c3a55"
    }
    Text {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: 12
        text: pill.name
        font.pixelSize: 22; font.bold: true; font.letterSpacing: 2.0
        color: pill.active ? "white" : "#7e8ba6"
    }
}
