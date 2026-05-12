#pragma once

#include <QString>

namespace smbq6r {

// Wraps /dev/pwm. The Lavichip buzzer driver uses ioctl(fd, cmd, val):
//   cmd=0, val=*  -> mute / cancel
//   cmd=1, val=1  -> turn on continuously
//   cmd=1, val=0  -> turn off
//   cmd=3, val=ms -> beep for `ms` milliseconds (min 10)
//
// The device file may NOT be opened repeatedly per process (vendor manual
// §3.4 highlights this), so this class enforces single-instance ownership
// via HwIo.
class BuzzerController
{
public:
    BuzzerController();
    ~BuzzerController();

    BuzzerController(const BuzzerController&) = delete;
    BuzzerController& operator=(const BuzzerController&) = delete;

    bool isReady() const { return fd_ >= 0; }
    QString errorString() const { return error_; }

    // Beep for `ms` milliseconds (min clamped to 10).
    bool beep(int ms);

    // Continuous on / off.
    bool setHold(bool on);

    // Stop any currently active sound.
    bool silence();

private:
    int fd_ = -1;
    QString error_;
};

} // namespace smbq6r
