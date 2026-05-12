#include "diagnostics_model.h"

#include "hwio.h"
#include "led_controller.h"

namespace smbq6r {

DiagnosticsModel::DiagnosticsModel(QObject* parent)
    : QObject(parent)
{
}

int DiagnosticsModel::ledMask() const
{
    return HwIo::instance().leds().activeMask();
}

bool DiagnosticsModel::ledReady() const
{
    return HwIo::instance().leds().isReady();
}

void DiagnosticsModel::setLed(int port, bool on)
{
    if (HwIo::instance().leds().set(port, on)) {
        emit ledChanged();
    }
}

void DiagnosticsModel::allLedsOff()
{
    HwIo::instance().leds().allOff();
    emit ledChanged();
}

} // namespace smbq6r
