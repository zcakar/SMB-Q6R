// SMB-Q6R Teach Pendant — entry point.
//
// Phase 1 / Iteration A: Diagnostics demo wired to the HwIo singleton.
// Touch the LED buttons to drive /dev/leds; further peripherals are added
// in subsequent iterations.

#include "diagnostics_model.h"

#include <QGuiApplication>
#include <QFont>
#include <QFontDatabase>
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

    // The Lavichip image's Qt 5.15.10 build doesn't pick up system fonts
    // through fontconfig the way most desktop builds do — even though
    // /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf is installed,
    // QFontDatabase().families() doesn't list it, and the default font
    // ends up being a CJK face that's missing common Latin Unicode glyphs
    // (U+2212 minus, U+2014 em-dash, ↻ ▼ ▲ → · etc.) — they render as
    // tofu rectangles. Bundle DejaVu Sans in the QRC and register it as an
    // application font so we are independent of the system font setup.
    {
        const int regular = QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/DejaVuSans.ttf"));
        QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/DejaVuSans-Bold.ttf"));
        QString family;
        if (regular >= 0) {
            const QStringList fams = QFontDatabase::applicationFontFamilies(regular);
            if (!fams.isEmpty()) family = fams.first();
        }
        if (family.isEmpty()) family = QStringLiteral("Sans");
        QFont base(family);
        base.setStyleStrategy(QFont::PreferAntialias);
        QGuiApplication::setFont(base);
        qInfo() << "Application font:" << family;
    }

    qInfo() << "SMB-Q6R" << QGuiApplication::applicationVersion()
            << "on Qt" << qVersion()
            << "/" << QGuiApplication::platformName();

    smbq6r::DiagnosticsModel model;
    const bool isSim = model.simulatorMode();

    QQuickView view;
    view.setTitle(isSim
        ? QStringLiteral("SMB-Q6R · Host Simulator")
        : QStringLiteral("SMB-Q6R Hardware Mapping"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);

    // NOTE: do NOT name this context property "model" — that name is the
    // delegate-scope reserved word inside Repeater/ListView, which would
    // shadow ours and break every binding inside a delegate.
    view.rootContext()->setContextProperty(QStringLiteral("diag"), &model);

    // PlcSymbols singleton is wired through a qmldir at
    // qrc:/qml/generated/qmldir (produced by scripts/regen-symbols.py),
    // and PlcPage.qml reaches it via ``import "./generated"``. Going
    // through qmldir (rather than qmlRegisterSingletonType) means qmlls
    // sees the type during static analysis too, so the IDE doesn't flag
    // ``PlcSymbols.globalVars.GVL.Enable`` as an unqualified access.

    // On the actual pendant we go straight into the touchscreen UI and
    // run fullscreen as a kiosk. On a dev host with no Lavichip devices
    // we wrap Main.qml inside SimulatorBezel.qml, which draws the
    // physical pendant body (mode switch, E-stop, 14 jog buttons, dead-
    // man) around the screen area so it can all be exercised by mouse.
    if (isSim) {
        view.setSource(QUrl(QStringLiteral("qrc:/qml/SimulatorBezel.qml")));
    } else {
        view.setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    }

    if (view.status() == QQuickView::Error) {
        for (const QQmlError& err : view.errors()) {
            qCritical() << "QML error:" << err.toString();
        }
        return -1;
    }

    if (isSim) {
        view.resize(1600, 1000);
        view.show();
    } else {
        view.resize(1280, 800);
        // Fullscreen kiosk: just showFullScreen() — gives a borderless top
        // window without invoking override-redirect (FramelessWindowHint on
        // XFCE bypasses the WM and renders the window invisible behind the
        // compositor). To exit during development:  killall smb_q6r over SSH.
        view.showFullScreen();
    }
    view.requestActivate();
    view.raise();
    return app.exec();
}
