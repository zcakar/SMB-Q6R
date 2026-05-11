# SMB-Q6R — Start Here

> Bu dosyayı her oturum başında oku. Sonra `context.yaml`, `WORKPLAN.md` ve
> `ENGINEERING_LOG.md` dosyalarını sırasıyla incele.

## Proje Nedir?

**SMB-Q6R**, Şenzhen Lavichip **HN00-09Q6** endüstriyel Teach Pendant cihazı
üzerinde çalışan, **Fanuc 6R uyumlu 6 eksen kollaboratif robot** için kontrol
arayüzü ve programlama paneli. Düşük seviyeli hareket kontrolü ayrı bir
**CodeSys SMB-MAT-LC-C07** PLC tarafından yapılır; Teach Pendant yalnızca
yüksek seviye komut/durum köprüsüdür.

Önceki **MilCAD** (CAD/CAM) ve **SODOO** (Odoo iş zekâsı) projelerindeki
deneyim ile `.ai/` belgeleme şablonu burada da uygulanır.

## Sistem Bileşenleri

```
[Operatör] ↔ HN00-09Q6 (Teach Pendant) ↔ Ethernet ↔ SMB-MAT-LC-C07 (PLC) ↔ Servo Drv ↔ 6R Robot
   │              │                                       │
   │              │                                       └─→ EtherCAT veya analog
   │              └─→ Qt 5.15.10 UI (bu proje)
   └─→ E-Stop, Enable Switch, Mode Switch fiziksel kontrolü
```

## Donanım Özeti (HN00-09Q6)

- **SoC:** Rockchip RK3568 (4× Cortex-A53 1.2/1.8GHz, Mali-G52 MP2 GPU)
- **RAM:** 2 GB DDR4 / **eMMC:** 16 GB
- **Ekran:** 10.1" 1280×800 TFT + kapasitif çoklu dokunma (Ilitek)
- **OS:** Ubuntu 20.04.6 LTS (kernel 5.10.209 **PREEMPT_RT**)
- **Qt:** 5.15.10 (XCB / X11 platformu)
- **Network:** 100 Mb Ethernet (eth1) — varsayılan IP 192.168.1.245
- **Fiziksel:** 14 buton, E-Stop, 3-konum key switch, jog handwheel,
  3-stage Enable switch (S1/S2), 5 LED, buzzer
- **Sertifika:** UL / CE, IP65 ön panel, -20°C…60°C

## Mevcut Durum (2026-05-11)

- **Faz 0 (mevcut):** Bilgi toplama, `.ai/` altyapısı, cihaza SSH erişim sağlandı
- **Faz 1 (sıradaki):** Build sistemi (CMake) + ilk "Hello LED + Buzzer" uygulaması
- Cihazda kullanıcı: **`Tronlong`** (boş parola), uygulama yolu: `/userfs/app/`
- Mevcut tek uygulama: `/userfs/app/lyx_appDemo` (fabrika demo)

## Geliştirme Fazları

| Faz | Konu                                                   | Durum     |
|----:|--------------------------------------------------------|-----------|
| 0   | Bilgi toplama, .ai/ altyapı, cihaz keşfi                | Devam ed. |
| 1   | CMake build + cross-compile + "Hello Pendant" deneme    | Bekliyor  |
| 2   | PLC haberleşme katmanı (protokol seçimi + bağlantı)     | Bekliyor  |
| 3   | Güvenlik altsistemi (E-Stop, Enable, mode izleme)       | Bekliyor  |
| 4   | Jog UI — kartezyen + eklem modu, handwheel entegrasyonu | Bekliyor  |
| 5   | I/O ekranı + sistem monitör (servo durum, alarm, log)   | Bekliyor  |
| 6   | Program editörü (komut dizisi, kayıt/oynat, dosya I/O)  | Bekliyor  |
| 7   | Kalibrasyon, koordinat sistemleri, TCP/User Frame       | Bekliyor  |
| 8   | Üretim hazırlığı (kabuk, otomatik başlat, kilitleme)    | Bekliyor  |

Detay: [`WORKPLAN.md`](WORKPLAN.md)

## Kritik Tasarım Kararları

1. **Qt Widgets vs QML — Faz 1'de seçilecek.** XCB üzerindeyiz, ikisi de
   çalışır. QML daha modern; Widgets daha hızlı prototip.
2. **PLC protokolü** — Faz 2'de seçilecek. Adaylar:
   - **Modbus TCP** (basit, libmodbus mevcut)
   - **OPC UA** (modern, open62541)
   - **CodeSys ADS / TCP** (CodeSys-spesifik)
3. **Kinematik çözüm yeri** — PLC mi yoksa Pendant mı? Önerilen: konum/IK
   PLC tarafında, Pendant sadece komut göndericisi; ama Pendant'ta hafif
   bir önizleme kinematiği bulunabilir.

## İletişim Dili

- **Kod ve yorumlar:** English
- **Kullanıcı iletişimi:** Türkçe
- **Commit mesajları:** English, Conventional Commits formatı

## Sonra Okumak Lazım

1. [`HARDWARE_SPEC.md`](HARDWARE_SPEC.md) — HN00-09Q6 tam donanım belgesi
2. [`HARDWARE_API.md`](HARDWARE_API.md) — Linux cihaz dosyası API'leri
3. [`DEVICE_RUNTIME.md`](DEVICE_RUNTIME.md) — Cihazın canlı çalışma durumu
4. [`PLC_INTEGRATION.md`](PLC_INTEGRATION.md) — CodeSys SMB-MAT-LC-C07 notları
5. [`DEVELOPMENT_SETUP.md`](DEVELOPMENT_SETUP.md) — Build/deploy iş akışı
