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

## Faz 1 — Build Sistemi + Donanım Diagnostiği  ✅ TAMAMLANDI (+Faz 1.5)

**Amaç:** Cross-compile pipeline'ı tamamlamak ve cihazın tüm donanımını
**vendor referans demosuyla karşılaştırmalı** doğrulamak. Phase 1 sonu
itibarıyla bizim app fiziksel donanımı vendor'ın kendi demolarıyla 1:1
aynı şekilde yönetebiliyor olmalı.

> **Phase 1.5 ek:** Donanım haritalama bittikten sonra OPC UA istemcisi
> de aynı plan altında eklendi. Detay: [PHASE1_STATUS.md](PHASE1_STATUS.md)
> ve [PLC_INTEGRATION.md](PLC_INTEGRATION.md).

**Yapılanlar (2026-05-11 → 2026-05-12)**
- ✅ Docker Focal-arm64 builder image (`smb-q6r-builder:focal-arm64`)
- ✅ CMake + cmake/aarch64-linux-gnu.cmake toolchain (Noble artığı,
     deprecated; Docker yolu kullanılıyor)
- ✅ `scripts/docker-build.sh` + `scripts/deploy.sh` + `scripts/device-permissions.sh`
- ✅ HwIo singleton + 5 subsystem (LED, Switch, Buzzer, Backlight, MatrixKeys)
- ✅ Vendor HWInterfaceDemo'ların tümü CMake'e adapte + cross-compile
     edildi (`vendor-demos/`); cihaza deploy edilmiş — oracle olarak hazır
- ✅ Iter B/C/D/E **birleşik** tek sayfalı UI olarak teslim edildi
- ✅ Light-theme ABB FlexPendant esinli arayüz; realistic key tasarımı
- ✅ Kiosk fullscreen; FramelessWindowHint XFCE'de görünmez yapıyordu, kaldırıldı
- ✅ Mode bit swap (HN00-09Q6 vendor docs'tan farklı: bit 4=Auto)
- ✅ udev permission setup (plugdev grubu)
- ✅ GitHub remote `git@github.com:zcakar/SMB-Q6R.git` (push edildi)

**Phase 1 Closeout (2026-05-22 → 2026-06-02)**
- ✅ LED port → fiziksel LED haritası: 0=ENABLE, 1=SERVO, 2=STOP (canlı doğrulandı)
- ✅ Matrix key 14 KEY_* kodu live-capture sonrası **hardcoded** baked-in
- ✅ DejaVu Sans QRC'ye gömüldü (tofu glyph fix)
- ✅ systemd --user service + autostart + cold-boot test geçti
- ✅ Sim mode bezel (host-side virtual pendant)
- ✅ Tab UI: HARDWARE + PLC CONSOLE
- ✅ Network alias 192.168.0.245/24 netplan persistent

**Phase 1.5 — OPC UA Bootstrap (2026-05-22 → 2026-06-02)**
- ✅ open62541 v1.3.10 LTS, statik link via FetchContent
- ✅ PlcLink worker QThread + queued signals
- ✅ Connect/Disconnect + namespace browse + subscribe + read/write
- ✅ Live bağlantı doğrulandı: opc.tcp://192.168.0.2:4840
- ✅ CodeSys node ID formatı keşfedildi:
     `ns=4;s=|var|MAT LC-C07.Application.<GVL>.<Var>`
- ✅ PlcPage: tablo + add form + log + 12 default watch

**Bekleyen (deferred)**
- ⏳ Iter F: Wheel handler `/dev/input/event1` (rotary) — Phase 4 jog ile birleşecek
- ⏳ Iter G: SystemInfo widget — düşük öncelik
- ⏳ PLC heartbeat → Phase 2 başında
- ⏳ Reconnect-on-disconnect → Phase 2 başında

**Bitiş Kriteri (18 madde)**
1. Cross-compile pipeline tekrarlanabilir
2. App açılır, 7 sekme görünür, dokunmatik çalışır
3-10. Her donanım için kendi uygulamamız bekleneni yapar
11-17. **Karşılaştırma:** Her donanım için vendor demosuyla bizim app aynı
       fiziksel sonucu üretir (LED port haritası, key code haritası,
       switch byte deseni, vd. — `PHASE1_TEST_PLAN.md` §4)
18. ENGINEERING_LOG.md'de "HN00-09Q6 #1 Phase 1 closeout" entry'sinde tüm
    haritalamalar belgelenmiş

**Tahmini Süre:** 1.5–2 hafta (vendor karşılaştırma sayesinde keşif maliyeti azalır)

**İlgili dokümanlar**
- [`SDK_INVENTORY.md`](SDK_INVENTORY.md) — vendor kaynaklarının değer analizi
- [`PHASE1_TEST_PLAN.md`](PHASE1_TEST_PLAN.md) — karşılaştırma matris ve adımlar
- [`DEVICE_DEPLOY_NOTES.md`](DEVICE_DEPLOY_NOTES.md) — paket gereksinimleri

---

## Faz 2 — PLC Haberleşme  🟡 KISMEN (1.5 bootstrap'te scaffolding bitti)

**Amaç:** PLC ile iletişimi kurmak; bağlantı kaybı, heartbeat, alarm
yayılımı işliyor olmak.

**Phase 1.5 sırasında erken tamamlanan**
- ✅ Protokol seçimi: **OPC UA** (open62541 v1.3.10 statik)
- ✅ PlcLink Qt-thread tabanlı, sinyal/slot ile UI'ya iletir:
  - `connectToServer(url)` / `disconnectFromServer()`
  - `readNode(nodeId)` / `subscribeNode(nodeId)` / `writeNode(nodeId, QVariant)`
  - `browseNamespace(maxDepth, maxNodes)`
- ✅ Bağlantı durum widget'i (PLC tab connectBar + header'da PLC pill)
- ✅ PLC ile canlı session: opc.tcp://192.168.0.2:4840 (`SessionState: Activated`)
- ✅ CodeSys symbols XML alındı, node ID formatı çözüldü

