# SMB-Q6R — Claude Code Project Instructions

> 6 eksen Fanuc 6R robot için CodeSys SMB-MAT-LC-C07 PLC tabanlı Teach Pendant arayüzü
> Donanım: Lavichip HN00-09Q6 (Rockchip RK3568 + Ubuntu 20.04 + Qt 5.15.10 + PREEMPT_RT)

## İlk Adımlar (Her Oturum)

1. `.ai/START_HERE.md` — proje yönelimi
2. `.ai/context.yaml` — yapılandırılmış proje durumu
3. `.ai/WORKPLAN.md` — mevcut faz, sıradaki eylemler
4. `.ai/ENGINEERING_LOG.md` — bilinen sorunlar, kararlar

## Proje Özeti

SMB-Q6R, Lavichip HN00-09Q6 endüstriyel Teach Pendant donanımı üzerinde çalışan,
**6 eksen Fanuc 6R uyumlu kollaboratif robot** için kontrol ve programlama arayüzüdür.
PLC arka ucu olarak **CodeSys SMB-MAT-LC-C07** kullanılır; PLC ile Ethernet üzerinden
endüstriyel bir protokolle haberleşir (Modbus TCP, OPC UA, EtherCAT-over-IP, ya da
özel TCP framing — Faz 2'de seçilecek).

## Mimari (3-Katman)

```
┌──────────────────────────────────────────────────────────┐
│  UI Layer (Qt 5.15.10 Widgets / QML — Faz 1'de karar)    │
│  Touch + 14 fiziksel buton + jog wheel + key switch      │
├──────────────────────────────────────────────────────────┤
│  Core Logic (C++17)                                      │
│  Kinematics, jog komutu, program yöneticisi, IO map      │
├──────────────────────────────────────────────────────────┤
│  Driver Layer                                            │
│  • HwIo (LED, buzzer, butonlar, encoder, backlight)      │
│  • PlcLink (PLC TCP/Modbus/OPC UA istemcisi)             │
│  • SafetyMonitor (E-Stop, Enable, mode switch)           │
└──────────────────────────────────────────────────────────┘
```

## Kritik Kurallar (İHLAL EDİLEMEZ)

1. **Güvenlik öncelikli:** E-Stop ve Enable Switch yolları UI thread'inden
   ayrı bir gerçek zamanlı thread'de izlenmeli. Hiçbir UI bloklaması güvenlik sinyalini
   geciktirmemeli.
2. **PLC ile asenkron iletişim:** Network I/O ana thread'i tutamaz; tüm PLC trafiği
   ayrı bir worker thread + Qt sinyal sistemi üzerinden.
3. **Hardware FD'leri tek-açık:** `/dev/leds`, `/dev/pwm`, `/dev/buttons`,
   `/dev/buttonstop` tekrar açılırsa **bozulur** (manualde uyarılmış). Singleton
   HwIo katmanı üzerinden açılır, uygulama yaşamı boyunca tutulur.
4. **Mode switch == işlem rejimi:** Auto / Manual / Stop konumu yazılım rejimi
   belirler. Jog sadece Manual'da, otomatik program sadece Auto'da çalışır.
5. **Real-time kernel'i bozma:** `PREEMPT_RT` kernel üzerindeyiz — uzun süreli
   spin lock veya disk I/O ana thread'de yapılmamalı.

## Donanım Hızlı Referans

| Fonksiyon                | Cihaz / Yol                                       |
|--------------------------|---------------------------------------------------|
| 14 fiziksel buton         | `/dev/input/event0` (matrix_keypad0)              |
| Jog wheel (handwheel)     | `/dev/input/event1` (rotary)                      |
| Touchscreen (1280×800)    | `/dev/input/event3` (ilitek_ts)                   |
| Mode switch (3-poz)       | `/dev/buttons` — bit 3=Auto, 4=Manual, 5=Stop     |
| Enable switch (S1/S2)     | `/dev/buttonstop` — bit 7=S1, 6=S2                |
| LED'ler (Stop/Servo/Enable)| `/dev/leds` — `ioctl(fd, state, port)` port 0-4  |
| Buzzer                    | `/dev/pwm` — `ioctl(fd, cmd, val)`                |
| Backlight (0-100)         | `/sys/class/backlight/backlight/brightness`       |
| Seri port                 | `/dev/ttyS2`                                      |
| Ethernet (PLC bağlantısı) | `eth1` — 192.168.1.245/24                         |

Detay: [`.ai/HARDWARE_API.md`](.ai/HARDWARE_API.md)

## Hızlı Komutlar

```bash
# Cihaza SSH (parola boş)
sshpass -p '' ssh -o PreferredAuthentications=password -o PubkeyAuthentication=no \
  Tronlong@192.168.1.245

# Uygulama dağıtımı
scp build/smb_q6r Tronlong@192.168.1.245:/userfs/app/

# Cihazda Qt ortamı
source /etc/profile.d/qt5.15.10.sh
source /etc/profile.d/qt_env.sh
```

## Kodlama Standartları

- **Dil:** C++17 (Qt 5.15.10 desteklediği üst sınır)
- **Kod ve yorumlar:** İngilizce
- **Kullanıcı iletişimi:** Türkçe
- **Namespace:** `smbq6r`
- **Sınıf:** `PascalCase`, üye değişken: `member_` (trailing underscore)
- **Dizin:** `inc/` başlıklar, `src/` kaynak; `ui/` Qt formları
- **Build:** CMake 3.16+ (Ubuntu 20.04 ile uyumlu)
- **Format:** `.clang-format` (Faz 0'da eklenecek)

## Referans Projeler

- **CADNC:** [`/home/embed/Dev/CADNC/`](file:///home/embed/Dev/CADNC/) — modern Qt6 CAD, `.ai/` deseni
- **SODOO:** [`/home/embed/Dev/SODOO/`](file:///home/embed/Dev/SODOO/) — kurumsal Odoo, detaylı doküman seti
- **Lavichip SDK & demolar:** [`/home/embed/Dev/QT6/TeachPendant/`](file:///home/embed/Dev/QT6/TeachPendant/)

Tam liste: [`.ai/REFERENCES.md`](.ai/REFERENCES.md)
