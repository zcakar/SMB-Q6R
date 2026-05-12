// Panel card with gradient body, soft border and a title strip on top.
// Children declared inside become content overlaid on the body.

import QtQuick 2.12

Item {
    id: card
    property string title: ""

    Rectangle {
        anchors.fill: parent
        radius: 12
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#1f2a42" }
            GradientStop { position: 1.0; color: "#121828" }
        }
        border.color: "#2c3a55"; border.width: 1
    }
    Rectangle {  // title strip — rounded top only (mask bottom corners)
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 1
        height: 28
        radius: 12
        color: "#1c2942"
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom; height: parent.height / 2
            color: "#1c2942"
        }
    }
    Text {
        text: card.title
        anchors.top: parent.top; anchors.topMargin: 6
        anchors.left: parent.left; anchors.leftMargin: 14
        font.pixelSize: 12; font.bold: true
        color: "#9aa8bd"
        font.letterSpacing: 1.0
    }
}
