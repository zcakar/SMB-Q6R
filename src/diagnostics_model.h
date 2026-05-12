#pragma once

#include <QObject>

namespace smbq6r {

// QML-facing facade over HwIo. Holds no hardware state of its own — every
// property reads from the singleton on demand, every invokable forwards to
// it. This keeps the QML/C++ boundary thin and lets Phase 4+ swap the
// hardware backend (e.g. for a simulator) without touching the QML.
class DiagnosticsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ledMask     READ ledMask     NOTIFY ledChanged)
    Q_PROPERTY(bool ledReady   READ ledReady    NOTIFY ledChanged)

public:
    explicit DiagnosticsModel(QObject* parent = nullptr);

    int  ledMask()  const;
    bool ledReady() const;

public slots:
    // Toggle LED at the given port (0..4) on or off.
    void setLed(int port, bool on);

    // Turn every LED off.
    void allLedsOff();

signals:
    void ledChanged();
};

} // namespace smbq6r
