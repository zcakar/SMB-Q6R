# Donanım API Referansı

> Cihaz dosyalarının kullanımı. Demo kaynak kodlarından
> (`/home/embed/Dev/QT6/TeachPendant/2. Interface development demo/HWInterfaceDemo/`)
> ve manualden çıkarıldı; HN00-09Q6 üzerinde canlı doğrulandı (kısmen).

## Özet Tablo

| Aygıt                        | Mod   | Veri Modeli                      | Notlar                    |
|------------------------------|-------|----------------------------------|----------------------------|
| `/dev/input/event0`          | RO    | Linux `input_event`              | 14 matrix tuş              |
| `/dev/input/event1`          | RO    | Linux `input_event` (REL_WHEEL)  | Jog wheel                  |
| `/dev/input/event3`          | RO    | Linux `input_event` (touch)      | Ilitek dokunmatik           |
| `/dev/buttons`               | RO    | 8 char bytes                     | Mode switch (Auto/Man/Stop)|
| `/dev/buttonstop`            | RO    | 8 char bytes                     | Enable switch (S1/S2)      |
| `/dev/leds`                  | WO    | `ioctl(fd, state, port)`         | 3-5 LED                    |
| `/dev/pwm`                   | WO    | `ioctl(fd, cmd, val)`            | Buzzer                     |
| `/sys/class/backlight/.../brightness` | RW | ASCII 0..100              | Ekran aydınlatma           |
| `/dev/ttyS2`                 | RW    | termios                          | Seri port                  |

