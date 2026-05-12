// Light-theme panel card — white body, soft border, light-gray title strip.
// ABB FlexPendant aesthetic; children declared inside become content.

import QtQuick 2.12

Item {
    id: card
    property string title: ""

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: "#ffffff"
        border.color: "#d1d5db"
        border.width: 1
    }
    // Title strip — flat, slightly darker, separator line at bottom
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 1
        height: 32
        radius: 6
        color: "#f3f4f6"
        // mask bottom-rounded corners so only top is rounded
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom; height: parent.height / 2
            color: "#f3f4f6"
        }
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom; height: 1
            color: "#e5e7eb"
        }
    }
    Text {
        text: card.title
        anchors.top: parent.top; anchors.topMargin: 10
        anchors.left: parent.left; anchors.leftMargin: 14
        font.pixelSize: 12; font.bold: true
        font.letterSpacing: 1.0
        color: "#374151"
    }
}
