#include "hwio.h"
#include "led_controller.h"

#include <QDebug>

namespace smbq6r {

HwIo& HwIo::instance()
{
    static HwIo io;
    return io;
}

HwIo::HwIo()
    : led_(std::make_unique<LedController>())
{
    qInfo() << "HwIo: initialised"
            << "leds=" << (led_->isReady() ? "ok" : "FAIL");
}

HwIo::~HwIo() = default;

} // namespace smbq6r
