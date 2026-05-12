# Mühendislik Günlüğü

> Kararlar, sürpriz bulgular, çözülen sorunlar. Kronolojik (yeni → eski).

---

## 2026-05-12 (sonu) — Phase 1 Closeout: Persistence + 2-state Deadman

### Yapıldı

- **Persistent key map** eklendi: `src/key_map_store.{h,cpp}`,
  `DiagnosticsModel::{loadKeyMap, saveKeyMap, clearKeyMap}`. Cihazda
  `~/.smb-q6r/keymap.dat` (her satırda bir kod, -1 = boş, atomic write
  via .tmp + rename). Her tuş basışından sonra otomatik kaydedilir,
  açılışta yüklenir. Reset map butonu dosyayı da siler.
- **Pre-seed:** Kullanıcının 7 mapped key'i (fotoğraf 1'den okundu)
  cihaza önceden yazıldı; yeni binary açılışta okur ve kullanıcı kaldığı
  yerden devam eder.
- **Guided learn**: auto-learn'ün karışıklığı (kullanıcı J2+ basıyor,
  cell J4+ vuruluyor) çözüldü. Sarı vurgulu cell hangi butona basılacak
  diyor; basılınca o cell'e yazıyor, sonraki boş cell'e geçiyor.
- **Deadman 2-state**: Kullanıcı testi — sert basınca bile S2 kapanmıyor.
  Datasheet 3-stage der ama driver yalnız 2 durumu üretir.
  - !S1 && !S2 → RELEASED
  - S1 || S2  → ACTIVE
  - PANIC stage UI'dan çıkarıldı; küçük notta "Bu donanımda panic
    sürücü tarafından üretilmiyor" diyor.

### Phase 1 Kapsam (Tamam)

- ✅ 5 hwio subsystem (LED, switch, buzzer, backlight, matrix keys)
- ✅ Light-theme UI (ABB FlexPendant esinli)
- ✅ Realistic matrix key tasarımı (bezel + dark face)
- ✅ Persistent key map (~/.smb-q6r/keymap.dat)
- ✅ Cross-compile Docker pipeline
- ✅ udev permission setup
- ✅ Vendor demoları yan yana referans olarak deploy edildi
- ✅ GitHub: zcakar/SMB-Q6R, ~25 commit
- ✅ `.ai/` doküman seti (PHASE1_STATUS.md, ENGINEERING_LOG.md,
     DEVICE_RUNTIME.md, WORKPLAN.md, vd. güncel)

### Henüz Yapılmadı (Phase 1 Closeout Sonrası)

