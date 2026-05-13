#pragma once

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE

namespace smbq6r {

// Reads raw evdev events from /dev/input/event0 (the matrix keypad). We
// listen at the kernel level rather than through Qt's keyboard pipeline
// because:
//   * we want the *original* KEY_* code regardless of any X server / Qt
//     remapping done in /etc/profile.d/qt_env.sh;
//   * the active X session was started before our 'input' group fix took
//     effect, so X may not relay keypresses to Qt yet (until next relogin
//     or reboot).
class MatrixKeysMonitor : public QObject
{
    Q_OBJECT
public:
    explicit MatrixKeysMonitor(QObject* parent = nullptr);
    ~MatrixKeysMonitor() override;

    bool isReady() const { return fd_ >= 0 || simulator_; }
    bool isSimulator() const { return simulator_; }

    int     lastCode()    const { return lastCode_; }
    QString lastName()    const { return lastName_; }
    bool    lastPressed() const { return lastPressed_; }
    QStringList history() const { return history_; }

    // Inject a key event from the simulator (e.g. Qt keyboard event).
    void simulateKey(int code, bool pressed);

signals:
    void keyEvent();

private:
    void onReadable();

    int fd_ = -1;
    bool simulator_ = false;
    QSocketNotifier* notifier_ = nullptr;

    int     lastCode_    = 0;
    QString lastName_;
    bool    lastPressed_ = false;
    QStringList history_;
};

} // namespace smbq6r
