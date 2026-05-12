// SMB-Q6R Teach Pendant Hardware Mapping
//
// Visual mirror of the HN00-09Q6 (device code MAT-QT-TP-PC10C-Q6-UBT-L1).
// Light theme inspired by ABB FlexPendant / FANUC iPendant; layout uses
// explicit pixel positioning to avoid the anchor cascading bugs that
// caused earlier shifts.
//
// Auto-learn matrix-key mapping: any fresh physical-key press is recorded
// into the next empty cell. Re-press an already-mapped key to flash it.
// Tap a cell to clear it; the next press is then captured at that slot.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#f3f4f6"

    // ───── Palette ─────────────────────────────────────────────────
    QtObject {
        id: pal
        readonly property color bg:        "#f3f4f6"
        readonly property color text:      "#111827"
        readonly property color textSub:   "#4b5563"
        readonly property color textMuted: "#9ca3af"
        readonly property color border:    "#d1d5db"
        readonly property color borderHi:  "#9ca3af"
        readonly property color cardBg:    "#ffffff"
        readonly property color titleBg:   "#f3f4f6"
        readonly property color accent:    "#2563eb"
        readonly property color success:   "#16a34a"
        readonly property color warning:   "#d97706"
        readonly property color danger:    "#dc2626"
    }

    // ───── Header ──────────────────────────────────────────────────
    Rectangle {
        id: header
        x: 0; y: 0
        width: parent.width; height: 70
        color: "#ffffff"

        Column {
            anchors.left: parent.left; anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 3
            Text {
                text: "SMB-Q6R  Teach Pendant Hardware Mapping"
                font.pixelSize: 22; font.bold: true; color: pal.text
            }
            Text {
                text: "Device Code:  MAT-QT-TP-PC10C-Q6-UBT-L1"
                font.pixelSize: 12; font.family: "monospace"; color: pal.textSub
            }
        }
        // Right-side status indicators
        Row {
            anchors.right: parent.right; anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 18
            Repeater {
                model: [
                    { name: "LED",  ok: diag.ledReady       },
                    { name: "SWT",  ok: diag.switchReady    },
                    { name: "BUZ",  ok: diag.buzzerReady    },
                    { name: "BL",   ok: diag.backlightReady },
                    { name: "KEYS", ok: diag.keysReady      }
                ]
                Column {
                    spacing: 3
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 14; height: 14; radius: 7
                        color: modelData.ok ? pal.success : pal.borderHi
                        border.color: modelData.ok ? Qt.darker(pal.success, 1.3) : pal.border
                        border.width: 1
                    }
                    Text {
                        text: modelData.name
                        font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.5
                        color: modelData.ok ? pal.success : pal.textMuted
                    }
                }
            }
        }
        // Bottom accent line — ABB-style blue separator
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 3
            color: pal.accent
        }
    }

    // ───── LEFT PANEL ──────────────────────────────────────────────
    // Mode switch + deadman + buzzer/backlight + history
    Item {
        id: leftPanel
        x: 12; y: header.height + 12
        width: 514
        height: root.height - header.height - 24

        // Mode switch
        Card {
            id: modeCard
            x: 0; y: 0
            width: parent.width; height: 174
            title: "MODE SWITCH"

            Column {
                anchors.top: parent.top; anchors.topMargin: 44
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                ModePill {
                    name: "AUTO"
                    activeColor: pal.success
                    active: root.modeText.toUpperCase() === "AUTO"
                }
                ModePill {
                    name: "MANUAL"
                    activeColor: pal.warning
                    active: root.modeText.toUpperCase() === "MANUAL"
                }
            }
            // STOP pill on the right, larger
            ModePill {
                anchors.right: parent.right; anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 8
                width: 200
                name: "STOP"
                activeColor: pal.danger
                active: root.modeText.toUpperCase() === "STOP"
            }
            // Footer hint / raw byte
            Text {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.modeText === "—"
                    ? "↻  Anahtarı bir kez oynatın — sürücü yalnızca değişim algılar"
                    : "raw byte:  " + root.modeByteText
                font.pixelSize: 11; font.family: "monospace"
                color: root.modeText === "—" ? pal.warning : pal.textMuted
            }
        }

        // Deadman switch
        Card {
            id: deadmanCard
            x: 0; y: modeCard.y + modeCard.height + 10
            width: parent.width; height: 174
            title: "DEADMAN SWITCH  (tutamak)"

            Column {
                anchors.top: parent.top; anchors.topMargin: 44
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                DeadmanStage {
                    label: "RELEASED"
                    stageColor: pal.textMuted
                    active: !root.enS1 && !root.enS2
                }
                DeadmanStage {
                    label: "ACTIVE"
                    stageColor: pal.success
                    active: root.enS1 && root.enS2
                }
            }
            DeadmanStage {
                anchors.right: parent.right; anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 8
                width: 200
                label: "PANIC"
                stageColor: pal.danger
                active: (root.enS1 !== root.enS2)
            }
            Row {
                anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 24
                Text { text: "S1: " + (root.enS1 ? "1" : "0"); font.pixelSize: 11; font.family: "monospace"
                       color: root.enS1 ? pal.warning : pal.textMuted }
                Text { text: "S2: " + (root.enS2 ? "1" : "0"); font.pixelSize: 11; font.family: "monospace"
                       color: root.enS2 ? pal.warning : pal.textMuted }
                Text { text: "raw: " + root.enableByteText; font.pixelSize: 11; font.family: "monospace"
                       color: pal.textMuted }
            }
        }

        // Buzzer + Backlight
        Card {
            id: bbCard
            x: 0; y: deadmanCard.y + deadmanCard.height + 10
            width: parent.width; height: 134
            title: "BUZZER  ·  BACKLIGHT"

            // Buzzer beep buttons + HOLD
            Row {
                anchors.left: parent.left; anchors.leftMargin: 14
                anchors.top: parent.top; anchors.topMargin: 44
                spacing: 6
                Repeater {
                    model: [50, 200, 500, 1000]
                    PressButton {
                        width: 70; height: 38
                        label: modelData + " ms"
                        buttonColor: pal.accent
                        onClicked: diag.beep(modelData)
                    }
                }
                PressButton {
                    width: 96; height: 38
                    label: root.buzzerHeld ? "HOLD ON" : "hold"
                    buttonColor: root.buzzerHeld ? pal.danger : "#6b7280"
                    onClicked: {
                        root.buzzerHeld = !root.buzzerHeld
                        diag.holdBuzzer(root.buzzerHeld)
                    }
                }
            }
            // Backlight slider
            Item {
                anchors.left: parent.left; anchors.leftMargin: 14
                anchors.right: parent.right; anchors.rightMargin: 14
                anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                height: 30

                Text {
                    id: blText
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Backlight " + root.blValue
                    font.pixelSize: 12; color: pal.text
                }
                Rectangle {
                    id: blTrack
                    anchors.left: blText.right; anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: 8; radius: 4
                    color: "#e5e7eb"
                    border.color: pal.border; border.width: 1
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top; anchors.bottom: parent.bottom
                        anchors.margins: 1
                        width: (parent.width - 2) * root.blValue / Math.max(1, diag.backlightMax)
                        radius: 3
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#fde68a" }
                            GradientStop { position: 1.0; color: "#f59e0b" }
                        }
                    }
                    Rectangle {
                        width: 18; height: 18; radius: 9
                        color: "#fffdf6"
                        border.color: "#f59e0b"; border.width: 2
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

        // History
        Card {
            id: histCard
            x: 0; y: bbCard.y + bbCard.height + 10
            width: parent.width
            height: leftPanel.height - y
            title: "KEY HISTORY"

            Rectangle {
                id: lastKeyBar
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 12; anchors.rightMargin: 12
                anchors.top: parent.top; anchors.topMargin: 42
                height: 44
                radius: 6
                color: diag.lastKeyPressed ? "#dcfce7" : "#f9fafb"
                border.color: diag.lastKeyPressed ? pal.success : pal.border
                border.width: 1

                Rectangle {
                    anchors.left: parent.left; anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: 14; height: 14; radius: 7
                    color: diag.lastKeyPressed ? pal.success : pal.borderHi
                }
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 40
                    anchors.verticalCenter: parent.verticalCenter
                    text: diag.lastKeyCode === 0
                        ? "Last key:  (henüz tuş yok — bir fiziksel butona basın)"
                        : "Last:  " + diag.lastKeyName + "   code = " + diag.lastKeyCode +
                          "   " + (diag.lastKeyPressed ? "▼ PRESSED" : "▲ released")
                    font.pixelSize: 13; font.family: "monospace"
                    font.bold: diag.lastKeyPressed
                    color: diag.lastKeyPressed ? "#166534" : pal.text
                }
            }
            ListView {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: lastKeyBar.bottom; anchors.bottom: parent.bottom
                anchors.leftMargin: 12; anchors.rightMargin: 12
                anchors.topMargin: 6; anchors.bottomMargin: 10
                clip: true
                spacing: 1
                model: root.keyHistArr
                delegate: Text {
                    text: modelData
                    font.pixelSize: 11; font.family: "monospace"
                    color: pal.textSub
                }
            }
        }
    }

    // ───── RIGHT PANEL ─────────────────────────────────────────────
    // Indicator LEDs + 2×7 matrix-key mirror
    Item {
        id: rightPanel
        x: leftPanel.x + leftPanel.width + 12
        y: header.height + 12
        width: root.width - x - 12
        height: root.height - header.height - 24

        // LEDs
        Card {
            id: ledCard
            x: 0; y: 0
            width: parent.width; height: 130
            title: "INDICATOR LEDs"

            Row {
                anchors.top: parent.top; anchors.topMargin: 38
                anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                LedTile {
                    width: 130; height: parent.height
                    label: "STOP";   sub: "port 0"; ledColor: pal.danger
                    on: (root.ledShadow & 0x01) !== 0
                    onToggleRequested: diag.setLed(0, !on)
                }
                LedTile {
                    width: 130; height: parent.height
                    label: "SERVO";  sub: "port 1"; ledColor: pal.success
                    on: (root.ledShadow & 0x02) !== 0
                    onToggleRequested: diag.setLed(1, !on)
                }
                LedTile {
                    width: 130; height: parent.height
                    label: "ENABLE"; sub: "port 2"; ledColor: pal.success
                    on: (root.ledShadow & 0x04) !== 0
                    onToggleRequested: diag.setLed(2, !on)
                }
                LedTile {
                    width: 130; height: parent.height
                    label: "?";      sub: "port 3"; ledColor: pal.warning
                    on: (root.ledShadow & 0x08) !== 0
                    onToggleRequested: diag.setLed(3, !on)
                }
                Rectangle {
                    width: 90; height: parent.height; radius: 8
                    color: aoMa.pressed ? "#fee2e2" : "#ffffff"
                    border.color: pal.danger; border.width: 2
                    Text {
                        anchors.centerIn: parent
                        text: "ALL\nOFF"; horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: 14; font.bold: true; color: pal.danger
                    }
                    MouseArea { id: aoMa; anchors.fill: parent; onClicked: diag.allLedsOff() }
                }
            }
        }

        // Matrix keypad — 7 rows × 2 cols mirroring physical layout
        Card {
            id: keypadCard
            x: 0; y: ledCard.y + ledCard.height + 12
            width: parent.width
            height: rightPanel.height - y
            title: "MATRIX KEYPAD  (otomatik haritalama)"

            Column {
                anchors.left: parent.left; anchors.leftMargin: 12
                anchors.right: parent.right; anchors.rightMargin: 12
                anchors.top: parent.top; anchors.topMargin: 42
                anchors.bottom: footer.top; anchors.bottomMargin: 6
                spacing: 5

                Repeater {
                    model: 7
                    Item {
                        property int rowN: index
                        width: parent.width
                        height: (parent.height - parent.spacing * 6) / 7

                        KeyCell {
                            anchors.left: parent.left
                            width: (parent.width - 8) / 2
                            height: parent.height
                            cellIndex: rowN * 2
                            sign: "−"
                            axisLabel: "Axis " + (rowN + 1) + "  ·  J" + (rowN + 1) + "−"
                            code: root.keyMap[cellIndex] !== undefined ? root.keyMap[cellIndex] : -1
                            justPressed: code >= 0 && diag.lastKeyPressed && diag.lastKeyCode === code
                            selected: root.selectedCell === cellIndex
                            onTapped: root.tapCell(cellIndex)
                        }
                        KeyCell {
                            anchors.right: parent.right
                            width: (parent.width - 8) / 2
                            height: parent.height
                            cellIndex: rowN * 2 + 1
                            sign: "+"
                            axisLabel: "Axis " + (rowN + 1) + "  ·  J" + (rowN + 1) + "+"
                            code: root.keyMap[cellIndex] !== undefined ? root.keyMap[cellIndex] : -1
                            justPressed: code >= 0 && diag.lastKeyPressed && diag.lastKeyCode === code
                            selected: root.selectedCell === cellIndex
                            onTapped: root.tapCell(cellIndex)
                        }
                    }
                }
            }

            Item {
                id: footer
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12; anchors.rightMargin: 12
                height: 32
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Mapped:  " + root.mappedCount + " / 14   " +
                          (root.mappedCount < 14 ? "(otomatik öğrenme aktif)" : "(haritalama tamam)")
                    font.pixelSize: 11; color: pal.textSub
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 90; height: 26; radius: 4
                    color: resetMa.pressed ? "#fee2e2" : "#fafafa"
                    border.color: pal.border; border.width: 1
                    Text { anchors.centerIn: parent; text: "reset map"; font.pixelSize: 11; color: pal.textSub }
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

    // ───── Reactive state ──────────────────────────────────────────
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

    property int     mappedCount: {
        var c = 0
        for (var i = 0; i < keyMap.length; i++) if (keyMap[i] >= 0) c++
        return c
    }

    function tapCell(idx) {
        if (root.selectedCell === idx) {
            root.selectedCell = -1
        } else {
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
