# Geliştirme Ortamı Kurulumu

## Host Sistemi

Mevcut geliştirme makinesi: Ubuntu Linux (kullanıcı `embed`)
- `apt` paket yöneticisi
- `sshpass 1.09` mevcut
- `nmap`, `curl` mevcut
- Cihaz `192.168.1.245`, host'un `enx4ce1734a002a` interface'i (USB-to-Ethernet)
  üzerinden `192.168.1.1/24` ile birlikte aynı L2 ağında

## Hedef Cihaz

| Anahtar          | Değer                                  |
|------------------|----------------------------------------|
| IP               | 192.168.1.245                          |
| Kullanıcı         | Tronlong                               |
| Parola           | (boş)                                  |
| Mimari            | aarch64                                |
| Toolchain         | Cihazda gcc 9.4 (Ubuntu Focal)         |
| Qt               | 5.15.10 @ `/usr/lib/qt-5.15.10`        |

## Build Stratejisi

İki yol var; hangisini seçeceğimizi **Faz 1**'de doğrularız:

### Yol A — Cihazda native build (en hızlı başlangıç)
```bash
# Cihazda
sudo apt update
sudo apt install -y build-essential cmake git qtbase5-dev qttools5-dev
cd /userfs/dev/smb-q6r && cmake -B build && cmake --build build -j4
```

**Avantaj:** Toolchain ile uğraşma yok, Qt zaten kurulu.
**Dezavantaj:** Cihaz disk dolarsa sorun olur (1.4 GB boş); compile süresi.

### Yol B — Host'ta cross-compile (Faz 2+ için daha iyi)
```bash
# Host'ta gerekli paketler
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qtbase5-dev-tools

# Sysroot cihazdan rsync edilir
mkdir -p ~/sysroot-rk3568/usr
rsync -avz Tronlong@192.168.1.245:/usr/lib ~/sysroot-rk3568/usr/
rsync -avz Tronlong@192.168.1.245:/usr/include ~/sysroot-rk3568/usr/

# CMake toolchain file ile build
cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-toolchain.cmake -S .
```

**Avantaj:** Host'ta hızlı, CI uyumlu, IDE entegrasyonu kolay.
**Dezavantaj:** Sysroot hazırlama, Qt MOC için ayrı toolchain.

### Önerilen Karar

**Faz 1 → Yol A (native).**
**Faz 2 sonrası → Yol B (cross).** Birden çok geliştirme makinesi olursa
cross gerekecek.

## Deploy İş Akışı

```bash
# 1) Build (örnek: cihazda native)
ssh Tronlong@192.168.1.245 "cd /userfs/dev/smb-q6r && cmake --build build"

# 2) Çalıştırma (X üzerinde)
ssh -X Tronlong@192.168.1.245 \
  "source /etc/profile.d/qt_env.sh && /userfs/dev/smb-q6r/build/smb_q6r"

# 3) Yayın için /userfs/app/'a kopyala
scp build/smb_q6r Tronlong@192.168.1.245:/userfs/app/smb_q6r
```

Yaşam döngüsü `Makefile` veya `scripts/deploy.sh` ile sarmalanacak
(Faz 1 sonu).

## SSH Konfigürasyon Önerisi

Host'ta `~/.ssh/config`:
```
Host pendant
    HostName 192.168.1.245
    User Tronlong
    PreferredAuthentications password
    PubkeyAuthentication no
    StrictHostKeyChecking accept-new
```

Sonra: `ssh pendant`, `scp file pendant:/userfs/app/`

İlerleyen aşamada **SSH key** kurmak çok daha iyi:
```bash
ssh-keygen -t ed25519 -f ~/.ssh/pendant_key
ssh-copy-id -i ~/.ssh/pendant_key.pub pendant
```

## Otomatik Başlatma (Faz 8)

systemd servisi:

```ini
# /etc/systemd/system/smb-q6r.service
[Unit]
Description=SMB-Q6R Teach Pendant
After=graphical.target

[Service]
Type=simple
User=Tronlong
EnvironmentFile=/etc/profile.d/qt_env.sh
ExecStart=/userfs/app/smb_q6r
Restart=always
RestartSec=2

[Install]
WantedBy=graphical.target
```

`sudo systemctl enable smb-q6r.service`

## Logging

Standart yaklaşım:
- `qInstallMessageHandler` ile `qDebug/qWarning` çıktıları dosyaya
- `~/smb-q6r-logs/smb-q6r-YYYY-MM-DD.log` (her gün rotation)
- 14 gün sonrası otomatik silinir (Faz 5'te `LogRotator` sınıfı)

Cihazın `journalctl`'i de geliştirme zamanı çok yardımcıdır:
```bash
ssh pendant "journalctl -u smb-q6r.service -f"
```

## Mevcut Demo'yu Çalıştırma (Test)

Cihazdaki fabrika demo'su:
```bash
ssh pendant
source /etc/profile.d/qt_env.sh
DISPLAY=:0 /userfs/app/lyx_appDemo &
```

Ekranda görünmesi gerek (Qt Widgets uygulamasıdır, XCB üzerinden çalışır).
