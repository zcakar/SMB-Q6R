// Phase 1 / Iteration A — LED tile grid wired to HwIo via DiagnosticsModel.
// Each tile drives one /dev/leds port; the actual physical LED that lights
// up is recorded into .ai/ENGINEERING_LOG.md after empirical testing.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#0c1322"

    // ----- header -----
    Text {
        id: title
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        text: "SMB-Q6R Diagnostics"
        font.pixelSize: 36
        font.bold: true
        color: "#7CFC00"
    }

    Text {
        id: subtitle
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: title.bottom
        anchors.topMargin: 8
        text: "Phase 1 / Iteration A — LED test  |  hwio: " +
              (model.ledReady ? "ready" : "FAIL")
        font.pixelSize: 16
        color: model.ledReady ? "#C0C0C0" : "#FF5555"
    }

    // ----- LED tile row -----
    Row {
        id: ledRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: subtitle.bottom
        anchors.topMargin: 80
        spacing: 24

        Repeater {
            model: 5  // ports 0..4

            Rectangle {
                width: 180
                height: 240
                radius: 12
                border.width: 2
                border.color: on ? "#7CFC00" : "#445566"
                color: on ? "#2a5a2a" : "#1a2233"

                // Each tile remembers its own state; we cross-check against
                // the model's mask whenever it changes (see Connections).
                property int  port: index
                property bool on:   ((root.ledMaskShadow & (1 << port)) !== 0)

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 18
                    text: "Port " + port
                    font.pixelSize: 22
                    font.bold: true
                    color: "#ffffff"
                }

                // LED bulb glyph
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: 8
                    width: 80
                    height: 80
                    radius: width / 2
                    color: on ? "#7CFC00" : "#28324a"
                    border.color: on ? "#A4FF40" : "#445566"
                    border.width: 3
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 18
                    text: on ? "ON" : "OFF"
                    font.pixelSize: 18
                    color: on ? "#7CFC00" : "#6A7A93"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: model.setLed(port, !parent.on)
                }
            }
        }
    }

    // ----- ALL OFF -----
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: ledRow.bottom
        anchors.topMargin: 36
        width: 240
        height: 56
        radius: 8
        color: allOffMa.pressed ? "#A52525" : "#C03030"

        Text {
            anchors.centerIn: parent
            text: "ALL OFF"
            font.pixelSize: 20
            font.bold: true
            color: "white"
        }

        MouseArea {
            id: allOffMa
            anchors.fill: parent
            onClicked: model.allLedsOff()
        }
    }

    // ----- footer -----
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Mask: 0b" + root.ledMaskShadow.toString(2).padStart(5, "0") +
              "   (physical port↔LED mapping pending physical test)"
        font.pixelSize: 14
        color: "#6A7A93"
        font.family: "monospace"
    }

    // ----- bridge to C++ model -----
    // Hold the active mask in a JS-readable property so tile bindings can
    // react to it. Updated from the model's ledChanged signal.
    property int ledMaskShadow: 0

    Connections {
        target: model
        onLedChanged: root.ledMaskShadow = model.ledMask
    }

    Component.onCompleted: root.ledMaskShadow = model.ledMask
}
