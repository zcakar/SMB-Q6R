#include "diagnostics_model.h"

#include "hwio.h"
#include "led_controller.h"
#include "switch_monitor.h"
#include "buzzer_controller.h"
#include "backlight_controller.h"
#include "matrix_keys_monitor.h"
#include "key_map_store.h"

#include <QVector>

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

bool DiagnosticsModel::simulatorMode()
{
    auto& io = HwIo::instance();
    return io.leds().isSimulator()
        || io.switches().isSimulator()
        || io.buzzer().isSimulator()
        || io.backlight().isSimulator()
        || io.matrixKeys().isSimulator();
}

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

QVariantList DiagnosticsModel::loadKeyMap()
{
    const QVector<int> data = KeyMapStore::load();
    QVariantList out;
    out.reserve(data.size());
    for (int v : data) out.append(v);
    return out;
}

void DiagnosticsModel::saveKeyMap(const QVariantList& codes)
{
    QVector<int> v;
    v.reserve(codes.size());
    for (const QVariant& x : codes) v.append(x.toInt());
    KeyMapStore::save(v);
}

void DiagnosticsModel::clearKeyMap()
{
    KeyMapStore::clear();
}

void DiagnosticsModel::simSetMode(const QString& name)
{
    SwitchMonitor::Mode m = SwitchMonitor::Mode::None;
    if (name.compare("Auto",   Qt::CaseInsensitive) == 0) m = SwitchMonitor::Mode::Auto;
    if (name.compare("Manual", Qt::CaseInsensitive) == 0) m = SwitchMonitor::Mode::Manual;
    if (name.compare("Stop",   Qt::CaseInsensitive) == 0) m = SwitchMonitor::Mode::Stop;
    HwIo::instance().switches().simulateMode(m);
}

void DiagnosticsModel::simSetDeadman(bool s1, bool s2)
{
    HwIo::instance().switches().simulateDeadman(s1, s2);
}

void DiagnosticsModel::simKeyEvent(int code, bool pressed)
{
    HwIo::instance().matrixKeys().simulateKey(code, pressed);
}

} // namespace smbq6r
