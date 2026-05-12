# Phase 1 — Genişletilmiş Test Planı (Vendor Demolarıyla Karşılaştırmalı)

> **Strateji:** Her donanım için vendor'ın çalışan referans demosunu
> cross-compile edip cihaza koymak, sonra **kendi** Diagnostics ekranımızla
> **paralel** çalıştırarak sonuçları karşılaştırmak. Vendor demosu
> oracle (kesin doğru cevap), bizim app değerlendirilmesi gereken.

## 0. Ön Koşullar

- ✅ Docker Focal-arm64 builder image hazır (`smb-q6r-builder:focal-arm64`)
- ✅ SSH erişimi (Tronlong@192.168.1.245, boş parola)
- ✅ deploy.sh + docker-build.sh çalışıyor
- ⏳ Vendor HWInterfaceDemo'ları bizim CMake pipeline'ında derlenmeli

## 1. Vendor Demo Cross-Compile Stratejisi

Her vendor demo bir Qt 5.12 Widgets uygulaması, `.pro` dosyası ve 3-4 C++
kaynağı içerir. Bizim CMake-merkezli sistemimize uydurmak için yedi mini
proje aynı CMakeLists'e ek hedefler olarak eklenir:

```
SMB-Q6R/
├── CMakeLists.txt                — ana smb_q6r hedefi
└── vendor-demos/                  — yeni
    ├── CMakeLists.txt             — alt hedefler (subdirectory)
    ├── led/                       — vendor led demo + CMake adaptasyonu
    ├── button/
    ├── matrixkeys/
    ├── pwm/
    ├── rotaryencoder/
    ├── backlight/
    └── wheel/
```

Her vendor demo bizim Docker image'ımızla derlenir, **ayrı binary'ler**
üretir (`smbq_vendor_led`, `smbq_vendor_button`, vs.), aynı strip + deploy
akışıyla cihaza taşınır.

**Üretilen artifacts:**
- `build-arm64/smb_q6r.stripped` — bizim uygulama (~27 KB)
- `build-arm64/vendor/led.stripped` — vendor LED demo (~15 KB)
- vd. Tek deploy script tümünü cihaza atar.

## 2. Karşılaştırma Matrisi (Cross-Validation)

Her donanım için 3 sütun: **vendor sonucu**, **bizim sonucumuz**, **eşleşme?**

### 2.1 LED Test

| Vendor (`smbq_vendor_led`) buton | Vendor'ın çağırdığı ioctl | Fiziksel LED       | Bizim DiagnosticsModel.setLed(N) | Bizim çağırdığımız ioctl | Fiziksel LED | Eşleşme |
|-----------------------------------|----------------------------|---------------------|----------------------------------|---------------------------|---------------|---------|
| led1                              | ioctl(fd, 1/0, 3)         | (gözlem)             | setLed(3, true)                  | ioctl(fd, 1, 3)           | (gözlem)      | ?       |
| led2                              | ioctl(fd, 1/0, 4)         |                      | setLed(4, true)                  | ioctl(fd, 1, 4)           |               | ?       |
| led3                              | ioctl(fd, 1/0, 2)         |                      | setLed(2, true)                  | ioctl(fd, 1, 2)           |               | ?       |
| led4                              | ioctl(fd, 1/0, 1)         |                      | setLed(1, true)                  | ioctl(fd, 1, 1)           |               | ?       |
| led5                              | ioctl(fd, 1/0, 0)         |                      | setLed(0, true)                  | ioctl(fd, 1, 0)           |               | ?       |

Sonuç: **Port → Fiziksel LED haritası**. ENGINEERING_LOG.md'ye yazılır:
```
HN00-09Q6 #1 (serial 74708edcccad9d45):
  port 0 → STOP (red)
  port 1 → SERVO (green)
  port 2 → ENABLE (green)
  port 3 → unused / not present
  port 4 → unused / not present
```
*(yukarıdaki örnek; gerçek harita fiziksel testle çıkar)*

