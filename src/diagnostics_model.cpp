#include "diagnostics_model.h"

#include "hwio.h"
#include "led_controller.h"
#include "switch_monitor.h"
#include "buzzer_controller.h"
#include "backlight_controller.h"
#include "matrix_keys_monitor.h"

namespace smbq6r {

DiagnosticsModel::DiagnosticsModel(QObject* parent)
    : QObject(parent)
{
    // Fan signals out of the C++ subsystems into our own NOTIFY signals so
    // every QML binding listens to a single object.
    auto& io = HwIo::instance();
    connect(&io.switches(),   &SwitchMonitor::modeChanged,
            this, &DiagnosticsModel::switchChanged);
    connect(&io.switches(),   &SwitchMonitor::enableChanged,
            this, &DiagnosticsModel::switchChanged);
    connect(&io.matrixKeys(), &MatrixKeysMonitor::keyEvent,
            this, &DiagnosticsModel::keyEvent);
}

int DiagnosticsModel::ledMask()  const { return HwIo::instance().leds().activeMask(); }
bool DiagnosticsModel::ledReady() const { return HwIo::instance().leds().isReady(); }

QString DiagnosticsModel::mode() const
{
    switch (HwIo::instance().switches().mode()) {
        case SwitchMonitor::Mode::Auto:   return QStringLiteral("Auto");
        case SwitchMonitor::Mode::Manual: return QStringLiteral("Manual");
        case SwitchMonitor::Mode::Stop:   return QStringLiteral("Stop");
        default:                          return QStringLiteral("—");
    }
}
QString DiagnosticsModel::modeByte() const  { return HwIo::instance().switches().modeByte(); }
bool DiagnosticsModel::enableS1() const     { return HwIo::instance().switches().enableS1(); }
bool DiagnosticsModel::enableS2() const     { return HwIo::instance().switches().enableS2(); }
QString DiagnosticsModel::enableByte() const{ return HwIo::instance().switches().enableByte(); }
bool DiagnosticsModel::switchReady() const  { return HwIo::instance().switches().isReady(); }

bool DiagnosticsModel::buzzerReady() const  { return HwIo::instance().buzzer().isReady(); }

int  DiagnosticsModel::backlight() const    { return HwIo::instance().backlight().value(); }
int  DiagnosticsModel::backlightMax() const { return HwIo::instance().backlight().maxValue(); }
bool DiagnosticsModel::backlightReady() const { return HwIo::instance().backlight().isReady(); }

int DiagnosticsModel::lastKeyCode() const   { return HwIo::instance().matrixKeys().lastCode(); }
QString DiagnosticsModel::lastKeyName() const { return HwIo::instance().matrixKeys().lastName(); }
bool DiagnosticsModel::lastKeyPressed() const { return HwIo::instance().matrixKeys().lastPressed(); }
QStringList DiagnosticsModel::keyHistory() const { return HwIo::instance().matrixKeys().history(); }
bool DiagnosticsModel::keysReady() const    { return HwIo::instance().matrixKeys().isReady(); }

void DiagnosticsModel::setLed(int port, bool on)
{
    if (HwIo::instance().leds().set(port, on)) emit ledChanged();
}

void DiagnosticsModel::allLedsOff()
{
    HwIo::instance().leds().allOff();
    emit ledChanged();
}

void DiagnosticsModel::setBacklight(int v)
{
    if (HwIo::instance().backlight().setValue(v)) emit backlightChanged();
}

void DiagnosticsModel::beep(int ms)         { HwIo::instance().buzzer().beep(ms); }
void DiagnosticsModel::holdBuzzer(bool on)  { HwIo::instance().buzzer().setHold(on); }

} // namespace smbq6r
