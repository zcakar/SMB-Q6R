// SMB-Q6R Teach Pendant — entry point.
//
// Phase 1: Diagnostics demo. The HN00-09Q6 system Qt 5.12.8 only ships
// qml-module-qtquick2; QtQuick.Window / Controls / Layouts QML modules are
// absent. We therefore drive a QQuickView directly from C++ (which provides
// its own QWindow) and write every UI primitive against bare QtQuick 2.

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlError>
#include <QSurfaceFormat>
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

    QQuickView view;
    view.setTitle(QStringLiteral("SMB-Q6R Diagnostics"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 800);
    view.setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (view.status() == QQuickView::Error) {
        for (const QQmlError& err : view.errors()) {
            qCritical() << "QML error:" << err.toString();
        }
        return -1;
    }

    view.show();
    return app.exec();
}
