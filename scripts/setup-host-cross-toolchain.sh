#!/usr/bin/env bash
# Set up arm64 multiarch + Qt 5.15 cross-compile toolchain on Ubuntu Noble host.
#
# This script is IDEMPOTENT — safe to re-run.
# Reverse with:  scripts/teardown-host-cross-toolchain.sh
#
# Usage:  sudo scripts/setup-host-cross-toolchain.sh

set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Bu script root yetkisi gerekir. 'sudo scripts/setup-host-cross-toolchain.sh' ile çalıştırın."
    exit 1
fi

CODENAME="$(lsb_release -cs)"
if [[ "$CODENAME" != "noble" ]] && [[ "$CODENAME" != "jammy" ]]; then
    echo "Uyari: bu script Ubuntu Noble (24.04) veya Jammy (22.04) için test edildi. Mevcut: $CODENAME"
    read -r -p "Yine de devam edilsin mi? [y/N] " ans
    [[ "$ans" =~ ^[Yy]$ ]] || exit 1
fi

UBUNTU_SOURCES="/etc/apt/sources.list.d/ubuntu.sources"
ARM64_SOURCES="/etc/apt/sources.list.d/ubuntu-arm64-ports.sources"
BACKUP_DIR="/etc/apt/sources.list.d/.smb-q6r-backup"

echo "==> 1/6: Mevcut amd64 sources'u amd64'e kısıtla"
if ! grep -q "^Architectures:" "$UBUNTU_SOURCES"; then
    mkdir -p "$BACKUP_DIR"
    cp -n "$UBUNTU_SOURCES" "$BACKUP_DIR/ubuntu.sources.bak" || true
    # deb822 formatında her boş satır yeni bir paragrafı ayırır.
    # Her paragrafa Architectures: amd64 ekle (eğer yoksa).
    awk '
        BEGIN { added = 0 }
        /^Types:/ { added = 0 }
        /^Architectures:/ { added = 1 }
        /^Signed-By:/ {
            if (!added) {
                print "Architectures: amd64"
                added = 1
            }
        }
        { print }
    ' "$UBUNTU_SOURCES" > "${UBUNTU_SOURCES}.tmp"
    mv "${UBUNTU_SOURCES}.tmp" "$UBUNTU_SOURCES"
    echo "    Tamam. Yedek: $BACKUP_DIR/ubuntu.sources.bak"
else
    echo "    Atlandi (zaten Architectures alani var)."
fi

echo "==> 2/6: arm64 ports sources ekle"
cat > "$ARM64_SOURCES" <<EOF
Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports/
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-backports
Components: main restricted universe multiverse
Architectures: arm64
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports/
Suites: ${CODENAME}-security
Components: main restricted universe multiverse
Architectures: arm64
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
echo "    Tamam: $ARM64_SOURCES"

echo "==> 3/6: dpkg architecture arm64 etkinleştir"
dpkg --add-architecture arm64
echo "    Etkinleştirilmiş arch'ler:"
dpkg --print-foreign-architectures | sed 's/^/    /'

echo "==> 4/6: apt update"
apt-get update

echo "==> 5/6: cross-compile toolchain + Qt 5 arm64 dev paketleri kur"
# Not: pkg-config Ubuntu Noble'de multiarch-aware; ayri 'pkg-config-aarch64-linux-gnu'
# paketi yok. Toolchain file PKG_CONFIG_LIBDIR ile arm64 .pc yollarini hedefler.
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    crossbuild-essential-arm64 \
    pkg-config \
    qtbase5-dev:arm64 \
    qtbase5-dev-tools \
    qtdeclarative5-dev:arm64 \
    qttools5-dev-tools \
    qml-module-qtquick-controls2:arm64 \
    qml-module-qtquick-window2:arm64 \
    qml-module-qtquick-layouts:arm64 \
    qml-module-qtquick2:arm64 \
    libqt5quickcontrols2-5:arm64 \
    cmake \
    ninja-build

echo "==> 6/6: Dogrulama"
echo -n "    aarch64-linux-gnu-g++ : "; aarch64-linux-gnu-g++ --version | head -1
echo -n "    cmake                 : "; cmake --version | head -1
echo -n "    Qt5Core .pc (arm64)   : "
pkg-config --variable=prefix --define-variable=ARCH=arm64 Qt5Core 2>/dev/null \
    || ls /usr/lib/aarch64-linux-gnu/qt5 2>/dev/null \
    || echo "(pkg-config bulamadi - manual yola bakin)"

cat <<'EOF'

==================================================================
 Host arm64 cross-compile ortamı hazır.
 Sonraki adim:
   cd /home/embed/Dev/QT6/TeachPendant/SMB-Q6R
   cmake -B build-arm64 \
     -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake -S .
   cmake --build build-arm64 -j$(nproc)
==================================================================
EOF