- Iter F — Jog wheel handler (/dev/input/event1)
- Iter G — System info widget
- CNC DT550 RAR çıkar (Phase 2 hazırlık; `apt install p7zip-full` host'ta lazım)

---

## 2026-05-12 — Phase 1 UI Rewrite + GitHub

### HN00-09Q6 LED Haritalama — DOĞRULANMIŞ

Fiziksel test (kullanıcı 2026-05-12):

| Port | Fiziksel LED  | Renk    |
|------|---------------|---------|
| 0    | **ENABLE**    | Yeşil   |
| 1    | **SERVO**     | Yeşil   |
| 2    | **STOP**      | Kırmızı |
| 3    | (yok)         | —       |
| 4    | EINVAL        | (driver reddediyor) |

Datasheet'in "From left to right: Stop (Red), Servo (Green), Enable
(Green)" sıralaması fiziksel paneldedir; **port numarası ile fiziksel
soldan-sağa sıra ters**. `LedController::set(port, on)` çağrılarında
yukarıdaki tablo bağlayıcıdır.

### Deadman Switch Bit Haritalama — DOĞRULANMIŞ

Vendor docs: `bit 6 = S2, bit 7 = S1` (right-most chars).
Gerçek: `buf[0] = S1, buf[1] = S2` (left-most chars). Vendor'ın kendi
button.cpp'si de buf[0] ve buf[1]'i okuyor. Pressing the deadman flips
raw byte from `00000000` → `10000000`.



### Yapıldı

- **Light-theme UI rewrite:** Tüm QML dosyaları ABB FlexPendant / FANUC
  iPendant estetiğine geçirildi (white card + light gray title bar + dark
  text + ABB-blue accent line).
- **Realistic key visuals:** `KeyCell.qml` — outer light plastic bezel
  (gradient), inner dark key face (gradient), large white +/− glyph,
  axis label, code readout. Tıklamak cell'i temizliyor, sonraki basış
  oraya kaydolur.
- **Deadman terminology:** "Enable Switch" → "Deadman Switch" (endüstri
  standart adlandırma). C++ tarafı API (`enableS1/S2`) vendor driver
  adlandırmasıyla uyumlu kalsın diye değişmedi; yalnız UI string'leri
  değişti.
- **Kiosk fullscreen sabit:** `FramelessWindowHint` kaldırıldı — XFCE
  compositor'da override-redirect mode görünmez pencere üretiyordu.
  Saf `showFullScreen()` + `raise()` + `requestActivate()` kararlı çözüm.
- **GitHub remote:** `git@github.com:zcakar/SMB-Q6R.git` üzerine push.
  Bilgisayardaki SSH key zcakar olarak authenticate ediyor.

### Bulgular (Bugün doğrulanan)

- **`/dev/buttons` driver edge-only:** Open + select(1s) + blocking read(2s)
  testleri yapıldı; freshly opened fd'de **state push yok**, yalnız
  transition push'lar var. → UI'da "anahtarı bir kez oynatın" prompt'u
  açılışta normal davranış.
- **Mode bit swap doğrulandı:** Vendor manuali bit 3=Auto / bit 4=Manual
  diyor; HN00-09Q6'da fiziksel "Auto" pozisyonu bit 4'ü set ediyor.
  `SwitchMonitor` artık swapped yorumla.
- **GLIBC_2.34 sembolu:** Host'un Noble 24.04 glibc 2.39'undan compile
  edilince binary `__libc_start_main@GLIBC_2.34` çağrısı ekliyor; cihaz
  glibc 2.31. Docker Focal-arm64 imajıyla build geçti.
- **Vendor `lyx_appDemo`** sistem Qt 5.12.8'e link'lenmiş (vendor 5.15.10
  ayrı yolda, `/etc/profile.d/qt_env.sh` source edilince devreye giriyor).
  Bu yüzden bizim binary'leri sistem Qt 5.12.8'le çalıştırmak doğru tercih.
- **`component` keyword Qt 5.12'de yok:** Inline component declaration
  parse hatası verir; her component ayrı `.qml` dosyasına çıkarıldı.
- **`model` Repeater scope'unda rezerve:** C++ context property `model`
  adıyla expose edilince Repeater delegate body'lerinde shadow ediliyor;
  property adını `diag` yaptık.

### Kararlar

- **Light theme final:** Karanlık mod kullanılmayacak (operatör ortamında
  vardiyalar günışığında).
- **Vendor 5.15.10 kullanılmayacak:** ABI riski yüzünden sistem 5.12.8'de
  kalıyoruz; ileride Qt 5.15-only feature gerekirse Docker imajını
  Jammy'ye geçirmeyi değerlendiririz (glibc 2.35 ile yine cihazla
  uyumsuz olabilir — alternatif yol vendor 5.15.10'a köprü kurmak).
- **Auto-learn matrix mapping:** Fiziksel butona bas → kod sıradaki boş
  cell'e otomatik yazılır. Tap-then-press paradigması kaldırıldı.
- **Cihaza paket kurma yok:** UI yalnız `qml-module-qtquick2` kullanıyor.

---

## 2026-05-11 — Phase 0 + Phase 1 Iter A-E Drive

### Yapıldı

- `.ai/` doküman seti kuruldu (CADNC şablonu esinli)
- Cihaza SSH + device map (15 input device, dpkg listesi)
- Docker `smb-q6r-builder:focal-arm64` imajı build edildi (Qt 5.12.8,
  glibc 2.31, crossbuild-essential-arm64, ninja, cmake)
- HwIo singleton mimari: LedController, SwitchMonitor, BuzzerController,
  BacklightController, MatrixKeysMonitor
- Tüm 7 vendor `HWInterfaceDemo` Qt Widgets app'i bizim Docker pipeline'da
  cross-compile edilip `/home/Tronlong/vendor-demos/` altına deploy edildi
  (oracle / yan yana karşılaştırma için)
- udev rule + plugdev grubuyla `/dev/{leds,buttons,buttonstop,pwm}` ve
  `/dev/input/event*` Tronlong erişimine açıldı
- LED port range 0..3 confirme edildi (port 4 → EINVAL)
- Phase 1 testi tamamı tek ekranda: 4 LED tile, mode switch, deadman,
  buzzer, backlight, matrix keys, history

### Bulgular

- **Cihaz:** Lavichip HN00-09Q6, device code **MAT-QT-TP-PC10C-Q6-UBT-L1**,
  RK3568 + Ubuntu 20.04 + Linux 5.10.209-rt89 PREEMPT_RT, hostname
  `langyuxin`
- **SSH:** `Tronlong` / boş parola (not `root` / `1234` from old manual)
- **Sudo:** NOPASSWD (`echo "" | sudo -S` çalışıyor)
- **Vendor Qt 5.15.10:** `/usr/lib/qt-5.15.10/` yalnızca runtime, dev
  headers yok, qmake yok
- **Apache2** cihazda çalışıyor (port 80 default page)
- **Telnet** açık (port 23)
- **vendor demo HMI** (`demoV1.2.0`) **6 eksen robot için DEĞİL** —
  enjeksiyon kalıp pick-and-place gantry için (action codes "X2 Fore",
  "Mold Close", "CorePuller"). UI mimarisi referans, action sözlüğü kullanılamaz.
- **Türkçe çeviri vendor'da var:** `turkey.qm` + 8 ini dosyası — format
  alınabilir, içerik kalıp robot içeriyor

### Sürpriz Sorunlar (Çözüldü)

- Eski manual SDK Qt 5.12.9 + kernel 4.9 + TI Sitara AM335x içindi;
  HN00-09Q6 farklı SoC (RK3568) ve daha yeni stack — manual genel olarak
  yanıltıcı; hardware API'leri (/dev/*) doğru kalmış
- `chmod +x` kabuk komut izinleri Claude Code session'da kararsız
- Bash `pkill -f /path/...` SSH session'da kendi parent shell'ini
  öldürüyor (pkill kendi cmdline'ında pattern arıyor) → `killall basename`

### Kararlar

- **Cross-compile Docker (Focal-arm64) seçildi** (native build cihazda
  yapılamaz, Noble host glibc uyumsuz)
- **Qt Widgets değil, QML** seçildi (kullanıcı tercihi); ama cihazın
  qml-module setine sığacak şekilde sadece QtQuick 2 primitives
- **Auto-learn key mapping** seçildi (tap-then-press değil)
- **Vendor demolar oracle olarak** korunacak — `vendor-demos/` ağacı

### Açık (henüz cevaplanmamış)

- LED port → fiziksel LED (STOP/SERVO/ENABLE) haritalaması — kullanıcı
  test edip söyleyecek
- 14 matrix key code'larının fiziksel pozisyona haritalaması — auto-learn
  ile fiziksel test sırasında yazılacak
- Mode switch'in kullanıcı tarafından hangi pozisyonda olduğunun
  ilk açılışta görünmesi — driver edge-only olduğu için sadece prompt
  ile çözülebiliyor
- Jog wheel (rotary, /dev/input/event1) için Iter F implementasyonu
- System info widget (Iter G)
- CNC DT550 RAR'ı çıkarma (host'a p7zip kurulması gerek)
