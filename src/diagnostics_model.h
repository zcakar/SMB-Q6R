#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace smbq6r {

// QML-facing facade exposing the full diagnostic state of the pendant.
// Each property reads through to the matching HwIo subsystem on demand;
// no state is duplicated here. Signals are re-fired from the matching
// subsystem so QML bindings stay in sync.
class DiagnosticsModel : public QObject
{
    Q_OBJECT

    // LEDs
    Q_PROPERTY(int     ledMask     READ ledMask     NOTIFY ledChanged)
    Q_PROPERTY(bool    ledReady    READ ledReady    CONSTANT)

    // Switches
    Q_PROPERTY(QString mode        READ mode        NOTIFY switchChanged)
    Q_PROPERTY(QString modeByte    READ modeByte    NOTIFY switchChanged)
    Q_PROPERTY(bool    enableS1    READ enableS1    NOTIFY switchChanged)
    Q_PROPERTY(bool    enableS2    READ enableS2    NOTIFY switchChanged)
    Q_PROPERTY(QString enableByte  READ enableByte  NOTIFY switchChanged)
    Q_PROPERTY(bool    switchReady READ switchReady CONSTANT)

    // Buzzer
    Q_PROPERTY(bool    buzzerReady READ buzzerReady CONSTANT)

    // Backlight
    Q_PROPERTY(int     backlight   READ backlight   WRITE setBacklight  NOTIFY backlightChanged)
    Q_PROPERTY(int     backlightMax READ backlightMax CONSTANT)
    Q_PROPERTY(bool    backlightReady READ backlightReady CONSTANT)

    // Matrix keys
    Q_PROPERTY(int         lastKeyCode    READ lastKeyCode    NOTIFY keyEvent)
    Q_PROPERTY(QString     lastKeyName    READ lastKeyName    NOTIFY keyEvent)
    Q_PROPERTY(bool        lastKeyPressed READ lastKeyPressed NOTIFY keyEvent)
    Q_PROPERTY(QStringList keyHistory     READ keyHistory     NOTIFY keyEvent)
    Q_PROPERTY(bool        keysReady      READ keysReady      CONSTANT)

    // Hardcoded HN00-09Q6 jog-button map: 14 codes in screen order
    //   [J1−, J1+, J2−, J2+, J3−, J3+, ..., J7−, J7+]
    // Live-captured 2026-05-13. Constant — no user mapping required.
    Q_PROPERTY(QVariantList defaultKeyMap READ defaultKeyMap  CONSTANT)

    // True when ANY subsystem fell back to its in-memory simulator (typical
    // when running on the development host with no Lavichip devices present).
    Q_PROPERTY(bool        simulatorMode  READ simulatorMode  CONSTANT)

    // OPC UA / PLC link
    Q_PROPERTY(QString     plcState       READ plcState       NOTIFY plcStateChanged)
    Q_PROPERTY(QString     plcServerUrl   READ plcServerUrl   NOTIFY plcStateChanged)
    Q_PROPERTY(QString     plcLastError   READ plcLastError   NOTIFY plcStateChanged)
    // Live snapshot of every subscribed node's last known value. Keyed by
    // OPC UA node id string ("ns=4;s=|var|MAT LC-C07.Application.GVL.Enable").
    // Emits plcValuesChanged on every update so QML bindings refresh.
    Q_PROPERTY(QVariantMap plcValues      READ plcValues      NOTIFY plcValuesChanged)

public:
    explicit DiagnosticsModel(QObject* parent = nullptr);

    // LEDs
    int  ledMask()  const;
    bool ledReady() const;

    // Switches
    QString mode() const;
    QString modeByte() const;
    bool enableS1() const;
    bool enableS2() const;
    QString enableByte() const;
    bool switchReady() const;

    // Buzzer
    bool buzzerReady() const;

    // Backlight
    int backlight() const;
    int backlightMax() const;
    bool backlightReady() const;
    void setBacklight(int v);

    // Matrix keys
    int lastKeyCode() const;
    QString lastKeyName() const;
    bool lastKeyPressed() const;
    QStringList keyHistory() const;
    bool keysReady() const;
    bool simulatorMode();
    QVariantList defaultKeyMap() const;

    // OPC UA
    QString plcState() const;
    QString plcServerUrl() const;
    QString plcLastError() const;
    QVariantMap plcValues() const { return plcValues_; }

public slots:
    void setLed(int port, bool on);
    void allLedsOff();
    void beep(int ms);
    void holdBuzzer(bool on);

    // Simulator drivers — invoked from QML when running on the host.
    void simSetMode(const QString& name);        // "Auto" / "Manual" / "Stop"
    void simSetDeadman(bool s1, bool s2);
    void simKeyEvent(int code, bool pressed);

    // PLC / OPC UA
    void plcConnect(const QString& endpointUrl);
    void plcDisconnect();
    void plcReadNode(const QString& nodeId);
    void plcSubscribeNode(const QString& nodeId);
    void plcWriteNode(const QString& nodeId, const QVariant& value);
    void plcBrowse();

signals:
    void ledChanged();
    void switchChanged();
    void backlightChanged();
    void keyEvent();
    void plcStateChanged();
    void plcValueRead(QString nodeId, QVariant value);
    void plcValueChanged(QString nodeId, QVariant value);
    void plcReadFailed(QString nodeId, QString reason);
    void plcNodeDiscovered(QString nodeId, QString browseName, int depth);
    void plcValuesChanged();
    void plcWriteSucceeded(QString nodeId);
    void plcWriteFailed(QString nodeId, QString reason);

private:
    QVariantMap plcValues_;
};

} // namespace smbq6r