### 2.2 Enable Switch (S1/S2)

| Fiziksel durum                 | Vendor `smbq_vendor_button` çıktısı (8 char) | Bizim `enableS1`, `enableS2` |
|--------------------------------|----------------------------------------------|------------------------------|
| Switch tamamen serbest (üst)   | "00000000" beklenir                          | (false, false)               |
| Orta konuma kibarca bas        | "00000011" veya "00000010"                   | (true, true) ?                |
| Tam basılı (panic)             | "00000000" veya "00000001"                   | (false, false) ?              |

Üç durumun **gerçek** byte değerleri kayıt edilir. Hangi byte deseninin
hangi konuma karşılık geldiği bilim deneyi gibi çıkartılır. Çıktı:

> **HN00-09Q6 Enable Switch tablosu**
> | byte[7..0] | Konum  | Aksiyon                  |
> |------------|--------|--------------------------|
> | (gözlem)   | OFF    | Servo enable yasak       |
> | (gözlem)   | ENABLED| Servo enable izinli      |
> | (gözlem)   | PANIC  | E-Stop benzeri kilitle   |

### 2.3 Mode Switch (3-konum Anahtar)

Aynı yöntem; vendor `smbq_vendor_rotaryencoder` ile.

| Konum  | Vendor byte deseni (8 char) | Bizim `mode` özelliği |
|--------|-----------------------------|------------------------|
| AUTO   | "00010000" beklenir (bit 3) | "Auto"                 |
| MANUAL | "00001000" beklenir (bit 4) | "Manual"               |
| STOP   | "00000100" beklenir (bit 5) | "Stop"                 |

### 2.4 Matrix Keypad (14 buton)

Vendor `smbq_vendor_matrixkeys` her tuş basışında Linux `input_event.code`
gösterir. Her 14 fiziksel butonu sırasıyla bas, gözlenen kodu kaydet.

Sonuç tablosu (örnek format):
```
Fiziksel konum (üreticinin numarası)  →  Linux KEY_*           kod
[Top-Left, col 1, row 1]              →  KEY_F1                59
[Top-Left, col 1, row 2]              →  KEY_F2                60
...
```

Bu tablo bizim `KeypadPage`'in fiziksel düzenini doğru çizmesi için lazım.
HT0804 manualinde 21 buton listelenmiş ama HN00-09Q6 14 buton; haritalama
farklı olabilir.

### 2.5 Buzzer (PWM)

| Vendor demo aksiyonu            | Bizim                                |
|---------------------------------|--------------------------------------|
| ioctl(fd, 0, 0) — sustur        | `model.silenceBuzzer()`              |
| ioctl(fd, 1, 1) — sürekli aç     | `model.holdBuzzer(true)`             |
| ioctl(fd, 3, 200) — 200ms bip    | `model.beep(200)`                    |

Sesli karşılaştırma: aynı çağrıya iki uygulama aynı sesi mi üretiyor?

### 2.6 Backlight

| Vendor demo değeri           | Ekran parlaklığı (gözlem)  | Bizim slider değeri | Ekran parlaklığı |
|-------------------------------|----------------------------|----------------------|--------------------|
| 0                             | Tam kapalı (kör)           | 0                    |                     |
| 25                            | Düşük                      | 25                   |                     |
| 50                            | Orta                       | 50                   |                     |
| 100                           | Tam parlak                 | 100                  |                     |

### 2.7 Wheel (Handwheel)

Vendor `smbq_vendor_wheel` her tıkta `QWheelEvent::angleDelta()` değerini
gösterir. Bizim app aynı değeri gösterir. **Yönün** ne ifade ettiğini
fiziksel test belirler:
- Saat yönünde döndürme → `delta > 0` mi `< 0` mı?
- Bir "tık" = kaç birim delta?

## 3. Test Akışı (Adım Adım)

### Adım 1 — Vendor demoları cross-compile et + deploy
```bash
# tek komut, hem bizim hem vendor build'leri
scripts/docker-build.sh --with-vendor-demos
scripts/deploy.sh --include-vendor
```

