#include "PickerWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("smb-q6r-symbol-picker"));
    QApplication::setApplicationVersion(QStringLiteral("0.3.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "SMB-Q6R PLC symbol picker — Live PLC or CodeSys Symbol XML,"
        " pick OPC UA tags, emit symbols.json for codegen."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption liveOpt(
        QStringList{QStringLiteral("l"), QStringLiteral("live")},
        QStringLiteral("OPC UA endpoint for the Live PLC tab"),
        QStringLiteral("endpoint"),
        QStringLiteral("opc.tcp://192.168.0.2:4840"));
    QCommandLineOption xmlOpt(
        QStringList{QStringLiteral("x"), QStringLiteral("xml")},
        QStringLiteral("Path to a CodeSys Symbol Configuration XML"),
        QStringLiteral("path"));
    QCommandLineOption devOpt(
        QStringList{QStringLiteral("d"), QStringLiteral("device")},
        QStringLiteral("Override the device name baked into the XML"),
        QStringLiteral("name"));
    QCommandLineOption outOpt(
        QStringList{QStringLiteral("o"), QStringLiteral("out")},
        QStringLiteral("Where to write the curated manifest"),
        QStringLiteral("path"), QStringLiteral("symbols/symbols.json"));
    parser.addOption(liveOpt);
    parser.addOption(xmlOpt);
    parser.addOption(devOpt);
    parser.addOption(outOpt);
    parser.process(app);

    const QString endpoint   = parser.value(liveOpt);
    const QString xmlPath    = parser.value(xmlOpt);
    const QString deviceName = parser.value(devOpt);
    const QString symbolsOut = parser.value(outOpt);

    PickerWindow win(xmlPath, deviceName, endpoint, symbolsOut);
    win.show();
    return app.exec();
}
