#pragma once

#include <memory>

namespace smbq6r {

class LedController;
// Forward declarations for future iterations:
// class BuzzerController;
// class SwitchMonitor;
// class BacklightController;
// class SystemInfo;

// Singleton facade that owns every kernel-device file descriptor for the
// HN00-09Q6 pendant. The vendor's Lavichip drivers (/dev/leds, /dev/pwm,
// /dev/buttons, /dev/buttonstop) tolerate exactly one open() per process —
// reopening corrupts internal driver state. Centralising ownership here
// guarantees that invariant.
//
// Construction is lazy and one-shot via instance(). Order of subcomponent
// construction is fixed (LED → buzzer → switches → backlight → system info)
// so a subsystem can depend on those listed before it.
class HwIo
{
public:
    // First call creates the singleton; subsequent calls return the same
    // instance. Not thread-safe for first call — invoke from the main
    // thread before any other thread touches HwIo.
    static HwIo& instance();

    LedController& leds() { return *led_; }

    // Disallow copy/move.
    HwIo(const HwIo&) = delete;
    HwIo& operator=(const HwIo&) = delete;

private:
    HwIo();
    ~HwIo();

    std::unique_ptr<LedController> led_;
};

} // namespace smbq6r
