# Phase 1 — Donanım Haritalama (Hardware Mapping) — Mevcut Durum

> Bu dosya **iki günlük çalışmanın özet eşdeğeridir.** Yeni bir sohbete
> başladığında bunu OKUYUNCA Phase 1'in nereye geldiğini, nasıl çalıştığını
> ve neyin neden böyle yapıldığını bilirsin. Bir önceki kapsam belgeleri:
> [SESSION_2026-05-12.md](SESSION_2026-05-12.md), [ENGINEERING_LOG.md](ENGINEERING_LOG.md).

---

## 1. Bir Cümleyle Durum

`SMB-Q6R Teach Pendant Hardware Mapping` uygulaması; Lavichip HN00-09Q6
(device code `MAT-QT-TP-PC10C-Q6-UBT-L1`) cihazında tam ekran kiosk olarak
çalışıyor, tüm 5 donanım altsistemi (LED, mode-switch, deadman-switch,
buzzer, backlight, matrix-keypad) bağlı, kullanıcı **kalıcı tuş haritası**
oluşturup `~/.smb-q6r/keymap.dat` dosyasında saklıyor. GitHub:
<https://github.com/zcakar/SMB-Q6R>.

---

## 2. Cihaz Donanım Eşlemesi — DOĞRULANMIŞ

### 2.1 İndikatör LED'leri

| Port    | Fiziksel LED  | Renk    | Driver Davranışı       |
|---------|---------------|---------|-------------------------|
| port 0  | **ENABLE**    | Yeşil   | `ioctl(fd, on, 0)`     |
| port 1  | **SERVO**     | Yeşil   | `ioctl(fd, on, 1)`     |
| port 2  | **STOP**      | Kırmızı | `ioctl(fd, on, 2)`     |
| port 3  | (yok)         | —       | ioctl kabul ediyor ama görünür LED yok |
| port 4  | —             | —       | `EINVAL` (driver reddediyor) |

`LedController::set(port, on)` çağrılarında bu tablo bağlayıcıdır.
HN00-09Q6 datasheet "Indicator Lights: From left to right: Stop (Red),
Servo (Green), Enable (Green)" — fiziksel sıralama soldan sağa STOP/SERVO/
ENABLE, **port numarası sırası ters** (yani port 0 ENABLE = en sağdaki
yeşil LED).

### 2.2 Mode Switch (3-Konum Anahtar)

| Konum    | Raw byte      | Bit pozisyonu     |
|----------|---------------|---------------------|
| AUTO     | `00010000`    | bit 4 (`buf[4]='1'`)|
| MANUAL   | `00001000`    | bit 3 (`buf[3]='1'`)|
| STOP     | `00000100`    | bit 5 (`buf[5]='1'`)|

> **Önemli:** Vendor manuali "bit 3 = Auto, bit 4 = Manual" der. HN00-09Q6
> bunu **ters** yapar. `SwitchMonitor` swap edilmiş yorumla çalışır.

**Açılış davranışı (kritik):** `/dev/buttons` driver **edge-only**;
açılışta state push'lamaz, yalnız transition'lar push'lanır. UI açılışta
mode = "—" gösterir, kullanıcı anahtarı bir kez oynatınca state lock'lanır.

### 2.3 Deadman Switch (Tutamak Üzerinde)

| Durum     | S1 (buf[0]) | S2 (buf[1]) |
|-----------|:-----------:|:-----------:|
| RELEASED  | 0           | 0           |
| ACTIVE    | 1           | 0           |
| (PANIC)   | —           | —           |

> **Datasheet 3-stage (Released/Active/Panic) der ama gerçek donanımda
> S2 hiç kapanmıyor:** ne kadar sert sıkılırsa sıkılsın `raw = 10000000`
> kalıyor. UI 2-stage olarak (Released / Active) gösterir, altta "Bu
> donanımda panic state sürücü tarafından üretilmiyor" notu var.

