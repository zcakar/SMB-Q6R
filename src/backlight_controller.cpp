#include "backlight_controller.h"

#include <QFile>
#include <QDir>
#include <QDebug>

namespace smbq6r {

namespace {
constexpr const char* kBacklightDir = "/sys/class/backlight";
}

BacklightController::BacklightController()
{
    QDir dir(kBacklightDir);
    const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        simulator_ = true;
        qInfo() << "BacklightController: SIMULATOR mode (no backlight sysfs)";
        return;
    }
    const QString panel = entries.first();
    brightnessPath_ = QString::fromLatin1("%1/%2/brightness").arg(kBacklightDir, panel);
    const QString maxPath = QString::fromLatin1("%1/%2/max_brightness").arg(kBacklightDir, panel);

    QFile mf(maxPath);
    if (mf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bool ok = false;
        const auto m = mf.readAll().trimmed().toInt(&ok);
        if (ok && m > 0) max_ = m;
    }

    QFile f(brightnessPath_);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        simulator_ = true;
        qInfo() << "BacklightController: SIMULATOR mode —" << f.errorString();
        return;
    }
    ready_ = true;
    qInfo() << "BacklightController: panel" << panel << "max=" << max_;
}

int BacklightController::value() const
{
    if (simulator_) return simValue_;
    if (!ready_) return -1;
    QFile f(brightnessPath_);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    bool ok = false;
    const auto v = f.readAll().trimmed().toInt(&ok);
    return ok ? v : -1;
}

bool BacklightController::setValue(int v)
{
    if (v < 0) v = 0;
    if (v > max_) v = max_;
    if (simulator_) { simValue_ = v; return true; }
    if (!ready_) return false;
    QFile f(brightnessPath_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "BacklightController::setValue:" << f.errorString();
        return false;
    }
    f.write(QByteArray::number(v));
    return true;
}

} // namespace smbq6r