**Phase 2'de yapılacaklar**
- ⏳ Heartbeat (100 ms periyot, monoton counter, `GVL.PendantHeartbeat` ↔ `GVL.PlcHeartbeat`)
- ⏳ Reconnect-on-disconnect (exponential backoff)
- ⏳ Jog komutları: pendant fiziksel butonu → PLC `GVL_Control_Var.jog_Negative_X` vb.
- ⏳ Mode switch state writeback → `GVL.Sys_Mode`
- ⏳ LED durumu PLC'den subscribe (Enable/Servo/Stop sinyalleri)
- ⏳ Joint position display (PLC'den live X/Y/Z + Cartesian)
- ⏳ Alarm view (PLC alarm GVL listesi)

**Bitiş Kriteri**
- Cihaz PLC ile heartbeat alıyor/veriyor, bağlantı kaybında UI uyarıyor,
  bir fiziksel jog butonuna basınca PLC değişkeni gerçekten değişiyor.

**Tahmini Süre:** 2–3 hafta (scaffolding hazır olduğu için kısaltıldı)

**İlgili doküman:** [`PLC_INTEGRATION.md`](PLC_INTEGRATION.md) — node ID formatı,
PlcLink API, default watch listesi, watchdog planı.

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
- [x] systemd servis dosyası, otomatik başlat — `scripts/systemd/smb-q6r.service` + `scripts/install-autostart.sh` (2026-05-13)
- [x] Kiosk modu (XFCE oturum, sadece bizim uygulama) — showFullScreen() + systemd
- [ ] Dokunmatik gizli imleç (`QT_QPA_FB_HIDECURSOR=1`)
- [ ] Boot logo (`/run/media/mmcblk1p1/logo.bmp`)
- [x] Watchdog: uygulama crash → systemd otomatik restart — `Restart=on-failure` + 5/60s burst cap
- [ ] PLC↔pendant heartbeat (Phase 2 ile birlikte yapılacak)
- [ ] Versiyon yönetimi (uygulama içi "Hakkında" + git tag → versiyon)
- [ ] Kullanıcı kılavuzu (Türkçe) — `docs/USER_MANUAL.md`

**Tahmini Süre:** 3 hafta (yarısı erken bitti — Phase 1.5 OPC UA bootstrap sırasında autostart + crash watchdog tamamlandı)

---

## Toplam Tahmini

~ 6 ay full-time, donanım gecikmeleri ve PLC entegrasyon sürprizleri
dahil değil. Realistic hedef: **8–10 ay**.