**Vendor docs hatası:** Manuel "bit 6 = S2, bit 7 = S1" der ama vendor'ın
kendi `button.cpp`'si `buf[0]/buf[1]`'i okur ve gerçek donanım da o
pozisyonlardan veri üretir. Bizim kod da `buf[0]/buf[1]` okur.

### 2.4 Matrix Klavye (14 Buton)

7 satır × 2 sütun:
- **Sol sütun**: J1−, J2−, J3−, J4−, J5−, J6−, J7−
- **Sağ sütun**: J1+, J2+, J3+, J4+, J5+, J6+, J7+

Linux KEY_* kodları **physical-to-screen eşlemesi** kullanıcıya bağlı
(her cihazın fabrikada özelleştirilmiş `default.keySet`'i farklı olabilir).
Bu yüzden UI **guided learn** ile çalışır: sarı vurgulu cell hangi
butona basılacağını söyler; kullanıcı basar, kod o cell'e yazılır,
sıradakine geçer.

Kullanıcı testinden (fotoğraf 1, 2026-05-12) capture edilen kısmi map:

| Cell      | Code |
|-----------|-----:|
| J1−       | 62   |
| J1+       | 3    |
| J2−       | 47   |
| J2+       | 5    |
| J3−       | 16   |
| J3+       | 30   |
| J4−       | 31   |
| J4+..J7+  | (TBD — kullanıcı tamamlayacak) |

**Bu map cihazdaki `~/.smb-q6r/keymap.dat`'de saklanır** — uygulamayı
kapatıp açtığında ezbere bilinir, kullanıcı yeniden mapping yapmak
zorunda değil.

### 2.5 Buzzer (Piezo)

| Çağrı                  | Etki                                |
|------------------------|-------------------------------------|
| `ioctl(fd, 0, *)`      | sustur (bazen `EINVAL` döner)       |
| `ioctl(fd, 1, 1)`      | sürekli AÇ                          |
| `ioctl(fd, 1, 0)`      | sürekli KAPAT                       |
| `ioctl(fd, 3, ms)`     | `ms` milisaniye bip (min 10 ms)     |

**Driver'da volume kontrolü yoktur** — duty cycle sabit. Timed beep
(`cmd=3`) tipik olarak HOLD modundan (`cmd=1`) **daha yumuşak** geliyor;
en yüksek sesli sürüm `HOLD ON` ile.

### 2.6 Backlight

`/sys/class/backlight/backlight/brightness` ASCII int 0..100.
`/sys/class/backlight/backlight/max_brightness` ile dinamik olarak max'i
keşfederiz. UI slider'ı **minimum 5'e clamp** edilmiştir (0'a inip
operatörü kör etmesin).

---

## 3. Yazılım Mimarisi

```
SMB-Q6R/
├── CMakeLists.txt                    Native build entry (Docker içinde)
├── docker/Dockerfile                 Focal-arm64 builder image
├── cmake/aarch64-linux-gnu.cmake     Multiarch cross-compile toolchain
│                                      (Noble artığı, kullanılmıyor)
├── scripts/
│   ├── docker-build.sh               Cross-compile via Docker (önerilen)
│   ├── deploy.sh                     scp + remote launch
│   ├── device-permissions.sh         udev rule install (tek seferlik)
│   ├── ssh-pendant.sh                wrapper
│   ├── setup-host-cross-toolchain.sh DEPRECATED (Noble-arm64 host paketleri)
│   └── teardown-host-cross-toolchain.sh DEPRECATED
├── src/
│   ├── main.cpp                      QGuiApplication + QQuickView fullscreen
│   ├── hwio.{h,cpp}                  Singleton — 5 alt sistem sahibi
│   ├── led_controller.{h,cpp}        /dev/leds (ioctl)
│   ├── switch_monitor.{h,cpp}        /dev/buttons + /dev/buttonstop
│   │                                  (QSocketNotifier; 10 sn açılış polling)
│   ├── buzzer_controller.{h,cpp}     /dev/pwm (ioctl)
│   ├── backlight_controller.{h,cpp}  sysfs r/w + dinamik max keşif
│   ├── matrix_keys_monitor.{h,cpp}   /dev/input/event0 raw evdev
│   ├── diagnostics_model.{h,cpp}     QML köprüsü (context property "diag")
│   └── key_map_store.{h,cpp}         ~/.smb-q6r/keymap.dat I/O
├── qml/
│   ├── Main.qml                      Tek sayfa light-theme layout
│   ├── Card.qml                      Beyaz panel + gri title strip
│   ├── ModePill.qml                  AUTO/MANUAL/STOP pill indicator
│   ├── DeadmanStage.qml              RELEASED/ACTIVE stage
│   ├── PressButton.qml               ABB-tarzı press button
│   ├── KeyCell.qml                   Realistic matrix-key (bezel + dark face)
│   ├── LedTile.qml                   İndikatör LED tile + bulb
│   └── qml.qrc                       Qt resource liste
└── vendor-demos/                     7 vendor Qt Widgets app
    └── CMakeLists.txt                Hepsi tek loop ile cross-compile
```

### 3.1 HwIo Singleton

Bütün kernel device FD'lerini (`/dev/leds`, `/dev/pwm`, `/dev/buttons`,
`/dev/buttonstop`, `/dev/input/event0`) **bir kere açar**, uygulama yaşamı
boyunca tutar. Vendor driver'ları **yeniden açma**ya tolerans göstermez
(Errno 22). Singleton bu invariantı garanti eder.

