# HN00-09Q6 — Donanım Şartnamesi

> Kaynak: `HN00-09Q6 en-US.pdf` (Lavichip V2.0)
> Doğrulama: cihaza canlı SSH ile yapılmıştır (2026-05-11)

## 1. Tanım

Endüstriyel ve kollaboratif robot uygulamaları için tasarlanmış 10.1"
çoklu-dokunma Teach Pendant. 3D animasyon görüntüleme, grafik programlama
ve sürükle-bırak komut dizisi düzenlemesini destekler.

## 2. Sistem

| Parametre        | Değer                                                       |
|------------------|-------------------------------------------------------------|
| SoC              | Rockchip RK3568 (rk805 PMIC + Mali GPU tespiti üzerinden)   |
| CPU              | Quad-core ARM Cortex-A53 @ 1.2 GHz nominal / 1.8 GHz max   |
| GPU              | Mali-G52 MP2, 700 MHz / 900 MHz max                          |
| RAM              | 2 GB DDR4                                                   |
| Depolama         | 16 GB eMMC (root partition 5.3 GB, kullanılabilir ~1.4 GB)  |
| RTC              | Yerleşik, pille beslemeli                                   |
| OS               | Ubuntu 20.04.6 LTS, Linux **5.10.209-rt89 PREEMPT_RT**      |
| Qt               | 5.15.10 (XCB platformu)                                     |

## 3. Ekran ve Dokunmatik

- 10.1 inch TFT, **1280 × 800** çözünürlük
- I2C kapasitif çoklu dokunma (Ilitek `ilitek_ts` sürücüsü, `/dev/input/event3`)
- Geri aydınlatma sysfs üzerinden: `/sys/class/backlight/backlight/brightness`
  (0–100 arası, 0 = kapalı)

## 4. Fiziksel Kontroller

### 4.1 Gösterge LED'leri (soldan sağa)

| Konum | İsim   | Renk    | Kullanım                        |
|-------|--------|---------|----------------------------------|
| L1    | STOP   | Kırmızı | Acil durdurma aktif / hata       |
| L2    | SERVO  | Yeşil   | Servo enerji altında             |
| L3    | ENABLE | Yeşil   | Enable switch orta konumda       |

Cihaz tarafında `/dev/leds` üzerinden, demo'da görülen LED port haritası
**HT0804 modeli** için 0–3 idi; HN00-09Q6'da fiziksel konum kontrolü
sırasında doğrulanmalı (Faz 1).

### 4.2 Acil Stop (Emergency Stop)

- Sarı etiketli kırmızı mantar buton
- Normal kapalı (NC), tek veya çift kontak seçeneği
- Saat yönünde çevirerek kilit açılır
- 16-pin konektör pin 15/16 → ST1/ST2

### 4.3 Enable Switch (Üç Konum)

- Dezaktif (üst) → Aktif (orta) → Panic (tam basılı)
- Çift kontak (S1, S2)
- Yazılım okuması: **`/dev/buttonstop`** — 8 bayt char dizisi
  - bit 7 (`buf[7]`) → S1
  - bit 6 (`buf[6]`) → S2
  - örnek: `"00000010"` → S2 basılı

### 4.4 Mode Switch (3-konum anahtar)

- Manual / Stop / Auto
- Yazılım okuması: **`/dev/buttons`** — 8 bayt char dizisi
  - bit 3 → Auto
  - bit 4 → Manual
  - bit 5 → Stop
  - örnek: `"00010000"` → Manual

### 4.5 Fiziksel Butonlar (14 adet, 2×7)

