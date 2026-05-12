#include "matrix_keys_monitor.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>

#include <QSocketNotifier>
#include <QDateTime>
#include <QDebug>

namespace smbq6r {

namespace {
constexpr const char* kDevice = "/dev/input/event0";
constexpr int kMaxHistory = 30;

// Best-effort name for the most common Linux input codes that the HN00-09Q6
// matrix keypad is likely to use. Anything not listed appears as "KEY_<n>".
QString codeName(int code)
{
    static const struct { int code; const char* name; } table[] = {
        {  1, "ESC"      }, {  2, "1" }, {  3, "2" }, {  4, "3" }, {  5, "4" },
        {  6, "5"        }, {  7, "6" }, {  8, "7" }, {  9, "8" }, { 10, "9" },
        { 11, "0"        }, { 14, "BACKSPACE" }, { 15, "TAB" },
        { 16, "Q"        }, { 17, "W" }, { 18, "E" }, { 19, "R" }, { 20, "T" },
        { 21, "Y"        }, { 28, "ENTER" }, { 52, "DOT" },
        { 59, "F1"       }, { 60, "F2" }, { 61, "F3" }, { 62, "F4" },
        { 63, "F5"       }, { 64, "F6" }, { 65, "F7" }, { 66, "F8" },
        { 67, "F9"       }, { 68, "F10" }, { 87, "F11" }, { 88, "F12" },
        {103, "UP"       }, {104, "PAGEUP" },
        {105, "LEFT"     }, {106, "RIGHT" }, {108, "DOWN" },
        {109, "PAGEDOWN" }, {204, "DASHBOARD" },
    };
    for (const auto& e : table) {
        if (e.code == code) return QString::fromLatin1(e.name);
    }
    return QStringLiteral("?");
}
} // namespace

MatrixKeysMonitor::MatrixKeysMonitor(QObject* parent)
    : QObject(parent)
{
    fd_ = ::open(kDevice, O_RDONLY | O_NONBLOCK);
    if (fd_ < 0) {
        qWarning() << "MatrixKeysMonitor:" << kDevice << "open failed —" << ::strerror(errno);
        return;
    }
    notifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated,
            this, &MatrixKeysMonitor::onReadable);
    qInfo() << "MatrixKeysMonitor: opened" << kDevice;
}

MatrixKeysMonitor::~MatrixKeysMonitor()
{
    if (fd_ >= 0) ::close(fd_);
}

void MatrixKeysMonitor::onReadable()
{
    input_event ev{};
    while (true) {
        const auto n = ::read(fd_, &ev, sizeof(ev));
        if (n != static_cast<ssize_t>(sizeof(ev))) break;
        if (ev.type != EV_KEY) continue;

        lastCode_    = ev.code;
        lastPressed_ = ev.value == 1;
        const QString name = codeName(ev.code);
        lastName_ = name == "?"
            ? QStringLiteral("KEY_%1").arg(ev.code)
            : QStringLiteral("KEY_%1").arg(name);

        // value: 0=release, 1=press, 2=auto-repeat
        const char* tag = ev.value == 1 ? "v" : (ev.value == 0 ? "^" : "~");
        const QString entry = QString::fromLatin1("%1 %2 (code %3)")
            .arg(tag, lastName_, QString::number(ev.code));
        history_.prepend(entry);
        while (history_.size() > kMaxHistory) history_.removeLast();

        emit keyEvent();
    }
}

} // namespace smbq6r
