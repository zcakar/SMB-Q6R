#include "switch_monitor.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <QSocketNotifier>
#include <QTimer>
#include <QDebug>

namespace smbq6r {

namespace {
constexpr const char* kButtonsDev     = "/dev/buttons";
constexpr const char* kButtonstopDev  = "/dev/buttonstop";
constexpr int kFrameLen = 8;
} // namespace

SwitchMonitor::SwitchMonitor(QObject* parent)
    : QObject(parent)
{
    buttonsFd_ = ::open(kButtonsDev, O_RDONLY | O_NONBLOCK);
    if (buttonsFd_ < 0) {
        qWarning() << "SwitchMonitor:" << kButtonsDev << "open failed —" << ::strerror(errno);
    } else {
        buttonsNotifier_ = new QSocketNotifier(buttonsFd_, QSocketNotifier::Read, this);
        connect(buttonsNotifier_, &QSocketNotifier::activated,
                this, &SwitchMonitor::onButtonsReadable);
        qInfo() << "SwitchMonitor: opened" << kButtonsDev;
    }

    buttonstopFd_ = ::open(kButtonstopDev, O_RDONLY | O_NONBLOCK);
    if (buttonstopFd_ < 0) {
        qWarning() << "SwitchMonitor:" << kButtonstopDev << "open failed —" << ::strerror(errno);
    } else {
        buttonstopNotifier_ = new QSocketNotifier(buttonstopFd_, QSocketNotifier::Read, this);
        connect(buttonstopNotifier_, &QSocketNotifier::activated,
                this, &SwitchMonitor::onButtonstopReadable);
        qInfo() << "SwitchMonitor: opened" << kButtonstopDev;
    }

    // Trigger an initial read so the model reflects current state without
    // requiring a state transition. The drivers may return EAGAIN if no
    // frame has been pushed yet — that's fine; we'll poll briefly below.
    if (buttonsFd_ >= 0)    onButtonsReadable();
    if (buttonstopFd_ >= 0) onButtonstopReadable();

    // Some kernels only push a /dev/buttons frame on edge transitions, so a
    // freshly opened fd has nothing waiting and the UI shows mode=None until
    // the operator wiggles the key. Poll at 200 ms for a few seconds so the
    // first frame is captured even without a transition.
    primeTimer_ = new QTimer(this);
    primeTimer_->setInterval(200);
    connect(primeTimer_, &QTimer::timeout, this, [this]() {
        if (buttonsFd_    >= 0) onButtonsReadable();
        if (buttonstopFd_ >= 0) onButtonstopReadable();
        if (mode_ != Mode::None) primeTimer_->stop();
    });
    primeTimer_->start();
    QTimer::singleShot(10'000, primeTimer_, &QTimer::stop);
}

SwitchMonitor::~SwitchMonitor()
{
    if (buttonsFd_     >= 0) ::close(buttonsFd_);
    if (buttonstopFd_  >= 0) ::close(buttonstopFd_);
}

void SwitchMonitor::onButtonsReadable()
{
    char buf[kFrameLen + 1] = {0};
    const auto n = ::read(buttonsFd_, buf, kFrameLen);
    if (n != kFrameLen) {
        return;  // EAGAIN or partial; QSocketNotifier will fire again.
    }

    // Build human-readable raw byte string for the UI.
    QString raw = QString::fromLatin1(buf, kFrameLen);
    Mode newMode = Mode::None;
    // HN00-09Q6 measured bit positions (differs from HT0803/HT0804 docs):
    //   bit 3 = Manual,  bit 4 = Auto,  bit 5 = Stop.
    // Verified empirically 2026-05-12: rotating the key toward the panel's
    // "Auto" label set bit 4 (originally documented as Manual). We swap.
    if (buf[3] == '1') newMode = Mode::Manual;
    if (buf[4] == '1') newMode = Mode::Auto;
    if (buf[5] == '1') newMode = Mode::Stop;

    if (raw != modeByte_ || newMode != mode_) {
        modeByte_ = raw;
        mode_ = newMode;
        emit modeChanged();
    }
}

void SwitchMonitor::onButtonstopReadable()
{
    char buf[kFrameLen + 1] = {0};
    const auto n = ::read(buttonstopFd_, buf, kFrameLen);
    if (n != kFrameLen) {
        return;
    }

    QString raw = QString::fromLatin1(buf, kFrameLen);
    bool s1 = buf[7] == '1';
    bool s2 = buf[6] == '1';

    if (raw != enableByte_ || s1 != s1_ || s2 != s2_) {
        enableByte_ = raw;
        s1_ = s1;
        s2_ = s2;
        emit enableChanged();
    }
}

} // namespace smbq6r
