// Realistic matrix-key tile. Outer light-grey "plastic" bezel surrounds a
// dark inset key face with a large +/− glyph, axis label and code readout.
// Mimics the physical key set on the right edge of the HN00-09Q6 pendant.

import QtQuick 2.12

Item {
    id: cell
    property int    cellIndex:   -1
    property string axisLabel:   "J1"
    property string sign:        "−"
    property int    code:        -1
    property bool   justPressed: false
    property bool   selected:    false
    signal tapped()

    // Outer plastic bezel (the recessed white-ish surround on the pendant)
    Rectangle {
        id: bezel
        anchors.fill: parent
        radius: 9
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: cell.selected ? "#fde68a" : "#eaecf0" }
            GradientStop { position: 1.0; color: cell.selected ? "#fbbf24" : "#c8ccd2" }
        }
        border.color: cell.selected ? "#d97706" : "#9ca0a8"
        border.width: 1
    }
    // Inner key face — dark cap with subtle gradient
    Rectangle {
        id: face
        anchors.fill: parent
        anchors.margins: 7
        radius: 6
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0
                color: cell.justPressed ? "#22c55e"
                       : cell.code >= 0 ? "#3a3f4a" : "#475264" }
            GradientStop { position: 0.45
                color: cell.justPressed ? "#16a34a"
                       : cell.code >= 0 ? "#1f242d" : "#2c3441" }
            GradientStop { position: 1.0
                color: cell.justPressed ? "#14532d"
                       : cell.code >= 0 ? "#0c0f15" : "#171f2b" }
        }
        border.color: cell.justPressed ? "#15803d" : "#0a0d13"
        border.width: 1
    }
    // Top highlight bar — adds 3D "shine"
    Rectangle {
        anchors.left:  face.left;   anchors.leftMargin: 4
        anchors.right: face.right;  anchors.rightMargin: 4
        anchors.top:   face.top;    anchors.topMargin: 4
        height: Math.max(6, face.height * 0.10)
        radius: 4
        color: "#ffffff"
        opacity: 0.10
    }
    // Axis label (top of face)
    Text {
        anchors.top: face.top; anchors.topMargin: 6
        anchors.horizontalCenter: face.horizontalCenter
        text: cell.axisLabel
        font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.5
        color: cell.justPressed ? "#bbf7d0" : "#9aa1ac"
    }
    // Large +/− glyph
    Text {
        anchors.centerIn: face
        text: cell.sign
        font.pixelSize: face.height * 0.50
        font.bold: true
        color: "white"
    }
    // Code readout (bottom of face)
    Text {
        anchors.bottom: face.bottom; anchors.bottomMargin: 6
        anchors.horizontalCenter: face.horizontalCenter
        text: cell.code >= 0 ? ("code " + cell.code) : "—"
        font.pixelSize: 10; font.family: "monospace"
        color: cell.justPressed ? "#bbf7d0" : "#9aa1ac"
    }
    MouseArea {
        anchors.fill: parent
        onClicked: cell.tapped()
    }
}
