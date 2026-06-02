# Phase 1 — Donanım Haritalama (+ Phase 1.5 OPC UA Bootstrap) — Mevcut Durum

> Bu dosya **proje boyunca yapılmış işin özet eşdeğeridir.** Yeni bir
> sohbete başladığında bunu OKUYUNCA Phase 1 + 1.5'in nereye geldiğini,
> neyin nasıl çalıştığını ve neyin neden böyle yapıldığını bilirsin.
> Daha derin tarihçe için:
> [SESSION_2026-05-12.md](SESSION_2026-05-12.md),
> [SESSION_2026-06-02.md](SESSION_2026-06-02.md),
> [ENGINEERING_LOG.md](ENGINEERING_LOG.md).

---

## 1. Bir Cümleyle Durum

`SMB-Q6R Teach Pendant Hardware Mapping` uygulaması; Lavichip HN00-09Q6
(device code `MAT-QT-TP-PC10C-Q6-UBT-L1`) cihazında **systemd user
service** ile boot'ta otomatik açılıyor, 5 donanım altsistemi (LED,
mode-switch, deadman-switch, buzzer, backlight, matrix-keypad) **+
OPC UA istemcisi** bağlı, kullanıcı **iki sekmeli arayüzde** (HARDWARE
+ PLC CONSOLE) hem donanım testleri yapıyor hem PLC değişkenlerini
canlı izleyip yazıyor. Host PC'de **simülatör bezel** ile donanımsız
geliştirme yapılabiliyor. GitHub: <https://github.com/zcakar/SMB-Q6R>.

---

## 2. Çalışan Özellikler — Tek Bakışta

| Alan | Durum | Detay |
|------|------|------|
| Cross-compile pipeline | ✅ | Docker Focal-arm64 (`scripts/docker-build.sh`) |
| Host build | ✅ | Native amd64 Qt 5.15 (`scripts/host-build.sh`) |
| Cihaza deploy | ✅ | `scripts/deploy.sh` → systemd start |
| systemd --user service | ✅ | `scripts/install-autostart.sh`, cold-boot test geçti |
| Sim mode fallback | ✅ | open()→sim her controller'da, bezel UI |
| 5 LED port haritası | ✅ | port 0 ENABLE · 1 SERVO · 2 STOP (canlı doğrulandı) |
| Mode switch (3-pos) | ✅ | Auto/Manual/Stop, vendor docs tersine — swapped |
| Deadman (2-state) | ✅ | Released/Active; S2 driver'da yok (donanım sınırı) |
| Buzzer (timed + hold) | ✅ | /dev/pwm ioctl 0/1/3 |
| Backlight slider | ✅ | /sys/class/backlight, 0–100 |
| Matrix keypad (14 tuş) | ✅ | **Hardcoded** kod haritası baked-in, mapping prompt yok |
| Font tofu fix | ✅ | DejaVu Sans QRC içinde bundled |
| Tab UI (HARDWARE / PLC) | ✅ | header altı 44px tab bar |
| OPC UA client | ✅ | open62541 v1.3.10 statik, worker QThread |
| PLC connect/disconnect | ✅ | UI'dan tetikleniyor, state machine queued signals |
| PLC namespace browse | ✅ | Connect'te otomatik, log'a node ID'ler |
| PLC subscribe + monitor | ✅ | DataChange callback → QML pills |
| PLC write (bool/int/double/string) | ✅ | PlcPage'de toggle butonu |
| PlcPage tablo + add form + log | ✅ | 12 default node, runtime'da yeni ekleme |
| Network alias (192.168.0.245/24) | ✅ | netplan'da persistent |

---

## 3. Cihaz Donanım Eşlemesi — DOĞRULANMIŞ

### 3.1 İndikatör LED'leri

| Port    | Fiziksel LED  | Renk    |
|---------|---------------|---------|
| port 0  | **ENABLE**    | Yeşil   |
| port 1  | **SERVO**     | Yeşil   |
| port 2  | **STOP**      | Kırmızı |
| port 3  | (yok, ioctl OK ama görünür LED yok) | — |
| port 4  | (yok, ioctl EINVAL) | — |

Fiziksel sıralama soldan-sağa STOP/SERVO/ENABLE, **port numarası ters**
(port 0 = en sağdaki yeşil LED).

### 3.2 Mode Switch (3-Konum Anahtar)

