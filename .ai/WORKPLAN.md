# SMB-Q6R Çalışma Planı

> Fazlar artımlıdır; her fazın çıktısı tek başına çalışan bir cihaz durumu olmalıdır.
> "Done" tanımı her fazın altındadır. Kabul kriteri sağlanmadan sonraki faza geçilmez.

---

## Faz 0 — Bilgi Toplama ve Altyapı  (mevcut)

**Amaç:** Donanımı, cihazı ve geliştirme bağlamını eksiksiz anlamak;
takım çalışmasının temelini atmak.

**Yapılacaklar**
- [x] HN00-09Q6 datasheet'ini özetle (`HARDWARE_SPEC.md`)
- [x] QT Teach pendant manualini özetle (`HARDWARE_API.md`)
- [x] HWInterfaceDemo kaynaklarını analiz et
- [x] Cihaza SSH bağlan, gerçek runtime'ı haritala (`DEVICE_RUNTIME.md`)
- [x] `.ai/` doküman setini yaz
- [ ] Ekranda örnek dosya yolları aç (kullanıcı isteği)
- [ ] `.claude/settings.json` ve `.gitignore` oluştur
- [ ] Git repo başlat (`git init` + ilk commit)

**Bitiş Kriteri**
- `.ai/` dizini eksiksiz, cihaz erişimi belgelenmiş, repo başlamış.

---

## Faz 1 — Build Sistemi + "Hello Pendant"

**Amaç:** En basit Qt uygulamasını cihaza dağıtıp çalıştırmak.
Tüm fiziksel donanımı (LED, buzzer, butonlar, enable, mode switch)
tek bir test ekranında göstermek.

**Yapılacaklar**
- CMake iskeleti (`CMakeLists.txt`, `inc/`, `src/`, `ui/`)
- `cmake/aarch64-toolchain.cmake` (cross için, opsiyonel)
- `scripts/deploy.sh` (build + scp)
- `HwIo` singleton:
  - LED kontrolü (set / blink)
  - Buzzer (kısa bip + sürekli)
  - Enable Switch okuma (S1/S2 sinyali)
  - Mode Switch okuma (Auto/Manual/Stop)
  - Backlight (slider 0..100)
- "DiagnosticsWindow" — tüm donanımı tek ekranda canlı göster
- Fiziksel test logu: LED port haritası, enable switch davranışı,
  mode switch debounce süresi → `ENGINEERING_LOG.md`'ye

**Bitiş Kriteri**
- Uygulama açılıyor, dokunmatik çalışıyor, her donanım girişi UI'da görünüyor,
  LED ve buzzer butonla kontrol edilebiliyor.

**Tahmini Süre:** 2–3 hafta

---

## Faz 2 — PLC Haberleşme

**Amaç:** PLC ile iletişimi kurmak; bağlantı kaybı, heartbeat, alarm
yayılımı işliyor olmak.

