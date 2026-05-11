# Phase 1 — Diagnostics Demo Design Specification

**Status:** Approved (design)
**Author:** SMB-Q6R team (kickoff 2026-05-11)
**Phase:** 1 of 8 (see `.ai/WORKPLAN.md`)

## 1. Goal

Build the first end-to-end Qt application that runs on the HN00-09Q6 teach
pendant and exercises every reachable peripheral. The purpose is two-fold:

1. **Verify hardware mapping** is what the documentation and SDK demos claim
   (notably the LED-port → physical-LED correspondence, which differs between
   HT0803 and HT0804 model families and needs physical confirmation on
   HN00-09Q6).
2. **Establish the project skeleton** — CMake build, Qt QML structure, the
   `HwIo` singleton facade, and the deploy script — that every subsequent
   phase will reuse without major refactor.

The application is a **diagnostic tool**, not a production UI. Its UX is
optimised for engineer use during commissioning.

## 2. Non-Goals

- No PLC connection in this phase. PLC integration is Phase 2.
- No safety logic. Safety state machine is Phase 3.
- No robot kinematics or jog command. That is Phase 4.
- No persistence (settings file, calibration). Phase 5+.

## 3. UI Framework Decision

**Qt QML** (Qt Quick Controls 2), per kickoff brainstorming.

Rationale:
- 1280×800 capacitive touchscreen with Mali-G52 GPU benefits from
  hardware-accelerated QML rendering.
- Forward-looking: phases 4 (jog) and 6 (program editor) benefit more from
  QML's animation and gesture support.
- The `HwIo` C++ layer is framework-neutral, so the QML choice does not
  constrain the hardware interface.

Trade-off accepted: All vendor demo apps (`HWInterfaceDemo`, `lyx_appDemo`)
are Qt Widgets. We will not be able to copy-paste from them for UI; we
re-implement against the same `/dev` API.

## 4. Build & Deploy Strategy

**Cross-compile from host (revised 2026-05-11 after device probe).**