### Adım 2 — Cihazda yan yana
Cihaz ekranında pencere yöneticisi (xfce4) ile bizim app ve vendor demosunu
**yan yana** yerleştir:
- Bizim: 1280×400 üst yarı
- Vendor: 1280×400 alt yarı

Bu görsel karşılaştırma için ideal.

### Adım 3 — Her donanım için 6 dakika
1. Vendor demo'sunu kapat, sadece bizim app'i bırak.
2. Fiziksel kontrolü (örn. LED port 0 toggle) yap. Sonucu kaydet.
3. Vendor demo'sunu aç, bizim app'i kapat.
4. Aynı kontrolü vendor üzerinden yap. Sonucu kaydet.
5. Eşleşme analizi. ENGINEERING_LOG.md'de **karşılaştırma tablosu**.

### Adım 4 — Anormallik raporu
Herhangi bir sapma varsa:
- "Sapma var" → kök neden analizi → bizim kodda hata mı, vendor mu, donanım mı?
- "Hep sapma var" → driver davranışı belge edilmemiş olabilir, AKIM METAL OEM
  kanalıyla vendor'a sorulur.

## 4. Kabul Kriterleri (genişletilmiş)

Phase 1 sonu için orijinal 10 kriterin üzerine:

11. ✅ Vendor LED demosu ile bizim LED testimiz **aynı fiziksel LED'i** yakıyor
    (tüm 5 port için).
12. ✅ Vendor button demosu ile bizim Enable Switch okumamız **aynı byte
    deseni** veriyor.
13. ✅ Vendor rotaryEncoder demosu ile bizim Mode Switch okumamız aynı.
14. ✅ Vendor matrixKeys demosu ile bizim Keypad listener'ımız aynı key
    code'ları yakalıyor.
15. ✅ Vendor backlight demosu ile bizim slider aynı değerde aynı parlaklığı
    veriyor.
16. ✅ Vendor wheel demosu ile bizim wheel handler aynı delta yönü
    kullanıyor (CW = +1 mı -1 mı).
17. ✅ Vendor pwm demosu ile bizim buzzer aynı duration için aynı süre
    bipliyor (kulak ile).
18. ✅ ENGINEERING_LOG.md'de "HN00-09Q6 #1 (serial: ...) Phase 1 closeout"
    entry'sinde tüm haritalamalar yazılı.

## 5. Çıktılar (Phase 1 sonu)

- `vendor-demos/` dizini (CMake adapte edilmiş 7 vendor demo)
- `build-arm64/vendor/*.stripped` (deploy edilebilir vendor binary'leri)
- `ENGINEERING_LOG.md`'de comprehensive mapping table
- `src/hwio.{h,cpp}` + 5 controller (LED, Buzzer, Switch, Backlight, SystemInfo)
- `src/diagnostics_model.{h,cpp}` (QML bridge)
- `qml/Main.qml` + 7 sayfa (tabbar ile)
- `docs/superpowers/specs/2026-05-11-phase1-diagnostics-design.md` (güncellenmiş)

## 6. Tahmini Süre

- Vendor demo CMake adaptasyonu: 1 gün
- Vendor demo deploy + ilk çalıştırma: 0.5 gün
- 7 donanım için karşılaştırmalı test: 1 gün (gerçek el aktivite)
- Bulguların ENGINEERING_LOG'a yazılması: 0.5 gün
- Bizim Iter B..G implementasyonu: 4-5 gün
- **Toplam:** 7-8 gün (1.5 hafta)

Orijinal Phase 1 (sadece bizim app) için 2-3 hafta plan vardı, daha gerçekçi
olarak vendor karşılaştırması eklenince **1.5 hafta** sınırına çekiyoruz.
Sebep: vendor demo'su zaten **doğrulanmış**, biz onunla **kıyaslama** yapıyoruz —
sıfırdan donanım keşfetmek yerine.
