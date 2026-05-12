#pragma once

#include <QString>

namespace smbq6r {

// Wraps /dev/leds. The Lavichip LED driver accepts ioctl(fd, state, port)
// where state=1 lights the LED and state=0 turns it off. Port indices map
// to physical LEDs in a model-dependent way (HT0803/HT0804/HN00-09Q6 all
// differ). On the HN00-09Q6 the physical mapping is determined empirically
// during Phase 1 and recorded in .ai/ENGINEERING_LOG.md.
//
// Critical: the device file must be opened exactly once per process — the
// vendor manual notes that repeated open() calls corrupt driver state. This
// class therefore disables copy/move and is owned by HwIo as a singleton.
class LedController
{
public:
    // HN00-09Q6 exposes 4 LED ports (0..3). Port 4 returns EINVAL from
    // the driver and is therefore excluded from iteration.
    static constexpr int kMaxPort = 3;
    static constexpr int kPortCount = 4;

    LedController();
    ~LedController();

    LedController(const LedController&) = delete;
    LedController& operator=(const LedController&) = delete;

    // True if /dev/leds was opened successfully.
    bool isReady() const { return fd_ >= 0; }
    QString errorString() const { return error_; }

    // Turn the given LED on or off. Returns false if the ioctl failed or
    // the port is out of range or the device is not ready. The internal
    // mask is updated regardless of physical outcome — we only know what
    // we asked for; the driver doesn't expose readback.
    bool set(int port, bool on);

    // Turn every port off (best-effort, ignores errors).
    void allOff();

    // Bitmask of ports currently believed to be on (bit N = port N).
    int activeMask() const { return mask_; }

private:
    int fd_ = -1;
    int mask_ = 0;
    QString error_;
};

} // namespace smbq6r
