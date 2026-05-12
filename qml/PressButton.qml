// Pressable button with gradient body that darkens while held.

import QtQuick 2.12

Rectangle {
    id: pb
    property string label: ""
    property color  buttonColor: "#3DA9FC"
    signal clicked()

    radius: 8
    gradient: Gradient {
        orientation: Gradient.Vertical
        GradientStop { position: 0.0; color: ma.pressed ? Qt.darker(pb.buttonColor, 1.4) : Qt.lighter(pb.buttonColor, 1.15) }
        GradientStop { position: 1.0; color: ma.pressed ? Qt.darker(pb.buttonColor, 1.7) : Qt.darker(pb.buttonColor, 1.2) }
    }
    border.color: Qt.darker(pb.buttonColor, 1.6); border.width: 1

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
