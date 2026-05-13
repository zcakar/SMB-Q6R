#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
class QTimer;
QT_END_NAMESPACE

namespace smbq6r {

// Reads /dev/buttons (3-position key switch: Auto / Manual / Stop)
// and /dev/buttonstop (2-stage enable switch: S1 / S2 contacts).
//
// Each device exposes an 8-char ASCII bitmap: a read returns 8 bytes where
// each byte is '0' or '1' indicating one switch bit. HN00-09Q6 positions
// were verified empirically (2026-05-12, see *.cpp comments) and differ
// from the vendor HT0803/HT0804 manuals:
//
//   /dev/buttons       buf[3]=Manual, buf[4]=Auto, buf[5]=Stop
//   /dev/buttonstop    buf[0]=S1,     buf[1]=S2
//
// The driver appears to push a new 8-byte frame every time any of the
// observed bits changes; QSocketNotifier::Read fires when data is waiting.
class SwitchMonitor : public QObject
{
    Q_OBJECT
public:
    enum class Mode { None, Auto, Manual, Stop };
    Q_ENUM(Mode)

    explicit SwitchMonitor(QObject* parent = nullptr);
    ~SwitchMonitor() override;

    bool isReady() const { return (buttonsFd_ >= 0 && buttonstopFd_ >= 0) || simulator_; }
    bool isSimulator() const { return simulator_; }

    Mode mode() const { return mode_; }
    bool enableS1() const { return s1_; }
    bool enableS2() const { return s2_; }
    QString modeByte()  const { return modeByte_; }
    QString enableByte() const { return enableByte_; }

    // Simulator drivers — used by DiagnosticsModel when no real hardware.
    void simulateMode(Mode m);
    void simulateDeadman(bool s1, bool s2);

signals:
    void modeChanged();
    void enableChanged();

private:
    void onButtonsReadable();
    void onButtonstopReadable();

    int buttonsFd_ = -1;
    int buttonstopFd_ = -1;
    bool simulator_ = false;
    QSocketNotifier* buttonsNotifier_ = nullptr;
    QSocketNotifier* buttonstopNotifier_ = nullptr;
    QTimer* primeTimer_ = nullptr;  // briefly polls at startup until first frame

    Mode mode_ = Mode::None;
    bool s1_ = false;
    bool s2_ = false;
    QString modeByte_   = QStringLiteral("--------");
    QString enableByte_ = QStringLiteral("--------");
};

} // namespace smbq6r