**Yapılacaklar**
- PLC dokümanı + sembol haritası edin (CodeSys projesini PLC'den çek)
- Protokol seçimi: Modbus TCP (varsayılan) veya OPC UA
- `PlcLink` sınıfı (Qt-thread tabanlı, sinyal/slot ile UI'ya iletir):
  - `connect(host, port)`
  - `disconnect()`
  - `readU16(address) → quint16`
  - `writeU16(address, value)`
  - `readFloat32(address) → float`
  - `subscribeChanges(address_range)` — sürekli okuma
- Heartbeat thread (100 ms periyot, monoton counter)
- Bağlantı durum widget'i (üst bar) — yeşil/sarı/kırmızı

**Bitiş Kriteri**
- Cihaz PLC ile heartbeat alıyor/veriyor, bağlantı kaybında UI uyarıyor,
  PLC'ye yazılan bir test değeri sembol monitöründe görünüyor.

**Tahmini Süre:** 3–4 hafta

---

## Faz 3 — Güvenlik Altsistemi

**Amaç:** E-Stop, Enable, Mode geçişlerinin yazılım tarafında doğru
modellenmesi; ihlallerin UI'da ve PLC'ye doğru yansıması.

**Yapılacaklar**
- `SafetyMonitor` sınıfı (dedicated thread, RT priority)
- Mod geçiş state machine: Stop → Manual → Auto, ihlallerde kilitle
- "Servo On/Off" buton — Enable Switch + Mode tutarlılığını şart koşar
- E-Stop overlay (modal, kırmızı, kapatılamaz; PLC reset ile geçer)
- Bağlantı kaybı + güvenlik ihlali kompozit alarm tablosu

**Bitiş Kriteri**
- E-Stop basıldığında uygulama kilitleniyor, çözüldüğünde reset gerekli,
  Mode değişimi UI'da hemen yansıyor, jog hakimiyeti yalnızca Manual'da
  ve Enable orta konumdayken veriliyor.

**Tahmini Süre:** 2 hafta

---

## Faz 4 — Jog UI

**Amaç:** Operatörün robotu güvenli şekilde elle hareket ettirebilmesi.

**Yapılacaklar**
- Joint Jog ekranı (J1..J6, +/- butonları, hız slider)
- Cartesian Jog ekranı (X, Y, Z, Rx, Ry, Rz)
- Frame seçici (World, Tool, User)
- Handwheel modu: jog wheel her tıkta küçük artımsal hareket
- Hız hassasiyet seçici (kaba / orta / hassas)
- Live pozisyon görüntüsü (eklem + Cartesian)
- (Opsiyonel) basit kinematik önizleme (sade çizgi diyagramı)

**Bitiş Kriteri**
- Robot tüm modlarda jog edilebiliyor, hız limitleri çalışıyor,
  E-Stop / Enable serbest bırakma anında jog duruyor.

**Tahmini Süre:** 4 hafta

---

## Faz 5 — IO Ekranı + Sistem Monitör

**Amaç:** PLC ve robot durumunun şeffaflığı.

**Yapılacaklar**
- DI/DO listesi (filtre, manuel force — yalnızca Manual mode)
- Servo durum, alarm aktif/pasif tablosu
- Log akışı (uygulama + PLC olayları)
- Sistem bilgisi widget'ı (CPU, RAM, uptime, IP)

**Bitiş Kriteri**
- Tüm PLC I/O görünüyor, alarmlar canlı geliyor, log dosyaya yazılıyor.

**Tahmini Süre:** 2–3 hafta

---

## Faz 6 — Program Editörü

**Amaç:** Robot programları yazma, kayıt, oynatma.

**Yapılacaklar**
- Program listesi (dosya yöneticisi tarzı)
- Komut tabanlı editör (insert, delete, edit step)
- Komut paleti: J move, L move, C move, WAIT, IF, CALL, LBL/JMP, IO SET
- Pozisyon kayıt (mevcut pozisyondan P[n] kaydet)
- PLC'ye program yükle / başlat / step / durdur

**Bitiş Kriteri**
- Programlar oluşturulabiliyor, USB'ye yedeklenebiliyor, PLC'ye
  yüklenip çalıştırılabiliyor.

**Tahmini Süre:** 6 hafta

---

## Faz 7 — Frame Editörü ve Kalibrasyon

**Amaç:** TCP, User Frame, Base Frame tanımlama sihirbazları.

**Yapılacaklar**
- TCP 4-point sihirbazı
- User Frame 3-point sihirbazı
- Manuel girilebilir frame tablosu
- Frame'i PLC'ye yazma + persist (dosya + EEPROM)

**Tahmini Süre:** 3 hafta

---

## Faz 8 — Üretim Hazırlığı

**Yapılacaklar**
- systemd servis dosyası, otomatik başlat
- Kiosk modu (XFCE oturum, sadece bizim uygulama)
- Dokunmatik gizli imleç (`QT_QPA_FB_HIDECURSOR=1`)
- Boot logo (`/run/media/mmcblk1p1/logo.bmp`)
- Watchdog: uygulama crash → systemd otomatik restart
- Versiyon yönetimi (uygulama içi "Hakkında" + git tag → versiyon)
- Kullanıcı kılavuzu (Türkçe) — `docs/USER_MANUAL.md`

**Tahmini Süre:** 3 hafta

---

## Toplam Tahmini

~ 6 ay full-time, donanım gecikmeleri ve PLC entegrasyon sürprizleri
dahil değil. Realistic hedef: **8–10 ay**.
