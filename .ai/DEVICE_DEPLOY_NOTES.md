# Cihaz Dağıtım Notları (Deploy Fixes)

> Yeni bir HN00-09Q6 cihaza (veya fabrika imajı sıfırlanmış birine) deploy
> yaparken karşılaşılabilecek paket eksikliklerinin envanteri ve çözümleri.
>
> **Mevcut yaklaşım (2026-05-11):** İlk cihazımızda Qt Quick Controls 2 paketi
> kurulu **değildi**. Cihaza ek paket kurmak yerine, **build tarafından** sadece
> cihazda mevcut olan modülleri kullanarak (`qml-module-qtquick2` 5.12.8) uygulamayı
> hazırladık. Bu sayede yeni cihaza deploy = tek satırlık `scp`. Ancak bazı
> cihazlar veya gelecekteki UI ihtiyaçları paket yüklemesini gerektirebilir.

## Hızlı Cihaz Denetimi (yeni cihaz takıldığında)

```bash
ssh Tronlong@<ip>
echo "--- Qt 5 system runtime ---"
dpkg -l | grep -E "^ii\s+libqt5(core|gui|qml|quick|widgets)5a?:arm64" \
  | awk '{print $2,$3}'
echo "--- QML modules ---"
dpkg -l | grep -E "^ii\s+qml-module" | awk '{print $2}'
echo "--- Vendor Qt (5.15.10) ---"
ls /usr/lib/qt-5.15.10/lib/libQt5*.so.5 2>/dev/null | head
echo "--- Backlight, devs ---"
ls /dev/leds /dev/buttons /dev/buttonstop /dev/pwm 2>&1
```

Beklenen baseline (mevcut imaj, 2025-07-11 yapımı):

| Paket                                     | Mevcut? |
|-------------------------------------------|---------|
| libqt5core5a:arm64 5.12.8                 | ✓       |
| libqt5gui5:arm64 5.12.8                   | ✓       |
| libqt5qml5:arm64 5.12.8                   | ✓       |
| libqt5quick5:arm64 5.12.8                 | ✓       |
| libqt5widgets5:arm64 5.12.8               | ✓       |
| libqt5network5:arm64 5.12.8               | ✓       |
| libqt5opengl5:arm64 5.12.8                | ✓       |
| libqt5svg5:arm64 5.12.8                   | ✓       |
| libqt5multimedia5:arm64 5.12.8            | ✓       |
| qml-module-qtquick2:arm64 5.12.8          | ✓       |
| qml-module-qtmultimedia:arm64 5.12.8      | ✓       |
| qml-module-qt-labs-folderlistmodel:arm64  | ✓       |
| **libqt5quickcontrols2-5:arm64**          | **✗**   |
| **libqt5quicktemplates2-5:arm64**         | **✗**   |
| **qml-module-qtquick-controls2:arm64**    | **✗**   |
| **qml-module-qtquick-layouts:arm64**      | **✗**   |
| **qml-module-qtquick-window2:arm64**      | **✗**   |
| **qml-module-qtquick-templates2:arm64**   | **✗**   |
| Vendor Qt 5.15.10 (custom path)           | ✓       |

## Eksik Paket Senaryosu — Adım Adım Kurulum

Eğer ileride Qt Quick Controls 2 veya Layouts'a ihtiyaç olursa (mesela jog
ekranında modern slider/dial gerekirse), aşağıdaki yöntem cihaz internete
ihtiyaç **duymadan** çalışır:

### 1. Host'ta .deb'leri Docker build image'ı üzerinden çek

Host'ta Docker daemon çalışıyor ve `smb-q6r-builder:focal-arm64` image'ı kurulu
olmalı (`scripts/docker-build.sh` ilk çalıştırmada yapar).

```bash
mkdir -p /tmp/q6r-debs
docker run --rm -v /tmp/q6r-debs:/out -w /out smb-q6r-builder:focal-arm64 \
  'apt-get update >/dev/null && apt-get download \
      libqt5quickcontrols2-5:arm64 \
      libqt5quicktemplates2-5:arm64 \
      qml-module-qtquick-controls2:arm64 \
      qml-module-qtquick-templates2:arm64 \
      qml-module-qtquick-layouts:arm64 \
      qml-module-qtquick-window2:arm64'
ls -lh /tmp/q6r-debs/
```

