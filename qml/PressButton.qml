// ABB-style pressable button. Subtle gradient (lighter top, darker bottom)
// inverts when pressed to suggest depression.

import QtQuick 2.12

Rectangle {
    id: pb
    property string label: ""
    property color  buttonColor: "#2563eb"
    signal clicked()

    radius: 6
    gradient: Gradient {
        orientation: Gradient.Vertical
        GradientStop { position: 0.0; color: ma.pressed ? Qt.darker(pb.buttonColor, 1.25) : Qt.lighter(pb.buttonColor, 1.1) }
        GradientStop { position: 1.0; color: ma.pressed ? Qt.darker(pb.buttonColor, 1.45) : pb.buttonColor }
    }
    border.color: Qt.darker(pb.buttonColor, 1.5); border.width: 1

    Text {
        anchors.centerIn: parent
        text: pb.label
        font.pixelSize: 13; font.bold: true; color: "white"
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        onClicked: pb.clicked()
    }
}
