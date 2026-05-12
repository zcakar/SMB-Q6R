#pragma once

#include <memory>

namespace smbq6r {

class LedController;
class SwitchMonitor;
class BuzzerController;
class BacklightController;
class MatrixKeysMonitor;

// Singleton facade owning every kernel-device file descriptor. The vendor
// Lavichip drivers tolerate exactly one open() per process; centralising
// ownership here enforces that invariant.
//
// Initialisation order is fixed: LED → switches → buzzer → backlight →
// matrix keys → wheel. Subsystems may rely on those declared before them.
class HwIo
{
public:
    static HwIo& instance();

    LedController&       leds()       { return *led_; }
    SwitchMonitor&       switches()   { return *switch_; }
    BuzzerController&    buzzer()     { return *buzzer_; }
    BacklightController& backlight()  { return *backlight_; }
    MatrixKeysMonitor&   matrixKeys() { return *matrix_; }

    HwIo(const HwIo&) = delete;
    HwIo& operator=(const HwIo&) = delete;

private:
    HwIo();
    ~HwIo();

    std::unique_ptr<LedController>       led_;
    std::unique_ptr<SwitchMonitor>       switch_;
    std::unique_ptr<BuzzerController>    buzzer_;
    std::unique_ptr<BacklightController> backlight_;
    std::unique_ptr<MatrixKeysMonitor>   matrix_;
};

} // namespace smbq6r
