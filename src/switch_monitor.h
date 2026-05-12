#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
class QTimer;
QT_END_NAMESPACE

namespace smbq6r {

// Reads /dev/buttons (3-position key switch: Auto / Manual / Stop)
// and /dev/buttonstop (3-stage enable switch: S1 / S2 contacts).
//
// Each device exposes an 8-char ASCII bitmap: a read returns 8 bytes where
// each byte is '0' or '1' indicating one switch bit.
//
//   /dev/buttons       bit 3 = Auto, bit 4 = Manual, bit 5 = Stop
//   /dev/buttonstop    bit 6 = S2,   bit 7 = S1
//
// (Vendor manual HT0804 §3.3 / §3.7; the byte order matches what we read
// with read(fd, buf, 8); buf[7] is the high-bit S1 ASCII char.)
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

    bool isReady() const { return buttonsFd_ >= 0 && buttonstopFd_ >= 0; }

    Mode mode() const { return mode_; }
    bool enableS1() const { return s1_; }
    bool enableS2() const { return s2_; }
    QString modeByte()  const { return modeByte_; }
    QString enableByte() const { return enableByte_; }

signals:
    void modeChanged();
    void enableChanged();

private:
    void onButtonsReadable();
    void onButtonstopReadable();

    int buttonsFd_ = -1;
    int buttonstopFd_ = -1;
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
