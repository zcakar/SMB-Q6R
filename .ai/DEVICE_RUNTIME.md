# Cihaz Canlı Durum Notları

> Bu dosya 2026-05-11 tarihinde SSH ile alınan **gerçek** cihaz durumudur.
> PDF datasheet ve eski SDK manualinde olmayan/farklı olan değerler burada
> kayıt altındadır. Donanım/imaj güncellenirse bu dosya yenilenmelidir.

## SSH Erişimi

```bash
# Parola boş (Enter)
sshpass -p '' ssh \
  -o StrictHostKeyChecking=no \
  -o PreferredAuthentications=password \
  -o PubkeyAuthentication=no \
  Tronlong@192.168.1.245
```

> **Not:** Manualde `root` / `1234` yazıyor; bu yalnızca eski HT0803/HT0804
> imajları için. HN00-09Q6'nın 2025-07-11 imajı `Tronlong` kullanıcısı +
> boş parola ile çalışıyor.

## OS

| Anahtar         | Değer                                                  |
|------------------|--------------------------------------------------------|
| Distro           | Ubuntu 20.04.6 LTS Focal Fossa                         |
| Kernel           | `Linux 5.10.209-rt89 SMP PREEMPT_RT aarch64`           |
| Build tarihi      | 2025-07-11 (CST)                                       |
| Hostname         | `langyuxin`                                            |
| Default user     | `Tronlong` (uid 1000), parola boş                       |
| Root parola      | Bilinmiyor — `sudo` kullanıcı yetkisi muhtemel          |
| Apache2          | Çalışıyor — port 80 default sayfa                       |
| Telnet           | Açık (port 23)                                          |
| SSH              | Açık (port 22) — OpenSSH ED25519                        |

## Donanım

CPU:
- 4 × ARM Cortex-A53 (CPU part 0xd03, ARMv8-A 64-bit)
- Implementer ARM (0x41)
- Seri No: `74708edcccad9d45`

Bellek:
- 1.9 GiB RAM (2 GB DDR4)
- 5.3 GB rootfs, 3.7 GB kullanılmış, 1.4 GB boş (Faz 6+ için disk yönetimi)
- Swap yok

## Network

```
2: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    link/ether 8e:df:ee:98:fe:50
    inet 192.168.1.245/24 brd 192.168.1.255 scope global eth1
```

Yapılandırma kaynağı: **netplan** (önceliklendirilmiş):
```
/etc/netplan/01-netcfg.yaml:
  eth1: static 192.168.1.245/24, gw 192.168.1.1, dns 8.8.8.8 / 8.8.4.4
```

> **Çelişki uyarısı:** `/etc/systemd/network/eth1.network` içinde `DHCP=yes`
> yazıyor ama netplan static'i renderer olarak networkd'yi yapılandırdığı
> için netplan kazanıyor. Faz 1'de `eth1.network`'i temizlemek karışıklığı
> önler.

Diğer interface'ler tanımlı (`eth0/2/3.network`) fakat link aşağı.
**eth1**, aviation konektör → adapter box → RJ45 yolu üzerinden bizim
TP-Link switch'e bağlı.

## Qt Çalışma Zamanı

- Kurulum kökü: `/usr/lib/qt-5.15.10`
- Profil scripti: `/etc/profile.d/qt5.15.10.sh` (LD_LIBRARY_PATH, QML2_IMPORT_PATH)
- Cihaz scripti: `/etc/profile.d/qt_env.sh` (XCB, DISPLAY=:0, input device map)

`qt_env.sh` içeriği (alıntı):
```bash
export QT_QPA_PLATFORM=xcb:size=1280x800
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/event0
export QT_QPA_EVDEV_MOUSE_PARAMETERS=/dev/input/event1:/dev/input/event3
export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/event2  # ← yanlış olabilir
```

> **Sorun:** `event2` aslında `rk805 pwrkey` (güç tuşu). Gerçek dokunmatik
> `event3` (`ilitek_ts`). Bu satırın `event3` olması gerekiyor; Faz 1'de
> düzeltilecek. **Yine de XCB kendi bilgi keşfini yaptığı için dokunmatik
> şimdilik çalışıyor.**

## Mevcut Uygulama

```
/userfs/app/
└── lyx_appDemo            (ELF aarch64, Qt5 Widgets, 41 KB, fabrika demosu)
```

`/userfs/app/` bizim uygulamanın da gideceği yer. Boot anında otomatik
başlatma için Faz 8'de `/etc/systemd/system/smb-q6r.service` eklenecek
(şu an `/etc/startup.sh` yok).

## Input Aygıtları

| Aygıt              | Driver         | Kullanım                                  |
|---------------------|----------------|--------------------------------------------|
| `/dev/input/event0` | matrix_keypad0 | 14 fiziksel buton                          |
| `/dev/input/event1` | rotary         | Jog wheel (handwheel)                      |
| `/dev/input/event2` | rk805 pwrkey   | Güç tuşu                                   |
| `/dev/input/event3` | ilitek_ts      | Kapasitif dokunmatik                       |
| `/dev/input/event4` | adc-keys       | ADC-tabanlı tuşlar (kullanım belirsiz)     |
| `/dev/input/event5` | gpio_keys      | GPIO-tabanlı tuşlar (kullanım belirsiz)    |

## Özel Cihaz Dosyaları (Lavichip)

| Aygıt              | Mod | Mevcut |
|---------------------|-----|--------|
| `/dev/buttons`      | RO  | ✓      |
| `/dev/buttonstop`   | RO  | ✓      |
| `/dev/leds`         | WO  | ✓      |
| `/dev/pwm`          | WO  | ✓      |

## Backlight

`/sys/class/backlight/backlight/brightness` → **100** (ilk okumada)
Aralık 0..100.

## Serial

`/dev/ttyS2` mevcut (`dialout` grubu).
`/dev/ttyS0` ve `/dev/ttyS1` yok — eski manuel HT0803'te `/dev/ttyS1`
belirtmişti, **HN00-09Q6'da `/dev/ttyS2`** kullanılır.

## Pratik İpuçları

- `sudo` çağırınca parola boş olabilir (kullanıcı sudoers'da NOPASSWD ile
  ayarlı görünüyor — `sudo whoami` test edilmemiş).
- Cihazda `git`, `vim`, `htop`, `lsblk` gibi standart Ubuntu paketleri
  muhtemelen kurulu; ihtiyaç olduğunda `dpkg -l | grep ...` ile kontrol.
- `apt update / install` cihaza paket eklemek için kullanılabilir
  (internet erişimi ayrı bir konu — şu an switch'ten host'a bağlı, host
  internet'i NAT etmiyor).
