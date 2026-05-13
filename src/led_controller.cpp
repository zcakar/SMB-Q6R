#include "led_controller.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <QDebug>

namespace smbq6r {

namespace {
constexpr const char* kDevice = "/dev/leds";

// Vendor convention: command number is the "state" (1=on, 0=off);
// argument is the port number.
constexpr int kCmdOff = 0;
constexpr int kCmdOn  = 1;
} // namespace

LedController::LedController()
{
    fd_ = ::open(kDevice, O_WRONLY);
    if (fd_ < 0) {
        // Device file is missing or unreadable — typical when running on the
        // development host. Fall back to an in-memory simulator that keeps
        // the mask and behaves identically from the UI's perspective.
        simulator_ = true;
        error_ = QStringLiteral("%1: %2 (simulator)").arg(kDevice, ::strerror(errno));
        qInfo() << "LedController: SIMULATOR mode —" << ::strerror(errno);
        return;
    }
    qInfo() << "LedController: opened" << kDevice << "fd=" << fd_;
}

LedController::~LedController()
{
    if (fd_ >= 0) {
        // Best-effort: leave LEDs in a known state at shutdown.
        allOff();
        ::close(fd_);
    }
}

bool LedController::set(int port, bool on)
{
    if (port < 0 || port > kMaxPort) {
        qWarning() << "LedController::set: port out of range" << port;
        return false;
    }

    if (!simulator_ && fd_ >= 0) {
        const int cmd = on ? kCmdOn : kCmdOff;
        if (::ioctl(fd_, cmd, port) < 0) {
            qWarning() << "LedController::set: ioctl failed for port" << port
                       << "—" << ::strerror(errno);
            return false;
        }
    }

    if (on) mask_ |=  (1 << port);
    else    mask_ &= ~(1 << port);
    return true;
}

void LedController::allOff()
{
    for (int p = 0; p <= kMaxPort; ++p) {
        set(p, false);
    }
}

} // namespace smbq6r
