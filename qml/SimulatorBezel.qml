// SMB-Q6R Host Simulator Bezel
//
// Wraps the touchscreen UI (Main.qml) with a clickable representation of
// the physical HN00-09Q6 pendant body: mode key switch, E-stop, status
// LEDs, 14 jog buttons (J1-J7 ±), and a deadman state selector. Each
// virtual control drives the same DiagnosticsModel sim* slots a real
// kernel event would route to, so the inner UI behaves identically to
// running on the actual device.
//
// Only loaded by main.cpp when DiagnosticsModel reports simulatorMode().

import QtQuick 2.15

Rectangle {
    id: bezel
    width: 1600
    height: 1000
    color: "#1f2937"        // dark pendant body
    focus: true

    // Operator can also type on the host keyboard; each key forwards an
    // EV_KEY event into the matrix-keys monitor, so any binding inside
    // Main.qml that watches lastKeyCode behaves the same as a click on
    // the virtual J-button.
    Keys.onPressed:  diag.simKeyEvent(event.nativeScanCode - 8, true)
    Keys.onReleased: diag.simKeyEvent(event.nativeScanCode - 8, false)

    // ─── Color tokens — match the FlexPendant-inspired palette used in
    //     Main.qml so the bezel feels continuous with the screen ──────
    readonly property color caseBg:      "#1f2937"
    readonly property color caseLine:    "#374151"
    readonly property color keyFace:     "#111827"
    readonly property color keyHi:       "#fbbf24"
    readonly property color keyLabel:    "#f3f4f6"
    readonly property color labelMuted:  "#9ca3af"
    readonly property color brandText:   "#6b7280"

    // ─── Top bezel: mode switch · status LEDs · E-STOP ──────────────
    Item {
        id: topBezel
        x: 0; y: 0
        width: parent.width; height: 100

        // 3-position mode key switch (Stop / Manual / Auto)
        Rectangle {
            id: modeSwitch
            x: 30; y: 18
            width: 280; height: 64
            color: bezel.keyFace
            border.color: bezel.caseLine; border.width: 2
            radius: 8

            Text {
                anchors.left: parent.left; anchors.leftMargin: 10
                anchors.top:  parent.top;  anchors.topMargin: 4
                text: "MODE"
                font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.0
                color: bezel.labelMuted
            }

            Row {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 6
                spacing: 4
                Repeater {
                    model: [
                        { label: "STOP",   name: "Stop"   },
                        { label: "MANUAL", name: "Manual" },
                        { label: "AUTO",   name: "Auto"   }
                    ]
                    Rectangle {
                        width: 82; height: 38
                        property bool sel: diag.mode === modelData.name
                        color: sel ? bezel.keyHi : "#374151"
                        border.color: sel ? "#fde68a" : "#4b5563"
                        border.width: 1
                        radius: 4
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 12; font.bold: true
                            color: sel ? "#1f2937" : bezel.keyLabel
                        }
                        MouseArea { anchors.fill: parent; onClicked: diag.simSetMode(modelData.name) }
                    }
                }
            }
        }

        // 3 indicator LEDs — read-only mirror of the real STOP/SERVO/ENABLE
        // ports (defaultKeyMap doc'd the port -> physical-LED mapping in
        // Main.qml; we replicate just the three visible ones here).
        Row {
            id: ledStack
            anchors.right: estop.left; anchors.rightMargin: 40
            anchors.verticalCenter: parent.verticalCenter
            spacing: 18
            Repeater {
                model: [
                    { name: "STOP",   port: 2, color: "#ef4444" },
                    { name: "SERVO",  port: 1, color: "#22c55e" },
                    { name: "ENABLE", port: 0, color: "#22c55e" }
                ]
                Column {
                    spacing: 4
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 14; height: 14; radius: 7
                        property bool on: (diag.ledMask & (1 << modelData.port)) !== 0
                        color: on ? modelData.color : "#1f2937"
                        border.color: on ? Qt.lighter(modelData.color, 1.3) : "#4b5563"
                        border.width: 1
                    }
                    Text {
                        text: modelData.name
                        font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.8
                        color: bezel.labelMuted
                    }
                }
            }
        }

        // E-STOP mushroom button. On real hardware the E-stop is wired
        // straight to the safety contactor and is invisible to software,
        // so this is a static visual — included so the layout matches the
        // physical pendant the operator sees.
        Rectangle {
            id: estop
            anchors.right: parent.right; anchors.rightMargin: 30
            anchors.verticalCenter: parent.verticalCenter
            width: 82; height: 82; radius: 41
            color: "#facc15"        // yellow surround
            border.color: "#a16207"; border.width: 3
            Rectangle {
                anchors.centerIn: parent
                width: 60; height: 60; radius: 30
                color: "#dc2626"    // red mushroom
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#ef4444" }
                    GradientStop { position: 1.0; color: "#991b1b" }
                }
            }
            Text {
                anchors.centerIn: parent
                text: "STOP"
                font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0
                color: "#ffffff"
            }
        }
    }

    // ─── Screen area (loads Main.qml) ───────────────────────────────
    // The real device touchscreen is 1280×800. We render Main.qml at
    // that exact size inside a slight recessed frame so it looks inset
    // into the pendant body.
    Rectangle {
        id: screenFrame
        x: 130; y: topBezel.height + 4
        width: 1280; height: 800
        color: "#000000"
        border.color: bezel.caseLine; border.width: 4
        radius: 2

        Loader {
            anchors.centerIn: parent
            width: 1280; height: 800
            source: "qrc:/qml/Main.qml"
        }
    }

    // ─── Right side: 7 rows × 2 jog buttons (J1- J1+ ... J7- J7+) ───
    Item {
        id: jogPanel
        x: screenFrame.x + screenFrame.width + 22
        y: screenFrame.y
        width: bezel.width - x - 18
        height: screenFrame.height

        // diag.defaultKeyMap is a 14-element list in screen order:
        //   [J1-, J1+, J2-, J2+, ..., J7-, J7+]
        readonly property var codes: diag.defaultKeyMap

        Column {
            anchors.fill: parent
            spacing: 8
            Repeater {
                model: 7
                Item {
                    property int axisIdx: index
                    width: parent.width
                    height: (parent.height - parent.spacing * 6) / 7

                    Row {
                        anchors.fill: parent
                        spacing: 6
                        Repeater {
                            model: [
                                { dirChar: "−", offset: 0 },
                                { dirChar: "+", offset: 1 }
                            ]
                            Rectangle {
                                width: (parent.width - parent.spacing) / 2
                                height: parent.height
                                property int  code: jogPanel.codes[axisIdx * 2 + modelData.offset]
                                property bool active: diag.lastKeyPressed
                                                   && diag.lastKeyCode === code
                                color: jogMa.pressed || active ? bezel.keyHi : bezel.keyFace
                                border.color: bezel.caseLine; border.width: 2
                                radius: 6
                                Text {
                                    anchors.centerIn: parent
                                    text: "J" + (axisIdx + 1) + "  " + modelData.dirChar
                                    font.pixelSize: 18; font.bold: true
                                    color: jogMa.pressed || active ? "#1f2937" : bezel.keyLabel
                                }
                                MouseArea {
                                    id: jogMa
                                    anchors.fill: parent
                                    // Real matrix-keypad driver emits EV_KEY value=1 on press
                                    // and value=0 on release; mirror that exactly.
                                    onPressed:  diag.simKeyEvent(parent.code, true)
                                    onReleased: diag.simKeyEvent(parent.code, false)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ─── Bottom bezel: deadman state + brand ────────────────────────
    Item {
        id: bottomBezel
        x: 0
        y: screenFrame.y + screenFrame.height + 4
        width: parent.width
        height: parent.height - y

        // Left side: deadman 3-state pill row. On real hardware the
        // deadman lives on the back of the handgrip; here it's a click-
        // injector so the operator can flip between Released, S1, S1+S2
        // while sitting at a desk.
        Row {
            x: 30
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "DEADMAN"
                font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.0
                color: bezel.labelMuted
                rightPadding: 6
            }

            Repeater {
                model: [
                    { label: "RELEASED", s1: false, s2: false },
                    { label: "S1",       s1: true,  s2: false },
                    { label: "S1+S2",    s1: true,  s2: true  }
                ]
                Rectangle {
                    property bool sel: diag.enableS1 === modelData.s1
                                    && diag.enableS2 === modelData.s2
                    width: 100; height: 36
                    color: sel ? bezel.keyHi : "#374151"
                    border.color: sel ? "#fde68a" : "#4b5563"
                    border.width: 1
                    radius: 4
                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: 11; font.bold: true
                        color: sel ? "#1f2937" : bezel.keyLabel
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: diag.simSetDeadman(modelData.s1, modelData.s2)
                    }
                }
            }
        }

        // Right side: model + simulator marker.
        Column {
            anchors.right: parent.right; anchors.rightMargin: 30
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                anchors.right: parent.right
                text: "HN00-09Q6  ·  Lavichip"
                font.pixelSize: 11; font.bold: true
                color: bezel.keyLabel
            }
            Text {
                anchors.right: parent.right
                text: "SMB-Q6R  ·  Host Simulator"
                font.pixelSize: 10
                color: bezel.brandText
            }
        }
    }
}