| Konum    | Raw byte    | Bit pozisyonu       |
|----------|-------------|---------------------|
| AUTO     | `00010000`  | bit 4 (`buf[4]='1'`)|
| MANUAL   | `00001000`  | bit 3 (`buf[3]='1'`)|
| STOP     | `00000100`  | bit 5 (`buf[5]='1'`)|

> Vendor manuali "bit 3 = Auto, bit 4 = Manual" der, HN00-09Q6 bunu
> **ters** yapar; `SwitchMonitor` swap edilmiş haritayla okur.
> Açılış değerini almak için `primeTimer_` 10 saniye boyunca 200 ms'de
> bir poll eder — driver edge-only çünkü.

### 3.3 Deadman (Enable) Switch

| Konum     | buf[0] | buf[1] | UI Etiketi |
|-----------|:------:|:------:|------------|
| Released  |   0    |   0    | RELEASED   |
| Active    |   1    |   0    | ACTIVE     |
| (panic)   |   1    |   1    | — (driver üretmiyor) |

Datasheet 3-stage diyor ama bu image'in driver'ı S2 (panic) bitini hiç
üretmiyor. UI iki state gösteriyor; PLC tarafında deadman validasyonu
ona göre yazılmalı.

### 3.4 Matrix Keypad (14 Jog Tuşu) — Sabit Harita

`DiagnosticsModel::defaultKeyMap()` (src/diagnostics_model.cpp) 14 KEY_*
kodunu screen sırasıyla baked-in tutar:

| Cell | Code | Linux KEY_ | | Cell | Code | Linux KEY_ |
|------|------|-----------|-|------|------|-----------|
| J1−  | 62   | F4        | | J1+  | 47   | V         |
| J2−  |  5   | 4         | | J2+  | 60   | F2        |
| J3−  |  4   | 3         | | J3+  |  3   | 2         |
| J4−  | 18   | E         | | J4+  | 17   | W         |
| J5−  | 32   | D         | | J5+  | 31   | S         |
| J6−  | 46   | C         | | J6+  | 45   | X         |
| J7−  | 30   | A         | | J7+  | 16   | Q         |

Önceki "tap-to-learn" akışı + `~/.smb-q6r/keymap.dat` kaldırıldı; mapping
prompt artık görünmez, ekran ilk açılışta direkt 14 tuş etiketlenmiş
şekilde gelir.

---

## 4. Tab UI Mimarisi

```
┌─ Header (70 px) ───────────────────────────────────────────────┐
│ SMB-Q6R  Teach Pendant Hardware Mapping     LED SWT BUZ BL KEYS PLC │
│ Device Code: MAT-QT-TP-PC10C-Q6-UBT-L1                              │
├─ Tab bar (44 px, koyu) ───────────────────────────────────────┤
│  [HARDWARE]   [PLC CONSOLE]                                     │
├─ Tab content (rest) ──────────────────────────────────────────┤
│ HARDWARE: leftPanel (514 px) + rightPanel (rest)               │
│   left:  MODE SWITCH · DEADMAN SWITCH · BUZZER·BACKLIGHT · KEY HISTORY │
│   right: INDICATOR LEDs (top) · MATRIX KEYPAD (rest, 7×2)      │
│ PLC:     connectBar (60 px) + body                              │
│   body:  watch tablosu (62 % genişlik) + add/log paneli (rest) │
└────────────────────────────────────────────────────────────────┘
```

- `root.currentPage` ("hardware" | "plc") tab seçilince güncellenir.
- Hardware panel'leri `visible: root.currentPage === "hardware"`.
- PlcPage `Loader { active: root.currentPage === "plc" }` ile lazy
  yüklenir, kapatıldığında bellek serbest.

---

## 5. Sim Mode (Host Build) — SimulatorBezel

Host'ta hiçbir /dev/* yok, her controller `simulator_` flag'ine düşer.
`DiagnosticsModel::simulatorMode()` true → main.cpp `SimulatorBezel.qml`
yükler. Bezel 1600×1000 koyu pendant gövdesi:

