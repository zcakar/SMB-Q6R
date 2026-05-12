// Phase 1 Diagnostics — single-page comprehensive hardware test.
//
// Pure QtQuick 2.12 (no Controls / Layouts / Window modules) so the binary
// runs against the HN00-09Q6 system Qt 5.12.8 install with no extra .deb's.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#0c1322"

    // ---------- Theme helpers ----------
    QtObject {
        id: theme
        readonly property color bg:       "#0c1322"
        readonly property color panel:    "#1a2233"
        readonly property color border:   "#445566"
        readonly property color accent:   "#7CFC00"
        readonly property color amber:    "#FFD700"
        readonly property color danger:   "#FF5555"
        readonly property color text:     "#E6E6E6"
        readonly property color muted:    "#9AA8BD"
    }

    // ---------- Header ----------
    Item {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 56
        Rectangle { anchors.fill: parent; color: theme.panel }

        Text {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "SMB-Q6R  Hardware Test"
            font.pixelSize: 22; font.bold: true
            color: theme.accent
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "leds=" + (model.ledReady    ? "ok" : "FAIL") +
                  "  sw=" + (model.switchReady ? "ok" : "FAIL") +
                  "  buz="+ (model.buzzerReady ? "ok" : "FAIL") +
                  "  bl=" + (model.backlightReady ? "ok" : "FAIL") +
                  "  keys="+ (model.keysReady    ? "ok" : "FAIL")
            font.pixelSize: 14; font.family: "monospace"
            color: theme.muted
        }
    }

    // ---------- LED row ----------
    Item {
        id: ledArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 12
        height: 200

        Text {
            id: ledLabel
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top
            text: "LEDs  (port → fiziksel?)"
            font.pixelSize: 16; color: theme.text
        }

        Row {
            anchors.top: ledLabel.bottom; anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 18

            Repeater {
                model: 4
                Rectangle {
                    width: 130; height: 150; radius: 10
                    color: on ? "#2a5a2a" : theme.panel
                    border.color: on ? theme.accent : theme.border
                    border.width: 2

                    property int  port: index
                    property bool on:   ((root.ledShadow & (1 << port)) !== 0)

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top; anchors.topMargin: 10
                        text: "Port " + port
                        font.pixelSize: 16; font.bold: true; color: theme.text
                    }
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 4
                        width: 60; height: 60; radius: 30
                        color: on ? theme.accent : "#28324a"
                        border.color: on ? "#A4FF40" : theme.border
                        border.width: 3
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                        text: on ? "ON" : "OFF"
                        font.pixelSize: 13; color: on ? theme.accent : theme.muted
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: model.setLed(port, !parent.on)
                    }
                }
            }
        }
        // ALL OFF
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: 160; height: 30; radius: 6
            color: allOffMa.pressed ? "#A52525" : "#C03030"
            Text { anchors.centerIn: parent; text: "ALL OFF"; color: "white"; font.bold: true }
            MouseArea { id: allOffMa; anchors.fill: parent; onClicked: model.allLedsOff() }
        }
    }

    // ---------- Mode + Enable + Buzzer + Backlight panel ----------
    Item {
        id: midPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: ledArea.bottom
        anchors.topMargin: 12
        height: 230

        // --- Mode (left third) ---
        Rectangle {
            id: modeBox
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top; anchors.bottom: parent.bottom
            width: (parent.width - 64) / 3
            radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 10
                text: "Mode Switch"; font.pixelSize: 16; color: theme.text
            }
            Column {
                anchors.centerIn: parent
                spacing: 8
                Repeater {
                    model: ["Auto", "Manual", "Stop"]
                    Row {
                        spacing: 12
                        Rectangle {
                            width: 22; height: 22; radius: 11
                            color: root.modeText === modelData ? theme.accent : theme.border
                        }
                        Text {
                            text: modelData; font.pixelSize: 22
                            color: root.modeText === modelData ? theme.accent : theme.muted
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                text: "raw: " + root.modeByteText
                font.pixelSize: 12; font.family: "monospace"; color: theme.muted
            }
        }

        // --- Enable Switch (middle) ---
        Rectangle {
            id: enableBox
            anchors.left: modeBox.right; anchors.leftMargin: 16
            anchors.top: parent.top; anchors.bottom: parent.bottom
            width: modeBox.width
            radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 10
                text: "Enable Switch (S1 / S2)"; font.pixelSize: 16; color: theme.text
            }
            Row {
                anchors.centerIn: parent
                spacing: 28
                // S1 indicator
                Column {
                    spacing: 6
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 60; height: 60; radius: 30
                        color: root.enS1 ? theme.amber : theme.border
                        border.color: root.enS1 ? "#FFEB3B" : theme.border
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "S1: " + (root.enS1 ? "1" : "0")
                        font.pixelSize: 14; color: theme.text
                    }
                }
                Column {
                    spacing: 6
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 60; height: 60; radius: 30
                        color: root.enS2 ? theme.amber : theme.border
                        border.color: root.enS2 ? "#FFEB3B" : theme.border
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "S2: " + (root.enS2 ? "1" : "0")
                        font.pixelSize: 14; color: theme.text
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                text: "raw: " + root.enableByteText
                font.pixelSize: 12; font.family: "monospace"; color: theme.muted
            }
        }

        // --- Buzzer + Backlight (right) ---
        Rectangle {
            id: bbBox
            anchors.left: enableBox.right; anchors.leftMargin: 16
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.top: parent.top; anchors.bottom: parent.bottom
            radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

            // Buzzer row
            Row {
                anchors.top: parent.top; anchors.topMargin: 16
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8
                Repeater {
                    model: [ { label: "50ms",  ms: 50 },
                             { label: "200ms", ms: 200 },
                             { label: "500ms", ms: 500 },
                             { label: "1 sn",  ms: 1000 } ]
                    Rectangle {
                        width: 64; height: 40; radius: 6
                        color: ma.pressed ? "#0D47A1" : "#1976D2"
                        Text { anchors.centerIn: parent; text: modelData.label; color: "white"; font.pixelSize: 13 }
                        MouseArea { id: ma; anchors.fill: parent; onClicked: model.beep(modelData.ms) }
                    }
                }
            }
            Text {
                anchors.top: parent.top; anchors.topMargin: 70
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Buzzer beep"
                font.pixelSize: 12; color: theme.muted
            }

            // Backlight slider
            Item {
                id: blRow
                anchors.bottom: parent.bottom; anchors.bottomMargin: 24
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 16; anchors.rightMargin: 16
                height: 60

                Text {
                    id: blLabel
                    text: "Backlight: " + root.blValue + " / " + model.backlightMax
                    font.pixelSize: 14; color: theme.text
                }
                Rectangle {
                    id: track
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: blLabel.bottom; anchors.topMargin: 14
                    height: 10; radius: 5
                    color: "#28324a"
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top; anchors.bottom: parent.bottom
                        width: parent.width * root.blValue / model.backlightMax
                        radius: 5
                        color: theme.amber
                    }
                    Rectangle {
                        id: thumb
                        width: 22; height: 22; radius: 11
                        color: "white"
                        border.color: theme.amber; border.width: 2
                        anchors.verticalCenter: parent.verticalCenter
                        x: parent.width * root.blValue / model.backlightMax - width/2
                    }
                    MouseArea {
                        anchors.fill: parent
                        anchors.topMargin: -10; anchors.bottomMargin: -10
                        onPressed: { setFromMouse(mouseX) }
                        onPositionChanged: if (pressed) setFromMouse(mouseX)
                        function setFromMouse(mx) {
                            var v = Math.max(0, Math.min(model.backlightMax,
                                Math.round(mx / track.width * model.backlightMax)))
                            model.backlight = v
                            root.blValue = v
                        }
                    }
                }
            }
        }
    }

    // ---------- Matrix Key log ----------
    Rectangle {
        id: keyArea
        anchors.left: parent.left; anchors.leftMargin: 16
        anchors.right: parent.right; anchors.rightMargin: 16
        anchors.top: midPanel.bottom; anchors.topMargin: 12
        anchors.bottom: parent.bottom; anchors.bottomMargin: 16
        radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

        Text {
            id: keyTitle
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top; anchors.topMargin: 10
            text: "Matrix Keys  —  basın gözleyin"
            font.pixelSize: 16; color: theme.text
        }

        // Last key big readout
        Row {
            anchors.left: keyTitle.right; anchors.leftMargin: 24
            anchors.verticalCenter: keyTitle.verticalCenter
            spacing: 10
            Rectangle {
                width: 14; height: 14; radius: 7
                color: model.lastKeyPressed ? theme.accent : theme.border
            }
            Text {
                text: "Last: " + (model.lastKeyCode === 0 ? "(henüz tuş yok)"
                    : model.lastKeyName + " (code " + model.lastKeyCode + ")"
                    + "  " + (model.lastKeyPressed ? "PRESSED" : "released"))
                font.pixelSize: 14; font.family: "monospace"
                color: theme.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // History list — vanilla ListView, no Controls
        ListView {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 16; anchors.rightMargin: 16
            anchors.top: keyTitle.bottom; anchors.topMargin: 12
            anchors.bottom: parent.bottom; anchors.bottomMargin: 12
            model: root.keyHistArr
            clip: true
            spacing: 2
            delegate: Text {
                text: modelData
                font.pixelSize: 13; font.family: "monospace"
                color: theme.text
            }
        }
    }

    // ---------- Reactive shadows (so QML bindings react) ----------
    property int    ledShadow:    0
    property string modeText:     "—"
    property string modeByteText: "--------"
    property bool   enS1:         false
    property bool   enS2:         false
    property string enableByteText: "--------"
    property int    blValue:      50
    property var    keyHistArr:   []

    Connections {
        target: model
        onLedChanged:      root.ledShadow = model.ledMask
        onSwitchChanged:   { root.modeText = model.mode;
                             root.modeByteText = model.modeByte;
                             root.enS1 = model.enableS1;
                             root.enS2 = model.enableS2;
                             root.enableByteText = model.enableByte }
        onBacklightChanged: root.blValue = model.backlight
        onKeyEvent:        root.keyHistArr = model.keyHistory
    }

    Component.onCompleted: {
        root.ledShadow      = model.ledMask
        root.modeText       = model.mode
        root.modeByteText   = model.modeByte
        root.enS1           = model.enableS1
        root.enS2           = model.enableS2
        root.enableByteText = model.enableByte
        root.blValue        = model.backlight
        root.keyHistArr     = model.keyHistory
    }
}
