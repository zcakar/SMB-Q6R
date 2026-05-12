// Three-stage enable-switch card. Active card glows with its stage colour.

import QtQuick 2.12

Item {
    id: stage
    property string label: ""
    property color  stageColor: "#4dff77"
    property bool   active: false
    width: 240; height: 50

    Rectangle {
        anchors.fill: parent
        radius: 8
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: stage.active ? Qt.lighter(stage.stageColor, 1.2) : "#1f2a42" }
            GradientStop { position: 1.0; color: stage.active ? Qt.darker(stage.stageColor, 1.4) : "#121828" }
        }
        border.color: stage.active ? Qt.lighter(stage.stageColor, 1.4) : "#2c3a55"
        border.width: stage.active ? 2 : 1
    }
    Text {
        anchors.centerIn: parent
        text: stage.label
        font.pixelSize: 18; font.bold: true; font.letterSpacing: 2.0
        color: stage.active ? "#001020" : "#7e8ba6"
    }
}
