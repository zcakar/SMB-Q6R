// SMB-Q6R Teach Pendant — entry point.
//
// Phase 1: Diagnostics demo. This file currently launches a minimal QML
// window to exercise the cross-compile + deploy pipeline. The full
// DiagnosticsModel + HwIo wiring is added incrementally.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include <QDebug>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("SMB-Q6R");
    QGuiApplication::setOrganizationName("smbq6r");
    QGuiApplication::setApplicationVersion("0.1.0");

    // Quick Controls 2 style — "Basic" is the lightweight default.
    QQuickStyle::setStyle("Basic");

    qInfo() << "SMB-Q6R" << QGuiApplication::applicationVersion()
            << "starting on Qt" << qVersion();

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/qml/Main.qml"));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load Main.qml";
        return -1;
    }

    return app.exec();
}