> **Kritik:** `/dev/leds`, `/dev/pwm`, `/dev/buttons`, `/dev/buttonstop`
> dosyaları **bir kere açılmalı** ve uygulama süresince açık kalmalıdır.
> Tekrar `open()` çağrısı sürücü tarafında hatalı duruma yol açabilir
> (Manual 3.4'te uyarılmış). Buna yönelik `HwIo` singleton tasarımı uygulanır.

---

## 1. LED Kontrolü — `/dev/leds`

```cpp
#include <fcntl.h>
#include <sys/ioctl.h>

int fd = ::open("/dev/leds", O_WRONLY);   // bir kere
::ioctl(fd, /*state*/ 1, /*port*/ 3);     // port 3'ü AÇ
::ioctl(fd, /*state*/ 0, /*port*/ 3);     // port 3'ü KAPAT
```

- `state`: 1 = aç, 0 = kapat
- `port`: 0..4 (cihaz modeline göre)
- HT0803 LED port haritası: Led1=3, Led2=4, Led3=2, Led4=1, Led5=0
- HT0804 (HN00-09Q6'ya yakın) LED port haritası: Led1=3, Led2=2, Led3=1, Led4=0
- **HN00-09Q6'da gerçek port haritası Faz 1'de fiziksel testle doğrulanacak.**

## 2. Buzzer — `/dev/pwm`

```cpp
int fd = ::open("/dev/pwm", O_RDWR);

::ioctl(fd, 0, 0);            // (cmd=0, *) → sustur
::ioctl(fd, 1, 1);            // (cmd=1, val=1) → sürekli aç
::ioctl(fd, 1, 0);            // (cmd=1, val=0) → sürekli kapat
::ioctl(fd, 3, 200);          // (cmd=3, ms=200) → 200ms bip
```

- Minimum süre: 10 ms
- **Tekrar açma yasak** (driver state'i bozulur)

## 3. Enable Switch — `/dev/buttonstop`

```cpp
int fd = ::open("/dev/buttonstop", O_RDONLY | O_NONBLOCK);
// QSocketNotifier ile asenkron oku
QSocketNotifier* sn = new QSocketNotifier(fd, QSocketNotifier::Read);
connect(sn, &QSocketNotifier::activated, this, [this, fd]() {
    char buf[8] = {0};
    if (::read(fd, buf, 8) == 8) {
        bool s1 = buf[7] == '1';   // bit 7
        bool s2 = buf[6] == '1';   // bit 6
        emit enableSwitchChanged(s1, s2);
    }
});
```

- 8 char dönüş, sadece S1 (bit 7) ve S2 (bit 6) anlamlı; geri kalan `'0'`
- Mantık:
  - S1=0, S2=0 → Dezaktif (üst veya tam basılı panic)
  - S1=1, S2=1 → Aktif (orta konum)
  - Üreticinin tasarımına göre S1/S2 ayrı yorumlanabilir; **fiziksel test
    gerekli** (Faz 1).

## 4. Mode Switch — `/dev/buttons`

```cpp
int fd = ::open("/dev/buttons", O_RDONLY | O_NONBLOCK);
char buf[8];
::read(fd, buf, 8);
bool auto_mode = buf[3] == '1';
bool manual    = buf[4] == '1';
bool stop      = buf[5] == '1';
```

- Bit 3 = Auto, Bit 4 = Manual, Bit 5 = Stop
- Aynı anda yalnızca bir tanesi '1' olur (3-pozisyonlu anahtar)

## 5. Matrix Keypad — `/dev/input/event0`

Standart Linux `input_event` yapısı (`<linux/input.h>`):

```cpp
#include <linux/input.h>

int fd = ::open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
input_event ev;
while (::read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
    if (ev.type == EV_KEY) {
        if (ev.value == 1) qDebug() << "Pressed key code:" << ev.code;
        if (ev.value == 0) qDebug() << "Released key code:" << ev.code;
    }
}
```

- Qt zaten X11 üzerinden bu olayları `keyPressEvent` olarak gönderiyor
  (qt_env.sh içinde `QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/event0`
  ayarlı).
- Doğrudan açmak istersen kendi dinleyicini yapmalısın **ve** Qt ile
  çakışma olasılığına dikkat etmelisin.
- Anahtar kod ↔ buton konumu eşlemesi Faz 1'de fiziksel testle çıkarılacak.

## 6. Jog Wheel — `/dev/input/event1`

Qt tarafında en kolay yol: **`QWheelEvent`** dinlemek. qt_env.sh içinde
event1 mouse parametresi olarak ayarlı, dolayısıyla wheel olayları
otomatik gelir.

```cpp
void MainWindow::wheelEvent(QWheelEvent* e) {
    int delta = e->angleDelta().y();   // > 0 yukarı, < 0 aşağı
    // delta normalde 120'nin katı; encoder her tıkta 120 birim üretir
}
```

Basılı durum **mouse press** olarak gelir; çift tık `mouseDoubleClickEvent`
ile yakalanır.

## 7. Backlight — `/sys/class/backlight/backlight/brightness`

```cpp
QFile f("/sys/class/backlight/backlight/brightness");
f.open(QIODevice::WriteOnly);
f.write("75");   // 0..100
```

- Mevcut değeri okumak için aynı dosyayı `ReadOnly` aç
- 0 yazmak ekran arkaplan ışığını söndürür

## 8. Network Ayarı

İki dosya etkileşir; **netplan** öncelikli:

`/etc/netplan/01-netcfg.yaml`:
```yaml
network:
  version: 2
  renderer: networkd
  ethernets:
    eth1:
      dhcp4: no
      addresses: [192.168.1.245/24]
      gateway4: 192.168.1.1
      nameservers:
        addresses: [8.8.8.8, 8.8.4.4]
```

IP değiştirmek:
```bash
sudo sed -i 's|192.168.1.245|192.168.1.X|' /etc/netplan/01-netcfg.yaml
sudo netplan apply
```

## 9. Seri Port — `/dev/ttyS2`

Eski manuelde `/dev/ttyS1` denmişti; **HN00-09Q6'da `/dev/ttyS2`** olarak
listelendi. Standart termios kullanımı:

```cpp
QSerialPort port;
port.setPortName("/dev/ttyS2");
port.setBaudRate(QSerialPort::Baud115200);
port.open(QIODevice::ReadWrite);
```

## 10. Önyükleme Logo'su

24-bit BMP, **1280×800** boyutunda (eski 800×600 değil), adı `logo.bmp`,
yolu `/run/media/mmcblk1p1/logo.bmp` (USB / SD ile aktar).

## 11. Dokunmatik Kalibrasyon

```bash
sudo rm /etc/pointercal
sudo reboot
# Yeniden başlayınca 5-nokta kalibrasyon ekranı gelir
```

## 12. Qt Çalışma Zamanı Ortamı

Cihazda Qt 5.15.10 hazır. Uygulamayı çalıştırmadan önce:
```bash
source /etc/profile.d/qt5.15.10.sh
source /etc/profile.d/qt_env.sh
```

Önemli env'ler:
- `QT_QPA_PLATFORM=xcb:size=1280x800`
- `LD_LIBRARY_PATH=/usr/lib/qt-5.15.10/lib:...`
- `QT_PLUGIN_PATH=/usr/lib/qt-5.15.10/plugins`

Mouse imlecini gizlemek için (Teach Pendant'ta istenmez):
```bash
export QT_QPA_FB_HIDECURSOR=1
```