- Linux input event olarak `/dev/input/event0` (`matrix_keypad0`)
- Standart input_event yapısı; type=1 (EV_KEY), code=KEY_*
- Varsayılan haritalama HT0804 dökümanındaki tabloyu izler; değiştirmek için
  `KeyValueDefine` aracı `default.keySet` dosyasını `/etc/key_config/`
  altına yazıyor (HN00-09Q6'da bu klasör boş — özelleştirme yapılmamış).

### 4.6 Jog Wheel (handwheel)

- Rotary encoder, **`/dev/input/event1`** (`rotary`)
- Qt'de fare tekerleği olarak görünür: `QWheelEvent::angleDelta()`
- Basılı durum mouse press olarak gelir; çift tık desteklenir

### 4.7 Buzzer

- PWM tabanlı, `/dev/pwm`
- `ioctl(fd, cmd, val)` semantiği:
  - `(0, *)` → sustur
  - `(1, 1)` → sürekli aç, `(1, 0)` → sürekli kapat
  - `(3, ms)` → süreli bip (ms cinsinden, minimum 10 ms)

## 5. Konektör / Kablolama

### 5.1 16-Pin Aviation Connector

| Pin | Sinyal | Açıklama                          |
|-----|--------|-----------------------------------|
| 1   | TX+    | EtherNet TX+                      |
| 2   | TX–    | EtherNet TX–                      |
| 3   | RX+    | EtherNet RX+                      |
| 4   | RX–    | EtherNet RX–                      |
| 5   | COM    | Enable Switch ortak               |
| 6   | NO     | Enable Switch normal açık         |
| 7   | NC     | Enable Switch normal kapalı       |
| 8   | CN1    | Key Switch (Mode) konum 1         |
| 9   | CN2    | Key Switch (Mode) konum 2         |
| 10  | XN_NO  | Key Switch normal açık            |
| 11  | +24 V  | Besleme girişi                    |
| 12  | 0 V    | Besleme dönüş                     |
| 13  | XN_NC  | Key Switch normal kapalı          |
| 14  | FG     | Şasi / koruyucu toprak            |
| 15  | ST1    | E-Stop NC kontak 1                |
| 16  | ST2    | E-Stop NC kontak 2                |

### 5.2 Adapter Box (Bağlantı Kutusu) — 12-pin Terminal

| Pin | Etiket | Fonksiyon                                  |
|-----|--------|--------------------------------------------|
| 1   | FG     | Toprak                                     |
| 2   | 0 V    | Besleme 0V                                 |
| 3   | 0 V    | Besleme 0V                                 |
| 4   | ST1    | E-Stop NC 1                                |
| 5   | ST2    | E-Stop NC 2                                |
| 6   | 24 V   | Besleme 24V                                |
| 7   | 24 V   | Besleme 24V                                |
| 8   | A      | Rezerve (muhtemelen RS485 A)               |
| 9   | B      | Rezerve (muhtemelen RS485 B)               |
| 10  | NC     | Enable NC                                  |
| 11  | NO     | Enable NO                                  |
| 12  | COM    | Enable Common                              |

Adapter box üzerinde ayrıca **RJ45 Ethernet** yuvası bulunur — bizim
TP-Link switch'e bu portu bağladık (kullanıcı fotoğrafından görülüyor).

## 6. Elektriksel

| Parametre        | Değer                             |
|------------------|-----------------------------------|
| Giriş gerilimi   | 18–75 VDC                         |
| Anma güç          | < 5 W                              |
| Yıldırım korumas | ± 2 kV                            |
| ESD              | Doğrudan ±4 kV, dolaylı ±8 kV     |
| Burst            | ± 2 kV, 5 kHz / 100 kHz           |

## 7. Çevresel

| Parametre        | Değer                             |
|------------------|-----------------------------------|
| Çalışma sıcaklığı | -20 °C … +60 °C                   |
| Saklama          | -25 °C … +70 °C                   |
| Nem              | %10–90 RH yoğuşmasız               |
| IP koruma        | Ön panel IP65, genel IP54         |
| Düşme            | 1 m (koruyucu kılıfla)            |
| Titreşim         | GB2423.10 standardı                |
| Net ağırlık       | < 1 kg (kablolar hariç)            |
| Boyut            | 282 × 169 × 87 mm (E-Stop dahil)  |
| Kablo ömrü       | 20 000 / 100 000 büküm            |

## 8. Sertifika

- UL, CE
- (HN00-09Q6 datasheet'inde 'industrial isolated power supply' belirtilmiştir)
