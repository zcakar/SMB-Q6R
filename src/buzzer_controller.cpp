#include "buzzer_controller.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <QDebug>

namespace smbq6r {

namespace {
constexpr const char* kDevice = "/dev/pwm";
constexpr int kCmdMute    = 0;
constexpr int kCmdHold    = 1;
constexpr int kCmdTimed   = 3;
constexpr int kMinDuration = 10;  // ms, per vendor manual
} // namespace

BuzzerController::BuzzerController()
{
    fd_ = ::open(kDevice, O_RDWR);
    if (fd_ < 0) {
        error_ = QStringLiteral("%1: %2").arg(kDevice, ::strerror(errno));
        qWarning() << "BuzzerController: open failed —" << error_;
        return;
    }
    qInfo() << "BuzzerController: opened" << kDevice;
}

BuzzerController::~BuzzerController()
{
    if (fd_ >= 0) {
        silence();
        ::close(fd_);
    }
}

bool BuzzerController::beep(int ms)
{
    if (fd_ < 0) return false;
    if (ms < kMinDuration) ms = kMinDuration;
    if (::ioctl(fd_, kCmdTimed, ms) < 0) {
        qWarning() << "BuzzerController::beep(" << ms << ") failed:" << ::strerror(errno);
        return false;
    }
    return true;
}

bool BuzzerController::setHold(bool on)
{
    if (fd_ < 0) return false;
    if (::ioctl(fd_, kCmdHold, on ? 1 : 0) < 0) {
        qWarning() << "BuzzerController::setHold(" << on << ") failed:" << ::strerror(errno);
        return false;
    }
    return true;
}

bool BuzzerController::silence()
{
    if (fd_ < 0) return false;
    if (::ioctl(fd_, kCmdMute, 0) < 0) {
        // Some kernels reject cmd=0 with EINVAL when buzzer is already off.
        // Fall back to "hold off".
        if (::ioctl(fd_, kCmdHold, 0) < 0) {
            return false;
        }
    }
    return true;
}

} // namespace smbq6r