Toplam ~1.8 MB. Tüm dosyalar Ubuntu Focal arm64 paketleri (cihaz imajıyla
birebir uyumlu).

### 2. Cihaza scp + dpkg

```bash
sshpass -p '' scp /tmp/q6r-debs/*.deb Tronlong@192.168.1.245:/tmp/q6r-debs/
sshpass -p '' ssh Tronlong@192.168.1.245 \
  'sudo dpkg -i /tmp/q6r-debs/*.deb'
```

> Cihazda `sudo` parola istiyorsa kullanıcıdan istenmeli. `Tronlong` NOPASSWD
> sudoers'da mı bilmiyoruz — Phase 0'da denenmedi.

### 3. Temizlik

```bash
sshpass -p '' ssh Tronlong@192.168.1.245 'rm -rf /tmp/q6r-debs'
rm -rf /tmp/q6r-debs
```

## Alternatif Yollar

### A. Vendor Qt 5.15.10 ile çalıştırma (ABI riski)

Cihazda `/usr/lib/qt-5.15.10/` altında **tam** Qt 5.15.10 kuruludur — Quick
Controls 2 dahil. Uygulamayı çalıştırırken `source /etc/profile.d/qt_env.sh`
çalıştırılırsa LD_LIBRARY_PATH bu yolu öne alır.

```bash
source /etc/profile.d/qt5.15.10.sh
source /etc/profile.d/qt_env.sh
DISPLAY=:0 /home/Tronlong/smb-q6r/smb_q6r
```

⚠️ **Risk:** Bizim binary Qt 5.12.8 başlıklarına karşı derlendi. 5.15 runtime
ile çalıştırmak Qt'nin resmi ABI politikasının dışında — çoğunlukla çalışır ama
crash veya yanlış davranış olasılığı var. Production'da önerilmez.

### B. Build'i Qt 5.15.10 hedefine geçirmek

`docker/Dockerfile` Focal yerine Jammy + Qt 5.15.3 kullanılarak yeniden
yazılabilir (glibc 2.35 → cihaz 2.31 yeniden uyumsuz olur; çözülmesi gerek).
Veya vendor SDK ile build edilirse 1:1 uyum sağlanır — ama vendor SDK
mevcut değil (Phase 0'da arandı, sadece eski TI Sitara için var).

### C. UI'yi Qt Widgets'a indirgemek

Cihazda `libqt5widgets5` zaten kurulu. Eğer Phase 4+'da QML'in karmaşıklığı
ağır gelirse Widgets'a dönülebilir — vendor `lyx_appDemo`'nun yolu.

## Mevcut Karar (Phase 1)

**QtQuick 2 + elle yapılmış primitive'ler.** Tek paket yüklemesi yok.
`qml-module-qtquick2` ile gelen `Item`, `Rectangle`, `Text`, `MouseArea`,
`Image`, `Timer`, `Animation` yeter — `TabBar`, `Button`, `Slider`, vb.
kendi `qml/components/` dizinimizde yapılır.

Bu yaklaşımın faydası:
- Yeni cihaza deploy: sadece `scp build-arm64/smb_q6r.stripped`
- Sıfır dependency drift
- Tüm UI stili tek elden — vendor'a bağlı değil

Maliyeti:
- Phase 1 daha çok QML yazıldı (~%50 daha)
- Future devs bu primitive'leri öğrenmek zorunda

## Engineering Log Bağlantısı

Her cihaz için kurulum sonrası `ENGINEERING_LOG.md`'de bir entry:
- Cihaz seri no
- IP
- Hangi paketlerin mevcut olduğu (yukarıdaki tablo ile karşılaştırılarak)
- Eksik varsa kurulum tarihi ve yöntemi
- LED port haritası (fiziksel test sonucu)
