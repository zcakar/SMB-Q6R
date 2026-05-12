// SMB-Q6R Teach Pendant Hardware Mapping
//
// Visual mirror of the HN00-09Q6 (device code MAT-QT-TP-PC10C-Q6-UBT-L1)
// physical layout: LEDs and keypad live on the right (matching where the
// operator's right hand falls); mode switch, enable switch and ancillary
// controls live on the left.
//
// Auto-learn matrix-key mapping: press any physical button and its key
// code is recorded into the next empty cell automatically; subsequent
// presses just highlight the already-mapped cell.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#0a1126" }
        GradientStop { position: 1.0; color: "#050813" }
    }

    QtObject {
        id: pal
        readonly property color panel:    "#172033"
        readonly property color panelHi:  "#1f2a42"
        readonly property color border:   "#2c3a55"
        readonly property color text:     "#E8EEF7"
        readonly property color muted:    "#7e8ba6"
        readonly property color accent:   "#7CFC00"
        readonly property color amber:    "#FFD700"
        readonly property color red:      "#ff4d4d"
        readonly property color green:    "#4dff77"
        readonly property color blue:     "#3DA9FC"
        readonly property color violet:   "#9D6BFF"
    }

    // ─── Header ──────────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 56
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#1c2942" }
            GradientStop { position: 1.0; color: "#0d1424" }
        }

        Rectangle { // bottom accent line
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: pal.accent }
                GradientStop { position: 1.0; color: pal.violet }
            }
        }

        Column {
            anchors.left: parent.left; anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: "SMB-Q6R Teach Pendant Hardware Mapping"
                font.pixelSize: 20; font.bold: true; color: pal.accent
            }
            Text {
                text: "Device: MAT-QT-TP-PC10C-Q6-UBT-L1"
                font.pixelSize: 11; font.family: "monospace"; color: pal.muted
            }
        }

        // Status indicators (top-right)
        Row {
            anchors.right: parent.right; anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14
            Repeater {
                model: [
                    { name: "LED",   ok: diag.ledReady       },
                    { name: "SWT",   ok: diag.switchReady    },
                    { name: "BUZ",   ok: diag.buzzerReady    },
                    { name: "BL",    ok: diag.backlightReady },
                    { name: "KEYS",  ok: diag.keysReady      }
                ]
                Column {
                    spacing: 2
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 12; height: 12; radius: 6
                        color: modelData.ok ? pal.green : pal.muted
                        Rectangle { // outer glow when ok
                            visible: modelData.ok
                            anchors.centerIn: parent
                            width: 22; height: 22; radius: 11
                            color: "transparent"
                            border.color: pal.green; border.width: 1
                            opacity: 0.4
                        }
                    }
                    Text {
                        text: modelData.name; font.pixelSize: 9
                        color: modelData.ok ? pal.green : pal.muted
                    }
                }
            }
        }
    }

    // ─── LEFT PANEL ─────────────────────────────────────────────────
    Item {
        id: leftPanel
        anchors.left: parent.left; anchors.leftMargin: 14
        anchors.top: header.bottom; anchors.topMargin: 12
        width: 720
        anchors.bottom: parent.bottom; anchors.bottomMargin: 14

        // — Mode Switch panel (top-left) —
        Card {
            id: modeCard
            anchors.left: parent.left
            anchors.top: parent.top
            width: parent.width / 2 - 8
            height: 270
            title: "MODE SWITCH"

            Column {
                anchors.top: parent.top; anchors.topMargin: 50
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12

                ModePill {
                    name:  "AUTO"
                    pillColor: pal.green
                    active: root.modeText.toUpperCase() === "AUTO"
                }
                ModePill {
                    name:  "MANUAL"
                    pillColor: pal.amber
                    active: root.modeText.toUpperCase() === "MANUAL"
                }
                ModePill {
                    name:  "STOP"
                    pillColor: pal.red
                    active: root.modeText.toUpperCase() === "STOP"
                }
            }
            Text {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.modeText === "—"
                    ? "(anahtarı oynatın — algılama bekleniyor)"
                    : "raw byte: " + root.modeByteText
                font.pixelSize: 11; font.family: "monospace"
                color: root.modeText === "—" ? pal.amber : pal.muted
            }
        }

        // — Enable Switch panel —
        Card {
            id: enableCard
            anchors.right: parent.right
            anchors.top: parent.top
            width: parent.width / 2 - 8
            height: 270
            title: "ENABLE SWITCH  (tutamak)"

            Column {
                anchors.centerIn: parent
                spacing: 10

                EnableStage { label: "RELEASED"; stageColor: pal.muted
                               active: !root.enS1 && !root.enS2 }
                EnableStage { label: "ACTIVE";   stageColor: pal.green
                               active: root.enS1 && root.enS2 }
                EnableStage { label: "PANIC";    stageColor: pal.red
                               active: (root.enS1 && !root.enS2) || (!root.enS1 && root.enS2) }
            }
            Row {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 18
                Text { text: "S1:" + (root.enS1?"1":"0"); font.pixelSize: 11; color: root.enS1?pal.amber:pal.muted; font.family: "monospace" }
                Text { text: "S2:" + (root.enS2?"1":"0"); font.pixelSize: 11; color: root.enS2?pal.amber:pal.muted; font.family: "monospace" }
                Text { text: "raw: " + root.enableByteText; font.pixelSize: 11; color: pal.muted; font.family: "monospace" }
            }
        }

        // — Buzzer + Backlight —
        Card {
            id: bbCard
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: modeCard.bottom; anchors.topMargin: 12
            height: 130
            title: "BUZZER  ·  BACKLIGHT"

            Row {
                anchors.top: parent.top; anchors.topMargin: 44
                anchors.left: parent.left; anchors.leftMargin: 20
                spacing: 8
                Repeater {
                    model: [50, 200, 500, 1000]
                    PressButton {
                        label: modelData + " ms"
                        buttonColor: pal.blue
                        width: 76; height: 38
                        onClicked: diag.beep(modelData)
                    }
                }
                PressButton {
                    label: root.buzzerHeld ? "HOLD ON" : "hold"
                    buttonColor: root.buzzerHeld ? pal.red : "#3a4658"
                    width: 90; height: 38
                    onClicked: {
                        root.buzzerHeld = !root.buzzerHeld
                        diag.holdBuzzer(root.buzzerHeld)
                    }
                }
            }

            // Backlight slider (bottom)
            Item {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 20; anchors.rightMargin: 20
                anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                height: 30

                Text {
                    id: blText
                    anchors.left: parent.left
                    text: "Backlight: " + root.blValue
                    font.pixelSize: 12; color: pal.text
                }
                Rectangle {
                    id: blTrack
                    anchors.left: blText.right; anchors.leftMargin: 14
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: 8; radius: 4
                    color: pal.panelHi
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top; anchors.bottom: parent.bottom
                        width: parent.width * root.blValue / Math.max(1, diag.backlightMax)
                        radius: 4
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#8C5A00" }
                            GradientStop { position: 1.0; color: pal.amber }
                        }
                    }
                    Rectangle {
                        width: 20; height: 20; radius: 10
                        color: "#fffdf6"
                        border.color: pal.amber; border.width: 2
                        anchors.verticalCenter: parent.verticalCenter
                        x: Math.max(0, parent.width * root.blValue / Math.max(1, diag.backlightMax) - width / 2)
                    }
                    MouseArea {
                        anchors.fill: parent
                        anchors.topMargin: -12; anchors.bottomMargin: -12
                        onPressed: setFromMouse(mouseX)
                        onPositionChanged: if (pressed) setFromMouse(mouseX)
                        function setFromMouse(mx) {
                            var v = Math.max(5, Math.min(diag.backlightMax,
                                Math.round(mx / blTrack.width * diag.backlightMax)))
                            diag.backlight = v
                            root.blValue = v
                        }
                    }
                }
            }
        }

        // — Last key + history —
        Card {
            id: histCard
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: bbCard.bottom; anchors.topMargin: 12
            anchors.bottom: parent.bottom
            title: "KEY HISTORY"

            Rectangle {
                id: lastKeyBar
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                anchors.top: parent.top; anchors.topMargin: 38
                height: 44
                radius: 8
                color: diag.lastKeyPressed ? "#1a3f1a" : pal.panelHi
                border.color: diag.lastKeyPressed ? pal.green : pal.border
                border.width: 1

                Rectangle {
                    anchors.left: parent.left; anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18; height: 18; radius: 9
                    color: diag.lastKeyPressed ? pal.green : pal.border
                }
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 42
                    anchors.verticalCenter: parent.verticalCenter
                    text: diag.lastKeyCode === 0
                        ? "Last key: (henüz tuş yok — bir tuşa basın)"
                        : "Last: " + diag.lastKeyName + "  code=" + diag.lastKeyCode +
                          "   " + (diag.lastKeyPressed ? "▼ PRESSED" : "▲ released")
                    font.pixelSize: 14; font.bold: diag.lastKeyPressed
                    font.family: "monospace"
                    color: diag.lastKeyPressed ? pal.green : pal.text
                }
            }

            ListView {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                anchors.top: lastKeyBar.bottom; anchors.topMargin: 8
                anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                model: root.keyHistArr
                clip: true
                spacing: 1
                delegate: Text {
                    text: modelData
                    font.pixelSize: 11; font.family: "monospace"; color: pal.text
                }
            }
        }
    }

    // ─── RIGHT PANEL — pendant mirror ────────────────────────────────
    Item {
        id: rightPanel
        anchors.left: leftPanel.right; anchors.leftMargin: 14
        anchors.right: parent.right; anchors.rightMargin: 14
        anchors.top: header.bottom; anchors.topMargin: 12
        anchors.bottom: parent.bottom; anchors.bottomMargin: 14

        // LED bar — mirroring the top-of-pendant indicator strip
        Card {
            id: ledBar
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            height: 140
            title: "INDICATOR LEDs"

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 12
                spacing: 12

                LedTile {
                    label: "STOP";   sub: "port 0"; ledColor: pal.red
                    on: (root.ledShadow & 0x01) !== 0
                    onToggleRequested: diag.setLed(0, !on)
                }
                LedTile {
                    label: "SERVO";  sub: "port 1"; ledColor: pal.green
                    on: (root.ledShadow & 0x02) !== 0
                    onToggleRequested: diag.setLed(1, !on)
                }
                LedTile {
                    label: "ENABLE"; sub: "port 2"; ledColor: pal.green
                    on: (root.ledShadow & 0x04) !== 0
                    onToggleRequested: diag.setLed(2, !on)
                }
                LedTile {
                    label: "?";      sub: "port 3"; ledColor: pal.amber
                    on: (root.ledShadow & 0x08) !== 0
                    onToggleRequested: diag.setLed(3, !on)
                }

                Rectangle { // ALL OFF beside LEDs
                    width: 84; height: 110; radius: 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: aoMa.pressed ? "#5a1a1a" : "#3a1212"
                    border.color: pal.red; border.width: 2
                    Text {
                        anchors.centerIn: parent
                        text: "ALL\nOFF"; horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: 14; font.bold: true; color: pal.red
                    }
                    MouseArea { id: aoMa; anchors.fill: parent; onClicked: diag.allLedsOff() }
                }
            }
        }

        // Keypad — 2 cols × 7 rows mirroring the right side of the pendant
        Card {
            id: keypadCard
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: ledBar.bottom; anchors.topMargin: 12
            anchors.bottom: parent.bottom
            title: "MATRIX KEYS  ·  basın (otomatik haritalanır)"

            // Per-row layout: 7 rows top→bottom, each with [−] [+]
            Column {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                anchors.top: parent.top; anchors.topMargin: 44
                anchors.bottom: resetBar.top; anchors.bottomMargin: 10
                spacing: 4

                Repeater {
                    model: 7

                    Item {
                        property int rowN: index
                        width: parent.width
                        height: (parent.height - parent.spacing * 6) / 7

                        KeyCell {
                            anchors.left: parent.left
                            width: parent.width / 2 - 4
                            height: parent.height
                            cellIndex: rowN * 2          // left half of pair: "−"
                            sign: "−"
                            rowLabel: "row " + (rowN + 1)
                            code: root.keyMap[cellIndex] !== undefined ? root.keyMap[cellIndex] : -1
                            justPressed: code >= 0 && diag.lastKeyPressed && diag.lastKeyCode === code
                            selected: root.selectedCell === cellIndex
                            onTapped: root.tapCell(cellIndex)
                        }
                        KeyCell {
                            anchors.right: parent.right
                            width: parent.width / 2 - 4
                            height: parent.height
                            cellIndex: rowN * 2 + 1      // right half: "+"
                            sign: "+"
                            rowLabel: "row " + (rowN + 1)
                            code: root.keyMap[cellIndex] !== undefined ? root.keyMap[cellIndex] : -1
                            justPressed: code >= 0 && diag.lastKeyPressed && diag.lastKeyCode === code
                            selected: root.selectedCell === cellIndex
                            onTapped: root.tapCell(cellIndex)
                        }
                    }
                }
            }

            // Reset / counter footer
            Item {
                id: resetBar
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 14; anchors.rightMargin: 14
                height: 30

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Mapped: " + root.mappedCount + " / 14   " +
                          (root.mappedCount < 14 ? "(auto-learn aktif)" : "(haritalama tamam)")
                    font.pixelSize: 11; color: pal.muted
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 90; height: 24; radius: 4
                    color: resetMa.pressed ? "#3a2020" : "#241818"
                    border.color: pal.border; border.width: 1
                    Text { anchors.centerIn: parent; text: "reset map"; font.pixelSize: 11; color: pal.muted }
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
        }
    }

    // ─── Reactive state ──────────────────────────────────────────────
    property int     ledShadow:      0
    property string  modeText:       "—"
    property string  modeByteText:   "--------"
    property bool    enS1:           false
    property bool    enS2:           false
    property string  enableByteText: "--------"
    property int     blValue:        50
    property var     keyHistArr:     []
    property var     keyMap:         [-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1]
    property int     selectedCell:   -1
    property bool    buzzerHeld:     false

    // Count of cells already mapped (-1 means unmapped).
    property int     mappedCount: {
        var c = 0
        for (var i = 0; i < keyMap.length; i++) if (keyMap[i] >= 0) c++
        return c
    }

    function tapCell(idx) {
        // Manual override: tap any cell to clear it. The next physical key
        // press will be re-assigned to the first empty cell (cells fill
        // left-to-right top-to-bottom). Re-tap to cancel selection.
        if (root.selectedCell === idx) {
            root.selectedCell = -1
        } else {
            // Clear the tapped cell so it becomes the new "next-empty" slot.
            var copy = root.keyMap.slice()
            copy[idx] = -1
            root.keyMap = copy
            root.selectedCell = idx
        }
    }

    Connections {
        target: diag
        onLedChanged:        root.ledShadow = diag.ledMask
        onSwitchChanged: {
            root.modeText       = diag.mode
            root.modeByteText   = diag.modeByte
            root.enS1           = diag.enableS1
            root.enS2           = diag.enableS2
            root.enableByteText = diag.enableByte
        }
        onBacklightChanged:  root.blValue = diag.backlight
        onKeyEvent: {
            // Auto-learn: on a fresh (un-mapped) press, assign the code to
            // the next empty cell (lowest index where keyMap[i] === -1).
            if (diag.lastKeyPressed) {
                var code = diag.lastKeyCode
                var copy = root.keyMap.slice()
                var alreadyMapped = false
                for (var i = 0; i < copy.length; i++) {
                    if (copy[i] === code) { alreadyMapped = true; break }
                }
                if (!alreadyMapped) {
                    for (var j = 0; j < copy.length; j++) {
                        if (copy[j] === -1) {
                            copy[j] = code
                            root.keyMap = copy
                            break
                        }
                    }
                }
            }
            root.keyHistArr = diag.keyHistory
        }
    }

    Component.onCompleted: {
        root.ledShadow      = diag.ledMask
        root.modeText       = diag.mode
        root.modeByteText   = diag.modeByte
        root.enS1           = diag.enableS1
        root.enS2           = diag.enableS2
        root.enableByteText = diag.enableByte
        root.blValue        = diag.backlight
        root.keyHistArr     = diag.keyHistory
    }

}
