#include "hwio.h"
#include "led_controller.h"
#include "switch_monitor.h"
#include "buzzer_controller.h"
#include "backlight_controller.h"
#include "matrix_keys_monitor.h"

#include <QDebug>

namespace smbq6r {

HwIo& HwIo::instance()
{
    static HwIo io;
    return io;
}

HwIo::HwIo()
    : led_      (std::make_unique<LedController>())
    , switch_   (std::make_unique<SwitchMonitor>())
    , buzzer_   (std::make_unique<BuzzerController>())
    , backlight_(std::make_unique<BacklightController>())
    , matrix_   (std::make_unique<MatrixKeysMonitor>())
{
    qInfo() << "HwIo: initialised"
            << "leds="      << (led_->isReady()       ? "ok" : "FAIL")
            << "switches="  << (switch_->isReady()    ? "ok" : "FAIL")
            << "buzzer="    << (buzzer_->isReady()    ? "ok" : "FAIL")
            << "backlight=" << (backlight_->isReady() ? "ok" : "FAIL")
            << "keys="      << (matrix_->isReady()    ? "ok" : "FAIL");
}

HwIo::~HwIo() = default;

} // namespace smbq6r