### 3.2 QML/C++ Köprüsü

C++ tarafındaki `DiagnosticsModel` örneği QML'ye `diag` context property
adıyla expose edilir. **`model` ismi rezerve**dir (Repeater/ListView
delegate scope'unda gölgelenir) — bu yüzden ad `diag` seçildi.

QML componentleri ayrı `.qml` dosyalarındadır çünkü Qt 5.12'de inline
`component` keyword'ü yoktur (Qt 6 syntax).

### 3.3 Persistent Key Map

14 hücreli matrix-key kod haritası `~/.smb-q6r/keymap.dat` dosyasında
saklanır (her satırda bir integer, `-1` boş demek). Atomic write
(`.tmp` → rename). Uygulama her tuş basışından sonra otomatik kaydeder;
açılışta dosyayı yükler ve `selectedCell` ilk boş hücreyi gösterir.

`reset map` butonu dosyayı siler ve guided-learn cell 0'dan başlar.

### 3.4 Mode-switch Edge-only Driver

`/dev/buttons` driver yalnızca state geçişlerinde frame push'lar.
**Açılışta state pull edilemez** (Python ile test edildi: `select(1s)`
boş, blocking `read()` 2 sn timeout). UI ilk açılışta "—" gösterir,
kullanıcı anahtarı bir kez oynatınca state lock'lanır.

Compensation: `SwitchMonitor` 10 saniye boyunca 200 ms aralıklarla
edge-trigger'lı QSocketNotifier'ı dürter (silent EAGAIN olabilir, ama
ilk transition gelirse hemen yakalanır).

---

## 4. Build, Deploy, Çalıştırma

### 4.1 Host Önkoşulları

- Ubuntu 24.04 Noble (host)
- Docker daemon çalışıyor (`sudo systemctl start docker`)
- Kullanıcı `docker` grubunda
- `sshpass` kurulu (`apt install sshpass`)
- SSH key GitHub `zcakar` hesabıyla authenticate ediyor

### 4.2 Build Akışı

```bash
cd /home/embed/Dev/QT6/TeachPendant/SMB-Q6R

# 1. Cross-compile inside Docker (ilk seferinde ~3.5 dk image build)
scripts/docker-build.sh

# 2. Cihaza deploy + uzaktan başlat
scripts/deploy.sh

# Uygulamayı durdurmak için:
scripts/deploy.sh --kill-only
# veya doğrudan:
sshpass -p '' ssh Tronlong@192.168.1.245 'killall smb_q6r'
```

Çıktı: `build-arm64/smb_q6r.stripped` (~91 KB ELF aarch64).

### 4.3 Cihaz Yetkilendirme (Tek Seferlik)

Yeni bir cihaz takıldığında, ya da imaj sıfırlanırsa:

```bash
# Cihazda bir kez:
# - /etc/udev/rules.d/99-smb-q6r.rules kurulur
# - Tronlong input grubuna eklenir
# - /dev/* dosyaları plugdev erişimine açılır
scripts/device-permissions.sh
```

Detay: [DEVICE_DEPLOY_NOTES.md](DEVICE_DEPLOY_NOTES.md).

### 4.4 Vendor Demoları (Yan Yana Karşılaştırma)

7 vendor referans demosu (led, button, matrixKeys, pwm, rotaryEncoder,
wheel, backlight) `vendor-demos/` altında. `docker-build.sh` bunları da
otomatik derler; `deploy.sh` cihaza `/home/Tronlong/vendor-demos/` altına
kopyalar. Kullanım:

```bash
scripts/deploy.sh --run-vendor=led    # vendor LED demo'sunu aç
```

---

## 5. Bilinmesi Gereken İncelikler (Tuzaklar)

### 5.1 Qt 5.12 Tuzakları

- `component <Name> : <Base> { ... }` inline syntax **yoktur** (Qt 6'da
  eklendi). Component'ler ayrı dosyalarda.
- `model` Repeater/ListView delegate scope'unda **rezerve**dir; C++
  context property'leri farklı adlandırın (biz `diag` kullandık).
- `QtQuick.Controls 2`, `QtQuick.Layouts`, `QtQuick.Window` **cihazda
  yüklü değil** — yalnız `QtQuick 2.12` modülünü kullanırız. ApplicationWindow
  yerine QQuickView (C++ tarafından), Layouts yerine elle anchor.

### 5.2 XFCE / Compositor

`Qt::FramelessWindowHint` kullanırsanız XFCE pencereyi **override-redirect**
olarak yorumluyor ve compositor altında **görünmez** yapıyor. `showFullScreen()`
tek başına yeterli.

### 5.3 Cihaz Driver Davranışı

- `/dev/leds`, `/dev/pwm`, `/dev/buttons`, `/dev/buttonstop` her biri
  **bir kere** açılmalı. Yeniden açma driver state'ini bozar.
- `/dev/buttons` ve `/dev/buttonstop` **edge-only**: açılışta state pull yok.
- LED port 4 → `EINVAL`. Port 0..3 geçerli.
- Buzzer'da volume kontrolü yok; HOLD timed beep'ten yüksek.

### 5.4 Cross-Compile / glibc

Host (Noble) glibc 2.39, cihaz (Focal) glibc 2.31. Noble host doğrudan
build edince `__libc_start_main@GLIBC_2.34` sembolü gömülür, cihazda
loader fail eder. **Docker Focal-arm64 imajı zorunlu.** Yalnızca host'ta
build dene*me*yin.

### 5.5 Vendor Docs Hataları

| Konu                          | Manual diyor   | Gerçek      |
|-------------------------------|----------------|-------------|
| Mode switch bits               | bit 3=Auto, 4=Manual | Ters! bit 3=Manual, 4=Auto |
| Deadman bits                   | bit 6=S2, 7=S1 | buf[0]=S1, buf[1]=S2 |
| Deadman 3-stage panic          | Var            | Yok (S2 hiç kapanmıyor) |
| Serial port                    | /dev/ttyS1     | /dev/ttyS2 |
| SSH login                      | root/1234      | Tronlong/(boş) |
| Qt SDK                         | 5.12.9 / linuxfb-eglfs | 5.12.8 / xcb |
| Kernel                         | 4.9 (TI Sitara) | 5.10.209-rt89 (RK3568) |

---

## 6. UI Tasarım Notları

### 6.1 Light Theme Paleti

```
bg          #f3f4f6   text       #111827
cardBg      #ffffff   textSub    #4b5563
border      #d1d5db   textMuted  #9ca3af
accent      #2563eb   (ABB-blue)
success     #16a34a
warning     #d97706
danger      #dc2626
```

### 6.2 Layout (1280×800)

- **Header (h=70):** Başlık + device code + 5 hwio status nokta + alt ABB-mavi accent çizgi
- **Sol panel (w=514):**
  - MODE SWITCH (h=210)
  - DEADMAN SWITCH (h=210)
  - BUZZER · BACKLIGHT (h=130)
  - KEY HISTORY (kalanı)
- **Sağ panel (w=720+):**
  - INDICATOR LEDs (h=150) — 4 tile + ALL OFF
  - MATRIX KEYPAD (kalanı) — 7×2 grid

### 6.3 Realistic Key Cell

Her cell:
- Dış light-gray bezel (gradient: `#eaecf0` → `#c8ccd2`)
- İç dark key face (gradient `#475264` → `#171f2b`, mapped → `#3a3f4a` →
  `#0c0f15`, pressed → yeşil)
- Üst hafif beyaz "shine" overlay
- Axis label üstte (J1 −, J1 + vb)
- Büyük +/− glyph ortada
- Code readout altta

Selected cell (next to fill) sarı kenar (`#fbbf24`).

---

## 7. Phase 1 Kalan İşler

| İş                                          | Durum   |
|---------------------------------------------|---------|
| LED port haritası                           | ✅ Tamam |
| Mode switch bit haritası                    | ✅ Tamam |
| Deadman bit haritası (2-state olduğu netleşti) | ✅ Tamam |
| Backlight slider                            | ✅ Tamam |
| Buzzer (timed + hold)                       | ✅ Tamam |
| Matrix key 14 tuş + persistence             | ✅ Tamam |
| Tüm 14 tuşun haritasının tamamlanması       | ⏳ Kullanıcıda (UI rehberli) |
| Jog wheel handler (Iter F)                  | ⏳ Henüz yok |
| System info widget (Iter G)                 | ⏳ Henüz yok |
| CNC DT550 RAR çıkar (Phase 2 hazırlık)      | ⏳ p7zip-full kurulması gerek |

---

## 8. GitHub Workflow

```bash
# Remote (zaten kurulu):
# git@github.com:zcakar/SMB-Q6R.git

# Tipik döngü:
# 1. Değişiklik yap
# 2. Build + deploy + canlı test
# 3. Commit:
git add -A
git commit -m "<conventional commit message>"
# 4. Push:
git push origin main
```

SSH `zcakar` GitHub kullanıcısıyla authenticate ediyor; başka projeler
(`zzafercakar/sodoo`, `zzafercakar/cadnc`) farklı host alias kullanıyor
(`github-sodoo`, `github-zzafercakar`).

---

## 9. Sıradaki Sohbet İçin Hızlı Brief

> Yeni bir sohbete `START_HERE.md` ile başla. Aşağıdakileri biliyor olman
> yeterli:
>
> - Proje SMB-Q6R; Lavichip HN00-09Q6 cihazında çalışıyor.
> - Phase 1 (hardware mapping) **çalışır durumda**; uygulama cihazda
>   `~/.smb-q6r/keymap.dat`'a kalıcı tuş haritası kaydeder.
> - Cross-compile **Docker** üzerinden (`scripts/docker-build.sh`).
> - Deploy **scp + ssh launch** (`scripts/deploy.sh`).
> - Mode bits ve deadman bits vendor docs'tan **farklı** — bu dosyadaki
>   tabloya bak.
> - Devamında **Phase 2** (PLC haberleşme — CodeSys SMB-MAT-LC-C07,
>   protokol seçimi). [`PLC_INTEGRATION.md`](PLC_INTEGRATION.md) hazır.
> - Tüm `.ai/` dökümanları güncel.
