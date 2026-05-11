// Phase 1 smoke-test root. Only QtQuick 2 primitives — no Controls / Layouts
// modules required, matching the HN00-09Q6 device package set.

import QtQuick 2.12

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#0c1322"

    // ----- title -----
    Text {
        id: title
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 120
        text: "SMB-Q6R"
        font.pixelSize: 80
        font.bold: true
        color: "#7CFC00"
    }

    Text {
        id: subtitle
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: title.bottom
        anchors.topMargin: 24
        text: "Cross-compile pipeline OK — Qt " + Qt.application.version
        font.pixelSize: 22
        color: "#C0C0C0"
    }

    // ----- live clock -----
    Text {
        id: clock
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: subtitle.bottom
        anchors.topMargin: 36
        font.pixelSize: 36
        color: "#FFD700"
        font.family: "monospace"
    }

    // ----- press indicator -----
    Rectangle {
        id: btn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: clock.bottom
        anchors.topMargin: 60
        width: 360
        height: 80
        radius: 8
        border.color: "#0D47A1"
        border.width: 2
        color: ma.pressed ? "#1565C0" : "#1976D2"

        property int pressCount: 0

        Text {
            anchors.centerIn: parent
            // pressCount is a property of `btn` (parent); qualify so QML resolves
            // it in btn's scope rather than this Text's own (empty) property table.
            text: "Press counter: " + btn.pressCount
            font.pixelSize: 24
            color: "white"
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            onClicked: { btn.pressCount += 1; console.log("press", btn.pressCount) }
        }
    }

    // ----- footer status -----
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Phase 1 / step 1 of 4 — smoke test"
        font.pixelSize: 14
        color: "#6A7A93"
    }

    // ----- 1 Hz clock tick -----
    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            var d = new Date()
            var pad = function(n) { return (n < 10 ? "0" : "") + n }
            clock.text = d.getFullYear() + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate())
                + "  " + pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds())
        }
    }
}
