#pragma once

#include <QString>

namespace smbq6r {

// Wraps /sys/class/backlight/<dev>/brightness. The device file is plain
// text — ASCII integer 0..max. On HN00-09Q6 the max is 100 and the path
// is /sys/class/backlight/backlight/brightness. We re-resolve max from
// `max_brightness` in the same dir so the controller copes if a future
// kernel changes the units.
class BacklightController
{
public:
    BacklightController();

    bool isReady() const { return ready_ || simulator_; }
    bool isSimulator() const { return simulator_; }
    QString errorString() const { return error_; }

    // Returns 0..maxValue() on success, -1 on failure.
    int value() const;

    // Set brightness. Returns true on success.
    bool setValue(int v);

    int maxValue() const { return max_; }

private:
    bool ready_ = false;
    bool simulator_ = false;
    mutable int simValue_ = 75;
    int  max_   = 100;
    QString brightnessPath_;
    QString error_;
};

} // namespace smbq6r
