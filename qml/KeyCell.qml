// One cell of the 2×7 matrix-key mirror grid. Tap to clear (re-assign next
// press here). Highlights when the just-pressed code matches.

import QtQuick 2.12

Item {
    id: cell
    property int    cellIndex:   0
    property string sign:        "−"
    property string rowLabel:    ""
    property int    code:        -1
    property bool   justPressed: false
    property bool   selected:    false
    signal tapped()

    Rectangle {
        anchors.fill: parent
        radius: 10
        border.color: cell.selected    ? "#FFD700" :
                      cell.justPressed ? "#4dff77" :
                      cell.code >= 0   ? "#2c3a55" : "#243047"
        border.width: cell.selected ? 3 : (cell.justPressed ? 3 : 1)
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop {
                position: 0.0
                color: cell.selected    ? "#3a2f00" :
                       cell.justPressed ? "#1f5f1f" :
                       cell.code >= 0   ? "#27314a" : "#1a2336"
            }
            GradientStop {
                position: 1.0
                color: cell.selected    ? "#1a1700" :
                       cell.justPressed ? "#0c3a0c" :
                       cell.code >= 0   ? "#1a2236" : "#0e1626"
            }
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 4
        text: cell.rowLabel
        font.pixelSize: 9; color: "#7e8ba6"
    }
    Text {
        anchors.centerIn: parent
        text: cell.sign
        font.pixelSize: cell.height > 70 ? 42 : 32
        font.bold: true
        color: cell.selected    ? "#FFD700" :
               cell.justPressed ? "#4dff77" :
               cell.code >= 0   ? "#E8EEF7" : "#7e8ba6"
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 4
        text: cell.code >= 0 ? ("code " + cell.code) : "—"
        font.pixelSize: 10; font.family: "monospace"
        color: cell.justPressed ? "#4dff77" : "#7e8ba6"
    }
    MouseArea {
        anchors.fill: parent
        onClicked: cell.tapped()
    }
}