- Üst: 3-pos mode pills (Stop/Manual/Auto) + 3 indicator LED ayna + E-stop görsel
- Orta: 1280×800 inset alanda Main.qml yüklenir
- Sağ: 7×2 jog buton (defaultKeyMap'ten kod → simKeyEvent)
- Alt: Deadman 3-state pills + brand

Klavye girişleri de simKeyEvent'e forward'lanır.

---

## 6. PLC / OPC UA — Phase 1.5

### 6.1 Stack
- **open62541 v1.3.10 LTS**, CMake `FetchContent`, statik linkli.
- Worker `QThread` üzerinde tek `UA_Client*`; 50 ms `QTimer` ile
  `UA_Client_run_iterate()` pump.
- Cross-thread sinyaller queued; `PlcLink::State` `qRegisterMetaType`'lı.

### 6.2 Bağlantı

| Alan | Değer |
|------|------|
| PLC IP | 192.168.0.2 |
| Endpoint | `opc.tcp://192.168.0.2:4840` |
| Server published URL | `opc.tcp://SMB:4840` (warn — zararsız) |
| Security policy | None |
| User token | Anonymous |
| Pendant alias IP | 192.168.0.245/24 (netplan persistent) |

`scripts/configure-plc-network.sh` pendant eth1'ine ikinci IP alias
ekler, netplan'a yazar, anında ip-add'ler.

### 6.3 CodeSys Node ID Formatı

CodeSys'in OPC UA namespace'i (browse ile keşfedildi):

```
Application root: ns=4;s=|var|MAT LC-C07.Application
PLC device:       ns=4;s=|plc|MAT LC-C07
Variable:         ns=4;s=|var|MAT LC-C07.Application.<GVL>.<VarName>
```

Örnek: `ns=4;s=|var|MAT LC-C07.Application.GVL.Enable`

PLC symbols XML projesi adı (`CodeSysSP20_3Axis_CNC_SMB_LAZER_...`)
**node id'lerde geçmiyor** — sadece PLC device name (`MAT LC-C07`) +
"Application" geçiyor.

### 6.4 PlcLink API Özet

| Slot | Etki |
|------|------|
| `connectToServer(url)` | Async connect, başarılıysa auto-browse + reconfigureSubscriptions |
| `disconnectFromServer()` | Temiz teardown, plcValues cache temizlenir |
| `readNode(id)` | Tek seferlik read → `valueRead` |
| `subscribeNode(id)` | MonitoredItem ekle → her değişimde `valueChanged` |
| `writeNode(id, QVariant)` | bool/int/double/string → UA_Variant write |
| `browseNamespace(d=5, n=250)` | BFS, her node `nodeDiscovered` + qInfo log |

### 6.5 PlcPage UI

`PlcPage.qml` (PLC tab içeriği) iki sütun:

- **Sol (62%)**: Subscribed node tablosu — Label · NodeID · Value pill · Toggle (bool için) · Remove
- **Sağ**: "Add Node" formu (label + nodeId + writable checkbox) + son 30 event log satırı

Default watch listesi 12 node: `GVL.Enable`, EMG_01, 2× Door, 4× Limit,
emg_stop_Q00, laser_ctrl_Q03, pano_ayd_Q04.

### 6.6 Bilinen Sorun

- **Struct/function-block nodes** (örn `IoConfig_Globals.Device`, tip
  `T_CAADiagDeviceDefault`) doğrudan monitor edilince
  `BadAttributeIdInvalid` döner. Çocuk skalar field'lara subscribe etmek
  gerek. PlcPage default listesinde yok artık.

---

## 7. Build + Deploy Akışı

```bash
# Host (dev workstation, native amd64 Qt 5.15) — sim modunda çalıştırır
scripts/host-build.sh           # configure + build
scripts/host-build.sh run       # + launch (1600x1000 windowed)

# Cihaz (cross-compile)
scripts/docker-build.sh         # → build-arm64/smb_q6r.stripped (~1.8 MB)
scripts/deploy.sh               # scp + systemctl --user restart smb-q6r

# İlk kurulum (cihazda bir kerelik)
scripts/install-autostart.sh    # systemd unit + linger enable
scripts/configure-plc-network.sh # eth1 alias 192.168.0.245/24
scripts/device-permissions.sh   # udev rules for /dev/leds, /dev/buttons, ...
```

systemd unit `~/.config/systemd/user/smb-q6r.service` — `default.target`'a
bağlı, ExecStartPre 60 sn `xdpyinfo` poll'u ile X'i bekler, sonra başlar.

---

## 8. Kod / Dosya Haritası

```
src/
├── main.cpp                    # entry + sim-mode QML kaynak seçimi
├── hwio.cpp/h                  # tek-fd singleton, 6 alt-sistem ownership
├── led_controller.cpp/h        # /dev/leds + sim
├── switch_monitor.cpp/h        # /dev/buttons + /dev/buttonstop + sim
├── buzzer_controller.cpp/h     # /dev/pwm + sim
├── backlight_controller.cpp/h  # /sys/class/backlight + sim
├── matrix_keys_monitor.cpp/h   # /dev/input/event0 + sim
├── plc_link.cpp/h              # open62541 worker QThread
└── diagnostics_model.cpp/h     # QML-facing facade, sim slots + plc slots
qml/
├── Main.qml                    # screen UI: header · tabBar · hw panels
├── PlcPage.qml                 # PLC tab content
├── SimulatorBezel.qml          # host-only virtual pendant body
├── Card.qml · LedTile.qml · ModePill.qml · DeadmanStage.qml
├── KeyCell.qml · PressButton.qml
└── fonts/DejaVuSans.ttf + Bold.ttf  (QRC bundled, ~1.4 MB)
scripts/
├── docker-build.sh · host-build.sh · deploy.sh
├── install-autostart.sh · configure-plc-network.sh · device-permissions.sh
└── systemd/smb-q6r.service
docker/Dockerfile               # Focal arm64 builder image (git + python3 + qt5)
vendor-demos/
└── CodeSysSP20_3Axis_CNC_SMB_LAZER_17052025_1620.Device.Application.xml
                                # CodeSys symbol export (193 nodes)
```

---

## 9. Kalan İşler (Phase 1.5 Sonrası → Phase 2)

| İş | Öncelik | Not |
|----|---------|------|
| Jog wheel handler (/dev/input/event1) | M | Rotary encoder, hız ayarı için |
| System info widget (hostname/IP/uptime) | L | |
| PLC heartbeat / watchdog | **H** | Her N ms ping, koparsa soft-stop |
| Reconnect-on-disconnect | M | Şu an manuel Connect basmak gerek |
| QSettings ile son URL'i hatırla | L | |
| Struct node browse (IoConfig_Globals.Device children) | M | İhtiyaç doğunca |
| On-screen virtual numpad (URL editing) | L | USB klavye yokken |
| Phase 2: jog komutları PLC'ye → GVL_Control_Var | **H** | Faz 2 başlığı |
| Phase 2: Frame editör, program list, alarm view | H | |

---

## 10. GitHub Workflow

```bash
# Remote: git@github.com:zcakar/SMB-Q6R.git (zcakar SSH key)
# Branch: main (tek branch)
# Conventional commits

# Tipik döngü:
# 1. Değişiklik yap
# 2. scripts/host-build.sh (compile check, sim test)
# 3. scripts/docker-build.sh && scripts/deploy.sh (cihaz)
# 4. git add -A && git commit -m "<conv>"
# 5. git push origin main
```

---

## 11. Sıradaki Sohbet İçin Hızlı Brief

> Yeni bir sohbete `START_HERE.md` → bu dosya → güncel
> SESSION_*.md ile başla. Aşağıdakileri biliyor olman yeterli:
>
> - Proje SMB-Q6R; Lavichip HN00-09Q6 cihazında çalışıyor, systemd
>   ile boot'ta açılıyor.
> - Phase 1 (hardware) + Phase 1.5 (OPC UA bootstrap) **çalışır
>   durumda**; PLC 192.168.0.2'de canlı bağlanıyor, browse + subscribe
>   + read/write hepsi geçiyor.
> - Cross-compile **Docker** (`scripts/docker-build.sh`),
>   deploy **scp + systemctl** (`scripts/deploy.sh`).
> - Tab UI: HARDWARE + PLC CONSOLE (Main.qml header altında 44 px bar).
> - Mode/deadman bits vendor docs'tan **farklı** — bölüm 3'teki
>   tabloya bak.
> - 14 jog tuş kodları **hardcoded** (bölüm 3.4 tablosu);
>   `defaultKeyMap()` döndürür.
> - CodeSys node id formatı: `ns=4;s=|var|MAT LC-C07.Application.<GVL>.<Var>`
> - Devamında **Phase 2** (jog komutları PLC'ye gönderme, frame
>   editör, program list, watchdog). [`PLC_INTEGRATION.md`](PLC_INTEGRATION.md)
>   güncel.
> - Tüm `.ai/` dökümanları güncel.
