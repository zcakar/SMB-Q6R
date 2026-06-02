// PLC Console — fullscreen overlay for interacting with the live OPC UA
// connection. Reached from Main.qml's link-strip "PLC Console >" button,
// dismissed via the X in the upper-right corner.
//
// Layout
// ------
//   ╭ header ─────────────────────────────────────────────────╮
//   │  PLC CONSOLE          PLC: Connected      [X close]      │
//   │  Endpoint: opc.tcp://192.168.0.2:4840                    │
//   ╰──────────────────────────────────────────────────────────╯
//   ╭ left: watch table ────────────────╮ ╭ right: add / browse╮
//   │ Label    Value   Write   Remove   │ │  Add a node...     │
//   │ ───────────────────────────────── │ │  ns=4;s=...        │
//   │ Enable   TRUE   [set]   [✕]       │ │  [Subscribe]       │
//   │ EMG_01   false  ──     [✕]        │ │                    │
//   │ ...                               │ │  Recent log entries│
//   ╰───────────────────────────────────╯ ╰────────────────────╯
//
// Subscriptions are owned by PlcLink, but the page maintains its own
// `nodes` list so each row can carry display metadata (label, writable,
// kind) the PLC server can't tell us about.

import QtQuick 2.15
import QtQml 2.15

Rectangle {
    id: page
    color: "#0f172a"           // very dark slate to make the overlay feel modal

    QtObject {
        id: pal
        readonly property color text:     "#e2e8f0"
        readonly property color textSub:  "#94a3b8"
        readonly property color textDim:  "#64748b"
        readonly property color cardBg:   "#1e293b"
        readonly property color cardEdge: "#334155"
        readonly property color accent:   "#3b82f6"
        readonly property color success:  "#16a34a"
        readonly property color warning:  "#d97706"
        readonly property color danger:   "#dc2626"
        readonly property color rowEven:  "#1e293b"
        readonly property color rowOdd:   "#172033"
    }

    // ─── Default watch list — symbols we know live in this PLC's
    //     CodeSys symbol config. The user can add more via the right
    //     panel, and the list survives page reopens because page is
    //     reloaded on each open (subscriptions live in PlcLink, which
    //     persists across page reloads anyway). For host development
    //     without a server, this list still renders — pills just stay
    //     gray until a value arrives.
    property var nodes: [
        { id: "ns=4;s=|var|MAT LC-C07.Application.GVL.Enable",
          label: "GVL.Enable",  writable: true,  kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.EMG_01",
          label: "EMG_01",      writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Door_Left_10",
          label: "Door_Left",   writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Door_Right_11",
          label: "Door_Right",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Limit_Xpos_06",
          label: "Limit_Xpos",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Limit_xneg_07",
          label: "Limit_Xneg",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Limit_Ypos_05",
          label: "Limit_Ypos",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Limit_Yneg_04",
          label: "Limit_Yneg",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.Limit_Zneg_03",
          label: "Limit_Zneg",  writable: false, kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.emg_stop_Q00",
          label: "emg_stop",    writable: true,  kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.laser_ctrl_Q03",
          label: "laser_ctrl",  writable: true,  kind: "bool" },
        { id: "ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.pano_ayd_Q04",
          label: "pano_ayd",    writable: true,  kind: "bool" }
    ]

    // Rolling log of recent PLC events for the right column.
    property var log: []
    function logAppend(s) {
        var copy = page.log.slice()
        copy.unshift(Qt.formatTime(new Date(), "hh:mm:ss") + "  " + s)
        while (copy.length > 30) copy.pop()
        page.log = copy
    }

    // Subscribe on open + every reconnect.
    function subscribeAll() {
        for (var i = 0; i < page.nodes.length; i++) {
            diag.plcSubscribeNode(page.nodes[i].id)
        }
    }

    Connections {
        target: diag
        onPlcStateChanged: {
            page.logAppend("state -> " + diag.plcState)
            if (diag.plcState === "Connected") page.subscribeAll()
        }
        onPlcWriteSucceeded: page.logAppend("write OK  " + nodeId)
        onPlcWriteFailed:    page.logAppend("write ERR " + nodeId + " - " + reason)
        onPlcReadFailed:     page.logAppend("read ERR  " + nodeId + " - " + reason)
    }

    Component.onCompleted: {
        if (diag.plcState === "Connected") subscribeAll()
        logAppend("PLC Console opened")
    }

    // Swallow clicks so the underlying main UI doesn't react. The hardware
    // tab is hidden when this page is active, but click-through can still
    // hit anything sharing the same z order.
    MouseArea { anchors.fill: parent; onClicked: { /* eat */ } }

    // ─── Connect bar (page header) ───────────────────────────────
    // Pulls in the connection controls that used to live on Main.qml's
    // bottom strip. State pill on the left, editable endpoint URL in the
    // middle, Connect/Disconnect on the right.
    Rectangle {
        id: connectBar
        x: 12; y: 12
        width: parent.width - 24; height: 60
        color: pal.cardBg
        border.color: pal.cardEdge; border.width: 1
        radius: 6

        // State pill — colour codes the link status at a glance.
        Rectangle {
            id: statePill
            x: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 160; height: 36; radius: 4
            color: diag.plcState === "Connected"  ? pal.success
                 : diag.plcState === "Connecting" ? pal.warning
                 : diag.plcState === "Error"      ? pal.danger
                                                  : "#475569"
            Text {
                anchors.centerIn: parent
                text: "PLC: " + diag.plcState
                font.pixelSize: 13; font.bold: true; font.letterSpacing: 0.6
                color: "#ffffff"
            }
        }

        // Endpoint URL editor (monospace, click-to-focus).
        Rectangle {
            id: urlBox
            x: statePill.x + statePill.width + 14
            anchors.verticalCenter: parent.verticalCenter
            width: 520; height: 36; radius: 4
            color: pal.rowOdd
            border.color: urlField.activeFocus ? pal.accent : pal.cardEdge
            border.width: 1
            TextInput {
                id: urlField
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                verticalAlignment: TextInput.AlignVCenter
                font.pixelSize: 13; font.family: "monospace"
                color: pal.text
                selectByMouse: true
                clip: true
                text: diag.plcServerUrl.length > 0
                    ? diag.plcServerUrl
                    : "opc.tcp://192.168.0.2:4840"
            }
        }

        // Connect / Disconnect button — colour follows current state.
        Rectangle {
            id: connectBtn
            x: urlBox.x + urlBox.width + 14
            anchors.verticalCenter: parent.verticalCenter
            width: 140; height: 36; radius: 4
            property bool live: diag.plcState === "Connected"
                             || diag.plcState === "Connecting"
            color: connectMa.pressed
                ? Qt.darker(live ? pal.danger : pal.accent, 1.2)
                : (live ? pal.danger : pal.accent)
            Text {
                anchors.centerIn: parent
                text: connectBtn.live ? "Disconnect" : "Connect"
                font.pixelSize: 13; font.bold: true; font.letterSpacing: 0.6
                color: "#ffffff"
            }
            MouseArea {
                id: connectMa
                anchors.fill: parent
                onClicked: {
                    if (connectBtn.live) diag.plcDisconnect()
                    else                 diag.plcConnect(urlField.text)
                }
            }
        }

        // Last-error / endpoint hint fills the remainder.
        Text {
            anchors.left: connectBtn.right; anchors.leftMargin: 16
            anchors.right: parent.right; anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: diag.plcState === "Error" && diag.plcLastError.length > 0
                ? "Error: " + diag.plcLastError
                : diag.plcServerUrl.length > 0
                    ? "Endpoint: " + diag.plcServerUrl
                    : "Paste a CodeSys OPC UA endpoint above and Connect."
            font.pixelSize: 11; font.family: "monospace"
            color: diag.plcState === "Error" ? pal.danger : pal.textSub
            elide: Text.ElideRight
        }
    }

    // ─── Two-column body ────────────────────────────────────────
    Item {
        id: body
        x: 12; y: connectBar.y + connectBar.height + 12
        width: parent.width - 24
        height: parent.height - y - 12

        // ─ LEFT: subscribed nodes table ─
        Rectangle {
            id: tablePanel
            anchors.left: parent.left
            anchors.top:  parent.top
            anchors.bottom: parent.bottom
            width: Math.round(parent.width * 0.62)
            color: pal.cardBg
            border.color: pal.cardEdge; border.width: 1
            radius: 6

            // Column headers
            Item {
                id: tableHeader
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 12; anchors.rightMargin: 12
                height: 36
                Row {
                    anchors.fill: parent
                    spacing: 0
                    Text { width: 200; text: "LABEL";    font.pixelSize: 11; font.bold: true; color: pal.textDim
                           anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width - 200 - 120 - 100 - 60
                           text: "NODE ID"; font.pixelSize: 11; font.bold: true; color: pal.textDim
                           anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                    Text { width: 120; text: "VALUE";    font.pixelSize: 11; font.bold: true; color: pal.textDim
                           anchors.verticalCenter: parent.verticalCenter }
                    Text { width: 100; text: "ACTION";   font.pixelSize: 11; font.bold: true; color: pal.textDim
                           anchors.verticalCenter: parent.verticalCenter }
                    Text { width: 60;  text: "";         font.pixelSize: 11; font.bold: true; color: pal.textDim
                           anchors.verticalCenter: parent.verticalCenter }
                }
            }
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: tableHeader.bottom
                height: 1; color: pal.cardEdge
            }

            // Rows
            ListView {
                id: tableView
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: tableHeader.bottom; anchors.bottom: parent.bottom
                anchors.leftMargin: 12; anchors.rightMargin: 12
                anchors.topMargin: 1
                clip: true
                spacing: 0
                model: page.nodes
                delegate: Item {
                    width: tableView.width
                    height: 44

                    Rectangle {
                        anchors.fill: parent
                        color: index % 2 === 0 ? pal.rowEven : pal.rowOdd
                    }

                    Row {
                        anchors.fill: parent
                        spacing: 0

                        // Label
                        Text {
                            width: 200; height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.label
                            font.pixelSize: 13; font.bold: true
                            color: pal.text
                        }

                        // Node ID (monospace, ellipsized)
                        Text {
                            width: parent.width - 200 - 120 - 100 - 60
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.id
                            font.pixelSize: 11; font.family: "monospace"
                            color: pal.textSub
                            elide: Text.ElideMiddle
                        }

                        // Value pill
                        Item {
                            width: 120; height: parent.height
                            Rectangle {
                                anchors.centerIn: parent
                                width: 100; height: 28; radius: 4
                                property var val: diag.plcValues[modelData.id]
                                property bool hasV: val !== undefined && val !== null
                                property bool isBool: typeof val === "boolean"
                                color: !hasV               ? "#475569"
                                     : isBool && val       ? pal.success
                                     : isBool && !val      ? "#334155"
                                     :                       pal.accent
                                Text {
                                    anchors.centerIn: parent
                                    text: !parent.hasV ? "—"
                                        : parent.isBool ? (parent.val ? "TRUE" : "false")
                                        : String(parent.val)
                                    font.pixelSize: 12; font.bold: true
                                    font.family: "monospace"
                                    color: "#ffffff"
                                }
                            }
                        }

                        // Action button (write toggle for bools)
                        Item {
                            width: 100; height: parent.height
                            Rectangle {
                                visible: modelData.writable
                                anchors.centerIn: parent
                                width: 80; height: 28; radius: 4
                                color: setMa.pressed ? Qt.darker(pal.warning, 1.2) : pal.warning
                                Text {
                                    anchors.centerIn: parent
                                    text: "toggle"
                                    font.pixelSize: 11; font.bold: true
                                    color: "#ffffff"
                                }
                                MouseArea {
                                    id: setMa
                                    anchors.fill: parent
                                    onClicked: {
                                        var v = diag.plcValues[modelData.id]
                                        diag.plcWriteNode(modelData.id,
                                            (typeof v === "boolean") ? !v : v)
                                    }
                                }
                            }
                            Text {
                                visible: !modelData.writable
                                anchors.centerIn: parent
                                text: "read-only"
                                font.pixelSize: 10; color: pal.textDim
                            }
                        }

                        // Remove button
                        Item {
                            width: 60; height: parent.height
                            Rectangle {
                                anchors.centerIn: parent
                                width: 32; height: 28; radius: 4
                                color: rmMa.pressed ? pal.danger : "transparent"
                                border.color: pal.cardEdge; border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    font.pixelSize: 13; color: pal.textSub
                                }
                                MouseArea {
                                    id: rmMa
                                    anchors.fill: parent
                                    onClicked: {
                                        var copy = page.nodes.slice()
                                        copy.splice(index, 1)
                                        page.nodes = copy
                                        page.logAppend("removed " + modelData.label)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1; color: pal.cardEdge
                    }
                }
            }
        }

        // ─ RIGHT: add-node form + event log ─
        Rectangle {
            id: sidePanel
            anchors.left: tablePanel.right; anchors.leftMargin: 12
            anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.right: parent.right
            color: pal.cardBg
            border.color: pal.cardEdge; border.width: 1
            radius: 6

            // Add-node form
            Column {
                id: addForm
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 6

                Text {
                    text: "ADD NODE"
                    font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.8
                    color: pal.textDim
                }
                Text {
                    text: "Label (display name)"
                    font.pixelSize: 10; color: pal.textDim
                }
                Rectangle {
                    width: parent.width; height: 32; radius: 4
                    color: pal.rowOdd
                    border.color: lblField.activeFocus ? pal.accent : pal.cardEdge
                    border.width: 1
                    TextInput {
                        id: lblField
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        font.pixelSize: 12; color: pal.text
                        selectByMouse: true; clip: true
                    }
                }

                Text {
                    text: "Node ID (e.g. ns=4;s=|var|MAT LC-C07.Application.GVL.MyVar)"
                    font.pixelSize: 10; color: pal.textDim
                    wrapMode: Text.Wrap
                    width: parent.width
                }
                Rectangle {
                    width: parent.width; height: 32; radius: 4
                    color: pal.rowOdd
                    border.color: idField.activeFocus ? pal.accent : pal.cardEdge
                    border.width: 1
                    TextInput {
                        id: idField
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        font.pixelSize: 11; font.family: "monospace"; color: pal.text
                        selectByMouse: true; clip: true
                    }
                }

                Row {
                    spacing: 8
                    Rectangle {
                        width: 64; height: 28; radius: 4
                        color: writableBox.checked ? pal.accent : pal.cardEdge
                        Text { anchors.centerIn: parent; text: "writable"; font.pixelSize: 10; color: pal.text }
                        MouseArea { anchors.fill: parent; onClicked: writableBox.checked = !writableBox.checked }
                        property bool checked: false
                        id: writableBox
                    }
                    Rectangle {
                        width: 100; height: 28; radius: 4
                        color: addMa.pressed ? Qt.darker(pal.success, 1.2) : pal.success
                        Text { anchors.centerIn: parent; text: "+ Subscribe"; font.pixelSize: 11; font.bold: true; color: "#ffffff" }
                        MouseArea {
                            id: addMa
                            anchors.fill: parent
                            onClicked: {
                                if (idField.text.length === 0) return
                                var entry = {
                                    id: idField.text,
                                    label: lblField.text.length > 0 ? lblField.text : idField.text,
                                    writable: writableBox.checked,
                                    kind: "any"
                                }
                                var copy = page.nodes.slice()
                                copy.push(entry)
                                page.nodes = copy
                                diag.plcSubscribeNode(entry.id)
                                page.logAppend("subscribed " + entry.label)
                                idField.text = ""
                                lblField.text = ""
                            }
                        }
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: addForm.bottom; anchors.topMargin: 12
                height: 1; color: pal.cardEdge
            }

            // Event log
            Text {
                id: logTitle
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: addForm.bottom; anchors.topMargin: 20
                anchors.leftMargin: 12; anchors.rightMargin: 12
                text: "EVENT LOG"
                font.pixelSize: 11; font.bold: true; font.letterSpacing: 0.8
                color: pal.textDim
            }
            ListView {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: logTitle.bottom; anchors.topMargin: 6
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12; anchors.rightMargin: 12
                anchors.bottomMargin: 12
                clip: true
                model: page.log
                delegate: Text {
                    width: parent.width
                    text: modelData
                    font.pixelSize: 10; font.family: "monospace"
                    color: pal.textSub
                    elide: Text.ElideRight
                }
            }
        }
    }
}