Original plan was native build on device, but device probe revealed:
- No Qt dev headers (vendor's `/usr/lib/qt-5.15.10/` is runtime-only)
- No `qmake`, no `cmake` on device
- No internet path from device (default gateway is host)

Two device-local Qt runtimes exist: vendor **5.15.10** under
`/usr/lib/qt-5.15.10/lib/`, and Ubuntu Focal **5.12.8** at standard system
paths. Vendor's `lyx_appDemo` actually links against the system **5.12.8**
copy, but **we will target the vendor 5.15.10 copy** by sourcing
`/etc/profile.d/qt_env.sh` at launch — same convention the vendor's QML
modules and qt5 plugins are configured for.

### Toolchain on host (Ubuntu 24.04 Noble)

```bash
sudo dpkg --add-architecture arm64
# Add arm64-only sources from ports.ubuntu.com (separate file)
sudo apt update
sudo apt install -y \
    crossbuild-essential-arm64 \
    qtbase5-dev:arm64 \
    qtdeclarative5-dev:arm64 \
    qml-module-qtquick-controls2:arm64 \
    qml-module-qtquick-window2:arm64 \
    qml-module-qtquick-layouts:arm64
```

Host's Qt is **5.15.13** (Noble) — same minor branch as device's vendor
**5.15.10**, hence ABI-compatible. Build with 5.15.13 headers, link with
runtime 5.15.10 on device.

### CMake toolchain file

`cmake/aarch64-linux-gnu.cmake` sets `CMAKE_C_COMPILER`,
`CMAKE_CXX_COMPILER` to the cross binaries, `CMAKE_SYSROOT` to `/` (because
multiarch puts arm64 libs in standard prefix), and `CMAKE_PREFIX_PATH` to
arm64 Qt5.

### Deploy

- Build on host:
  `cmake -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake -S . && cmake --build build-arm64 -j$(nproc)`
- `scp build-arm64/smb_q6r Tronlong@192.168.1.245:/home/Tronlong/smb-q6r/`
- Launch on device with `source /etc/profile.d/qt_env.sh && DISPLAY=:0 ./smb_q6r`.

A single host-side script (`scripts/build-and-deploy.sh`) does cmake build
+ scp + remote launch.

## 5. Architecture

### 5.1 Layered View

```
┌────────────────────────────────────────────────────┐
│  QML UI (Quick Controls 2)                         │
│  Main.qml (TabBar + StackLayout)                   │
│  pages/{Led,Buzzer,Switch,Keypad,Wheel,            │
│         Backlight,System}Page.qml                  │
├────────────────────────────────────────────────────┤
│  DiagnosticsModel : QObject                        │
│  - exposes properties + invokable methods to QML   │
│  - aggregates HwIo facets, emits change signals    │
├────────────────────────────────────────────────────┤
│  HwIo (singleton)                                  │
│  + Led, Buzzer, Switch, Backlight, SystemInfo      │
│  Each owns a single fd opened at startup.          │
└────────────────────────────────────────────────────┘
```

### 5.2 Class Inventory

| Class                  | File                          | Responsibility                                                |
|------------------------|-------------------------------|---------------------------------------------------------------|
| `HwIo`                 | `src/hwio.{h,cpp}`            | Singleton; owns each subcomponent; opens fds once on first use|
| `LedController`        | `src/led_controller.{h,cpp}`  | `ioctl(fd, state, port)` wrapper; identify sequence           |
| `BuzzerController`     | `src/buzzer_controller.{h,cpp}`| `ioctl(fd, cmd, val)`; beep(ms) / hold(bool)                 |
| `SwitchMonitor`        | `src/switch_monitor.{h,cpp}`  | `QSocketNotifier` on `/dev/buttons` and `/dev/buttonstop`     |
| `BacklightController`  | `src/backlight_controller.{h,cpp}`| Read/write sysfs brightness                              |
| `SystemInfo`           | `src/system_info.{h,cpp}`     | Hostname, primary IP, uptime, free memory                     |
| `DiagnosticsModel`     | `src/diagnostics_model.{h,cpp}`| `QObject` exposed to QML; signals/properties surface         |

### 5.3 Threading

This phase: **single-threaded**. All hardware I/O runs in the Qt event loop
with `QSocketNotifier` for async reads. CPU load and latency requirements
are well within a single A53 core at idle.

Phase 3 (safety) will introduce a dedicated `SCHED_FIFO` thread for
emergency-stop and enable-switch monitoring. Designing `SwitchMonitor`
behind a clean API in Phase 1 makes that swap straightforward.

### 5.4 QML ↔ C++ Bridge

`DiagnosticsModel` is exposed as a **context property** named `model` in
`main.cpp` via `engine.rootContext()->setContextProperty("model", &model)`.

Exposed:
- `Q_PROPERTY(int ledMask READ ledMask NOTIFY ledChanged)` — bitmask of currently on LEDs
- `Q_PROPERTY(bool enableS1 READ enableS1 NOTIFY enableChanged)`
- `Q_PROPERTY(bool enableS2 READ enableS2 NOTIFY enableChanged)`
- `Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)` — "Auto" / "Manual" / "Stop" / "None"
- `Q_PROPERTY(int backlight READ backlight WRITE setBacklight NOTIFY backlightChanged)`
- `Q_PROPERTY(QString hostname READ hostname CONSTANT)`
- `Q_PROPERTY(QString primaryIp READ primaryIp NOTIFY primaryIpChanged)`
- `Q_PROPERTY(qint64 uptimeSeconds READ uptimeSeconds NOTIFY uptimeChanged)`
- `Q_INVOKABLE void setLed(int port, bool on)`
- `Q_INVOKABLE void identifyLeds()` — sequential 0..4 each 500ms
- `Q_INVOKABLE void beep(int milliseconds)`
- `Q_INVOKABLE void holdBuzzer(bool on)`

Matrix keys and wheel are handled in QML directly via `Keys.onPressed` and
`WheelHandler` since Qt's evdev mouse/keyboard pipeline is already wired.

## 6. File Layout

```
SMB-Q6R/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── hwio.{h,cpp}
│   ├── led_controller.{h,cpp}
│   ├── buzzer_controller.{h,cpp}
│   ├── switch_monitor.{h,cpp}
│   ├── backlight_controller.{h,cpp}
│   ├── system_info.{h,cpp}
│   └── diagnostics_model.{h,cpp}
├── qml/
│   ├── Main.qml
│   ├── pages/
│   │   ├── LedPage.qml
│   │   ├── BuzzerPage.qml
│   │   ├── SwitchPage.qml
│   │   ├── KeypadPage.qml
│   │   ├── WheelPage.qml
│   │   ├── BacklightPage.qml
│   │   └── SystemPage.qml
│   └── qml.qrc
├── scripts/
│   ├── ssh-pendant.sh        (existing)
│   ├── deploy.sh             (existing — will adapt)
│   └── sync-and-build.sh     (new)
└── docs/superpowers/specs/
    └── 2026-05-11-phase1-diagnostics-design.md   (this file)
```

## 7. Build & Deploy Flow

### `scripts/sync-and-build.sh`

```bash
HOST=192.168.1.245
USER=Tronlong
REMOTE=/home/$USER/smb-q6r

rsync -az --delete \
  --exclude build --exclude '.git' \
  ./ "$USER@$HOST:$REMOTE/"

ssh "$USER@$HOST" "cd $REMOTE && cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j4"

ssh "$USER@$HOST" "source /etc/profile.d/qt_env.sh && DISPLAY=:0 $REMOTE/build/smb_q6r"
```

(Wrapped with sshpass for the empty-password Tronlong user.)

### CMakeLists.txt key points

- `find_package(Qt5 COMPONENTS Core Quick QmlImportScanner REQUIRED)`
- `qt5_add_resources(QML_RESOURCES qml/qml.qrc)`
- Define `smb_q6r` executable with `src/*.cpp` and `${QML_RESOURCES}`
- `target_link_libraries(smb_q6r PRIVATE Qt5::Core Qt5::Quick)`
- `target_compile_features(smb_q6r PRIVATE cxx_std_17)`

## 8. UI Page Sketches

### LedPage
- 5 toggle switches (Port 0..4), labelled "Port N (waiting for ident)"
- "Identify" button → calls `model.identifyLeds()` → sequential blink
- After user observes physical match, they can type the physical name
  ("STOP", "SERVO", "ENABLE", etc.) next to each port — this is logged to
  console and copy-pasteable into `ENGINEERING_LOG.md`.

### BuzzerPage
- Short beep buttons: 50ms / 200ms / 500ms / 1000ms
- "Hold" toggle: keeps buzzer on while held
- Disclaimer: avoid prolonged hold (driver may stick — manual notes
  "device file cannot be opened repeatedly")

### SwitchPage
- Two big indicators: Enable Switch (S1 / S2) — green when active, red dot
  when both active (grip-pressed = enabled), blank when neither
- Mode display: large label "AUTO" / "MANUAL" / "STOP" with color coding
- Raw byte view (toggle): shows the 8-char buffer for debugging

### KeypadPage
- 14 keys laid out as 2 columns × 7 rows mimicking the physical pendant
- Pressed key flashes yellow for 200ms; persistent log below shows
  last 20 key codes + timestamps
- "Clear log" button

### WheelPage
- Big number: cumulative tick count (signed)
- Arrow indicator (↑ / ↓) flashes on each tick
- Button-press count for the wheel click
- "Reset counters" button

### BacklightPage
- Slider 0..100 bound to `model.backlight`
- Current value label
- Warning: setting to 0 turns off backlight (will need keypress / blind
  recovery)

### SystemPage
- Hostname, primary IP, uptime, free RAM, kernel version
- "Refresh" button + auto-refresh every 2 seconds

## 9. Test / Acceptance Criteria

Each item below should pass on the device before Phase 1 is closed:

1. App launches via `scripts/sync-and-build.sh` without manual intervention.
2. All 7 tabs are reachable by touch.
3. Each LED port (0..4) can be individually toggled; "Identify" sequences
   them. Physical port→LED map is recorded in `ENGINEERING_LOG.md`.
4. Short and hold buzzer work without stalling the UI.
5. Pressing Enable Switch (S1 then S2) updates UI within 100 ms.
6. Rotating the Mode Switch (Stop → Manual → Auto) updates UI within
   100 ms; only one position is "active" at a time.
7. All 14 physical buttons are recognised; KeypadPage shows their key code
   and the visual layout is correct (physical position matches UI position).
8. Jog wheel rotation produces matching delta on WheelPage; click registers.
9. Backlight slider visibly changes screen brightness.
10. SystemPage shows correct hostname `langyuxin`, IP `192.168.1.245`,
    Qt version `5.15.10`, kernel `5.10.209-rt89`.

## 10. Open Questions & Risks

1. **Qt dev headers may not be on the device.** If `qtbase5-dev` and
   `qtdeclarative5-dev` are absent, `apt install` requires either internet
   on the device (likely none — switch is local) or offline `.deb` files.
   **Mitigation:** check at the start of Phase 1; if missing, deploy
   `.deb`s from host via `scp` and `dpkg -i`.

2. **`/etc/profile.d/qt_env.sh` mis-routes touchscreen to event2** (the
   power-key device) instead of event3. This already works thanks to XCB
   auto-discovery, but should be fixed.
   **Action in Phase 1:** patch the file (with sudo) as part of the device
   setup step, document in `ENGINEERING_LOG.md`.

3. **LED driver state corruption on repeated open.** Not exercised in this
   phase if we keep one fd open via the singleton, but Phase 4+ may need
   the same guarantee — `HwIo` must enforce single-instance via private
   constructor + deleted copy/move.

4. **Hot-swap of the aviation cable** is a real-world scenario. This
   phase does not handle it (any I/O error will just print to log). Phase
   3 will detect and recover.

5. **Wheel direction convention** — does +1 mean clockwise (operator view)
   or counter-clockwise? Recorded once in Phase 1 from physical test, then
   referenced in Phase 4 jog mapping.

## 11. Out of Scope (deferred reminders)

- Internationalisation — Turkish UI strings are interleaved in QML files
  for this phase; i18n via `.ts` files happens in Phase 8.
- HiDPI / scaling — fixed 1280×800 layout, no responsive design.
- Theming / dark mode — single light theme is fine for now.
- Logging — `qDebug` to stdout only; structured logging is Phase 5.
- Crash recovery / supervisor — Phase 8 (systemd).

## 12. Definition of Done

Phase 1 is closed when:

- All 10 acceptance tests pass on the physical pendant.
- `ENGINEERING_LOG.md` contains a "Phase 1 closeout" entry with:
  - LED port→physical map
  - Matrix-key key-code → physical-position map
  - Enable switch S1/S2 interpretation (which combination = enabled)
  - Mode switch position → byte bit confirmation
  - Wheel direction convention
- `HwIo` is the only path to `/dev/*` in the codebase; no other file
  contains `open("/dev/...`.
- Source is committed to git, app deploys reproducibly via
  `scripts/sync-and-build.sh`.
