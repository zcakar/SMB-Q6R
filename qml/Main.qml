// Phase 1 smoke-test window. Will be replaced by the full 7-tab diagnostic
// shell once the cross-compile pipeline is verified.

// QML imports pinned to Qt 5.12 (device system Qt). Avoid 2.15-only API.
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    visible: true
    title: "SMB-Q6R Diagnostics (Phase 1)"

    background: Rectangle { color: "#0c1322" }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 24

        Label {
            text: "SMB-Q6R"
            font.pixelSize: 64
            color: "#7CFC00"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Cross-compile pipeline OK ✓"
            font.pixelSize: 20
            color: "#C0C0C0"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            id: clock
            text: Qt.formatDateTime(new Date(), "yyyy-MM-dd  HH:mm:ss")
            font.pixelSize: 28
            color: "#FFD700"
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Tıkla — pipeline kanıtı"
            Layout.alignment: Qt.AlignHCenter
            onClicked: console.log("button pressed at", new Date())
        }
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: clock.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd  HH:mm:ss")
    }
}
