// Phase 1 Diagnostics — visual pendant mirror.
//
// Layout mirrors the HN00-09Q6 physical pendant:
//   * Three named indicator LEDs (STOP / SERVO / ENABLE) — touch to toggle
//   * Mode switch rotary visual (Auto / Manual / Stop)
//   * Enable switch (S1 / S2) live indicator with derived state label
//   * 14-cell key matrix (7 cols × 2 rows: top "-" / bottom "+")
//     User assigns each physical button to a cell by tapping the cell
//     then pressing the button — code recorded in place.
//   * Buzzer + backlight controls
//   * Live "last key" readout + scrollable history footer
//
// Pure QtQuick 2.12; no Controls / Layouts / Window modules required.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800
    color: theme.bg

    // ---------- Theme ----------
    QtObject {
        id: theme
        readonly property color bg:        "#0a0f1c"
        readonly property color panel:     "#172033"
        readonly property color panelHi:   "#1f2a42"
        readonly property color border:    "#2c3a55"
        readonly property color text:      "#E8EEF7"
        readonly property color textMuted: "#8a99b3"
        readonly property color accent:    "#7CFC00"
        readonly property color amber:     "#FFD700"
        readonly property color red:       "#ff4d4d"
        readonly property color green:     "#4dff77"
        readonly property color blue:      "#3DA9FC"
    }

    // ---------- Header (40 px) ----------
    Rectangle {
        id: header
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 40
        color: theme.panel

        Text {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "SMB-Q6R  ·  Hardware Mirror"
            font.pixelSize: 18; font.bold: true; color: theme.accent
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text:  "led=" + (model.ledReady?"●":"○") +
                   "  sw="  + (model.switchReady?"●":"○") +
                   "  buz=" + (model.buzzerReady?"●":"○") +
                   "  bl="  + (model.backlightReady?"●":"○") +
                   "  kbd=" + (model.keysReady?"●":"○")
            font.pixelSize: 12; font.family: "monospace"; color: theme.textMuted
        }
    }

    // ---------- Row 1: LEDs + Mode + Enable (160 px) ----------
    Item {
        id: row1
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: header.bottom; anchors.topMargin: 10
        height: 160

        // --- 4 LED tiles in a row ---
        Row {
            id: ledRow
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            // Three named LEDs (STOP / SERVO / ENABLE) — assumed mapping
            // (left-to-right per HN00-09Q6 datasheet §2.1.1). Spare port 3
            // shown separately in case the assumption is wrong.
            LedTile { label: "STOP";   sub: "port 0"; ledPort: 0; ledColor: theme.red
                       on: (root.ledShadow & 0x01) !== 0
                       onToggleRequested: model.setLed(0, !on) }
            LedTile { label: "SERVO";  sub: "port 1"; ledPort: 1; ledColor: theme.green
                       on: (root.ledShadow & 0x02) !== 0
                       onToggleRequested: model.setLed(1, !on) }
            LedTile { label: "ENABLE"; sub: "port 2"; ledPort: 2; ledColor: theme.green
                       on: (root.ledShadow & 0x04) !== 0
                       onToggleRequested: model.setLed(2, !on) }
            LedTile { label: "?";      sub: "port 3"; ledPort: 3; ledColor: theme.amber
                       on: (root.ledShadow & 0x08) !== 0
                       onToggleRequested: model.setLed(3, !on) }

            // ALL OFF button column
            Item {
                width: 110; height: ledRow.height
                Rectangle {
                    anchors.centerIn: parent
                    width: 100; height: 38; radius: 6
                    color: allOffMa.pressed ? "#A52525" : "#C03030"
                    Text { anchors.centerIn: parent; text: "ALL OFF"; color: "white"; font.bold: true; font.pixelSize: 13 }
                    MouseArea { id: allOffMa; anchors.fill: parent; onClicked: model.allLedsOff() }
                }
            }
        }

        // --- Mode Switch panel ---
        Rectangle {
            id: modeBox
            anchors.left: ledRow.right; anchors.leftMargin: 20
            anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 220
            radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 10
                text: "MODE SWITCH"
                font.pixelSize: 12; font.bold: true; color: theme.textMuted
            }
            Column {
                anchors.centerIn: parent
                spacing: 8
                Repeater {
                    model: [
                        { name: "AUTO",   color: theme.green },
                        { name: "MANUAL", color: theme.amber },
                        { name: "STOP",   color: theme.red }
                    ]
                    Row {
                        spacing: 12
                        Rectangle {
                            width: 22; height: 22; radius: 11
                            color: root.modeText.toUpperCase() === modelData.name
                                   ? modelData.color : theme.border
                            border.color: root.modeText.toUpperCase() === modelData.name
                                          ? Qt.lighter(modelData.color, 1.3) : theme.border
                            border.width: 2
                        }
                        Text {
                            text: modelData.name; font.pixelSize: 20; font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.modeText.toUpperCase() === modelData.name
                                   ? modelData.color : theme.textMuted
                        }
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                text: "raw: " + root.modeByteText
                font.pixelSize: 11; font.family: "monospace"; color: theme.textMuted
            }
        }

        // --- Enable Switch panel ---
        Rectangle {
            id: enableBox
            anchors.left: modeBox.right; anchors.leftMargin: 16
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.top: parent.top; anchors.bottom: parent.bottom
            radius: 10; color: theme.panel; border.color: theme.border; border.width: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 10
                text: "ENABLE SWITCH  (tutamak arkası)"
                font.pixelSize: 12; font.bold: true; color: theme.textMuted
            }

            // Derived 3-state visual
            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 40
                width: 320; height: 60

                property string state:
                    root.enS1 && root.enS2 ? "ACTIVE" :
                    (!root.enS1 && !root.enS2 ? "RELEASED" : "PANIC")
                property color stateColor:
                    state === "ACTIVE"   ? theme.green :
                    state === "RELEASED" ? theme.textMuted :
                                           theme.red

                Row {
                    anchors.centerIn: parent
                    spacing: 14
                    Repeater {
                        model: [
                            { label: "RELEASED", color: theme.textMuted },
                            { label: "ACTIVE",   color: theme.green },
                            { label: "PANIC",    color: theme.red }
                        ]
                        Rectangle {
                            width: 100; height: 50; radius: 6
                            color: parent.parent.parent.state === modelData.label
                                   ? modelData.color : theme.panelHi
                            border.color: parent.parent.parent.state === modelData.label
                                          ? Qt.lighter(modelData.color, 1.3) : theme.border
                            border.width: 2
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                font.pixelSize: 14; font.bold: true
                                color: parent.parent.parent.parent.state === modelData.label
                                       ? "#001020" : theme.textMuted
                            }
                        }
                    }
                }
            }

            // S1 / S2 indicators + raw byte
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                spacing: 24
                Text {
                    text: "S1: " + (root.enS1 ? "1" : "0")
                    font.pixelSize: 12; color: root.enS1 ? theme.amber : theme.textMuted
                }
                Text {
                    text: "S2: " + (root.enS2 ? "1" : "0")
                    font.pixelSize: 12; color: root.enS2 ? theme.amber : theme.textMuted
                }
                Text {
                    text: "raw: " + root.enableByteText
                    font.pixelSize: 12; font.family: "monospace"; color: theme.textMuted
                }
            }
        }
    }

    // ---------- Row 2: 14-cell key matrix (280 px) ----------
    Item {
        id: row2
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: row1.bottom; anchors.topMargin: 10
        height: 280

        Text {
            id: kbTitle
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top
            text: "Matrix Keys  ·  cell'e dokun + fiziksel butona bas = haritalama"
            font.pixelSize: 13; color: theme.textMuted
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.top: parent.top
            text: "Last: " + (model.lastKeyCode === 0
                              ? "(yok)"
                              : model.lastKeyName + "  code=" + model.lastKeyCode +
                                "  " + (model.lastKeyPressed ? "▼ PRESSED" : "▲ released"))
            font.pixelSize: 13; font.family: "monospace"
            color: model.lastKeyPressed ? theme.accent : theme.textMuted
        }

        // 7-column × 2-row grid (top row = "-" jog, bottom = "+" jog per pendant photo)
        Grid {
            id: kbGrid
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.top: kbTitle.bottom; anchors.topMargin: 8
            columns: 7
            rows: 2
            spacing: 6
            property int cellW: (width - spacing * (columns - 1)) / columns
            property int cellH: 110

            Repeater {
                model: 14
                Rectangle {
                    width: kbGrid.cellW; height: kbGrid.cellH
                    radius: 8
                    property int cellIndex: index
                    property int colN: index % 7
                    property int rowN: Math.floor(index / 7)
                    property string sign: rowN === 0 ? "−" : "+"
                    property int  code: root.keyMap[cellIndex] || -1
                    property bool selected: root.selectedCell === cellIndex
                    property bool justPressed:
                        code >= 0 && model.lastKeyPressed && model.lastKeyCode === code

                    color: selected     ? "#5e4a00" :
                           justPressed  ? "#1a5f1a" :
                           code >= 0    ? theme.panelHi : theme.panel
                    border.color:
                        selected     ? theme.amber :
                        justPressed  ? theme.green :
                        code >= 0    ? theme.border : "#2a3548"
                    border.width: selected ? 3 : 1

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top; anchors.topMargin: 4
                        text: "col " + (parent.colN + 1)
                        font.pixelSize: 10; color: theme.textMuted
                    }
                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -8
                        text: parent.sign
                        font.pixelSize: 42; font.bold: true
                        color: parent.selected   ? theme.amber :
                               parent.justPressed? theme.green :
                               parent.code >= 0  ? theme.text :
                                                   theme.textMuted
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                        text: parent.code >= 0 ? "code " + parent.code : "—"
                        font.pixelSize: 11; font.family: "monospace"
                        color: parent.justPressed ? theme.green : theme.textMuted
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // Toggle selection: tap a cell, then press the
                            // physical button you want to assign here.
                            if (root.selectedCell === cellIndex) {
                                root.selectedCell = -1
                            } else {
                                root.selectedCell = cellIndex
                            }
                        }
                    }
                }
            }
        }

        // Reset map button (subtle)
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.right: parent.right; anchors.rightMargin: 16
            width: 100; height: 24; radius: 4
            color: resetMa.pressed ? "#3a2020" : "#2a1818"
            border.color: theme.border; border.width: 1
            Text { anchors.centerIn: parent; text: "reset map"; font.pixelSize: 11; color: theme.textMuted }
            MouseArea {
                id: resetMa
                anchors.fill: parent
                onClicked: {
                    var fresh = []
                    for (var i = 0; i < 14; i++) fresh.push(-1)
                    root.keyMap = fresh
                    root.selectedCell = -1
                }
            }
        }
    }

    // ---------- Row 3: Buzzer + Backlight (78 px) ----------
    Item {
        id: row3
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: row2.bottom; anchors.topMargin: 8
        height: 78

        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 16; anchors.rightMargin: 16
            radius: 8; color: theme.panel; border.color: theme.border; border.width: 1
        }

        // Buzzer (left). Driver has no volume control — timed beep can sound
        // faint; HOLD usually drives the PWM continuously and is louder.
        Row {
            anchors.left: parent.left; anchors.leftMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Text {
                text: "Buzzer"; font.pixelSize: 13; color: theme.textMuted
                anchors.verticalCenter: parent.verticalCenter
            }
            Repeater {
                model: [50, 200, 500, 1000]
                Rectangle {
                    width: 60; height: 36; radius: 6
                    color: ma.pressed ? "#0D47A1" : theme.blue
                    Text { anchors.centerIn: parent; text: modelData + "ms"; color: "white"; font.pixelSize: 11 }
                    MouseArea { id: ma; anchors.fill: parent; onClicked: model.beep(modelData) }
                }
            }
            // Hold toggle — continuous tone, typically louder than timed beep
            Rectangle {
                width: 100; height: 36; radius: 6
                color: root.buzzerHeld ? "#A52525" : "#2a3548"
                border.color: root.buzzerHeld ? theme.red : theme.border
                border.width: 2
                Text {
                    anchors.centerIn: parent
                    text: root.buzzerHeld ? "HOLD ON" : "hold off"
                    font.pixelSize: 11; font.bold: true
                    color: root.buzzerHeld ? "white" : theme.text
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.buzzerHeld = !root.buzzerHeld
                        model.holdBuzzer(root.buzzerHeld)
                    }
                }
            }
        }

        // Backlight (right)
        Item {
            anchors.right: parent.right; anchors.rightMargin: 32
            anchors.verticalCenter: parent.verticalCenter
            width: 460; height: 50

            Text {
                id: blLbl
                anchors.left: parent.left
                anchors.top: parent.top
                text: "Backlight: " + root.blValue + " / " + model.backlightMax
                font.pixelSize: 13; color: theme.text
            }
            Rectangle {
                id: blTrack
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: blLbl.bottom; anchors.topMargin: 10
                height: 10; radius: 5
                color: theme.panelHi
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * root.blValue / Math.max(1, model.backlightMax)
                    radius: 5
                    color: theme.amber
                }
                Rectangle {
                    width: 22; height: 22; radius: 11
                    color: "white"; border.color: theme.amber; border.width: 2
                    anchors.verticalCenter: parent.verticalCenter
                    x: Math.max(0, parent.width * root.blValue / Math.max(1, model.backlightMax) - width / 2)
                }
                MouseArea {
                    anchors.fill: parent
                    anchors.topMargin: -10; anchors.bottomMargin: -10
                    onPressed: setFromMouse(mouseX)
                    onPositionChanged: if (pressed) setFromMouse(mouseX)
                    function setFromMouse(mx) {
                        var v = Math.max(5, Math.min(model.backlightMax,
                            Math.round(mx / blTrack.width * model.backlightMax)))
                        model.backlight = v
                        root.blValue = v
                    }
                }
            }
        }
    }

    // ---------- Row 4: History footer (rest of screen) ----------
    Rectangle {
        anchors.left: parent.left; anchors.leftMargin: 16
        anchors.right: parent.right; anchors.rightMargin: 16
        anchors.top: row3.bottom; anchors.topMargin: 8
        anchors.bottom: parent.bottom; anchors.bottomMargin: 12
        radius: 8; color: theme.panel; border.color: theme.border; border.width: 1

        Text {
            id: hLbl
            anchors.left: parent.left; anchors.leftMargin: 14
            anchors.top: parent.top; anchors.topMargin: 6
            text: "Key history (newest first)"
            font.pixelSize: 11; color: theme.textMuted
        }
        ListView {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 14; anchors.rightMargin: 14
            anchors.top: hLbl.bottom; anchors.topMargin: 4
            anchors.bottom: parent.bottom; anchors.bottomMargin: 6
            model: root.keyHistArr
            clip: true
            spacing: 1
            delegate: Text {
                text: modelData
                font.pixelSize: 11; font.family: "monospace"
                color: theme.text
            }
        }
    }

    // ---------- Reactive properties (QML → C++ bridge) ----------
    property int     ledShadow:      0
    property string  modeText:       "—"
    property string  modeByteText:   "--------"
    property bool    enS1:           false
    property bool    enS2:           false
    property string  enableByteText: "--------"
    property int     blValue:        50
    property var     keyHistArr:     []
    property bool    buzzerHeld:     false

    // 14 cells, initially unassigned (-1). Tap a cell, then press the
    // physical button you want there — the code is recorded.
    property var     keyMap:         [-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1]
    property int     selectedCell:   -1

    Connections {
        target: model
        onLedChanged:        root.ledShadow = model.ledMask
        onSwitchChanged: {
            root.modeText       = model.mode
            root.modeByteText   = model.modeByte
            root.enS1           = model.enableS1
            root.enS2           = model.enableS2
            root.enableByteText = model.enableByte
        }
        onBacklightChanged:  root.blValue = model.backlight
        onKeyEvent: {
            // Record the just-pressed code into the currently selected cell.
            if (root.selectedCell >= 0 && model.lastKeyPressed) {
                var copy = root.keyMap.slice()
                copy[root.selectedCell] = model.lastKeyCode
                root.keyMap = copy
                root.selectedCell = -1
            }
            root.keyHistArr = model.keyHistory
        }
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
