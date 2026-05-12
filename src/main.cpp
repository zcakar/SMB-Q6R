// SMB-Q6R Teach Pendant — entry point.
//
// Phase 1 / Iteration A: Diagnostics demo wired to the HwIo singleton.
// Touch the LED buttons to drive /dev/leds; further peripherals are added
// in subsequent iterations.

#include "diagnostics_model.h"

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlError>
#include <QDebug>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("SMB-Q6R");
    QGuiApplication::setOrganizationName("smbq6r");
    QGuiApplication::setApplicationVersion("0.1.0");

    qInfo() << "SMB-Q6R" << QGuiApplication::applicationVersion()
            << "on Qt" << qVersion()
            << "/" << QGuiApplication::platformName();

    smbq6r::DiagnosticsModel model;

    QQuickView view;
    view.setTitle(QStringLiteral("SMB-Q6R Hardware Mapping"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 800);

    // NOTE: do NOT name this context property "model" — that name is the
    // delegate-scope reserved word inside Repeater/ListView, which would
    // shadow ours and break every binding inside a delegate.
    view.rootContext()->setContextProperty(QStringLiteral("diag"), &model);
    view.setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (view.status() == QQuickView::Error) {
        for (const QQmlError& err : view.errors()) {
            qCritical() << "QML error:" << err.toString();
        }
        return -1;
    }

    // Fullscreen kiosk: just showFullScreen() — gives a borderless top
    // window without invoking override-redirect (FramelessWindowHint on
    // XFCE bypasses the WM and renders the window invisible behind the
    // compositor). To exit during development:  killall smb_q6r over SSH.
    view.showFullScreen();
    view.requestActivate();
    view.raise();
    return app.exec();
}
