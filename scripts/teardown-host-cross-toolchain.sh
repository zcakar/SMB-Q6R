#!/usr/bin/env bash
# Reverse `setup-host-cross-toolchain.sh`:
#   * remove installed cross packages
#   * remove arm64 architecture
#   * restore ubuntu.sources from backup
#
# Usage:  sudo scripts/teardown-host-cross-toolchain.sh

set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Bu script root yetkisi gerekir. 'sudo' ile çalıştırın."
    exit 1
fi

UBUNTU_SOURCES="/etc/apt/sources.list.d/ubuntu.sources"
ARM64_SOURCES="/etc/apt/sources.list.d/ubuntu-arm64-ports.sources"
BACKUP="/etc/apt/sources.list.d/.smb-q6r-backup/ubuntu.sources.bak"

echo "==> 1/4: arm64 paketlerini kaldır"
apt-get remove --purge -y \
    'crossbuild-essential-arm64' \
    'qtbase5-dev:arm64' \
    'qtdeclarative5-dev:arm64' \
    'qml-module-qtquick-controls2:arm64' \
    'qml-module-qtquick-window2:arm64' \
    'qml-module-qtquick-layouts:arm64' \
    'qml-module-qtquick2:arm64' \
    'libqt5quickcontrols2-5:arm64' \
    'pkg-config-aarch64-linux-gnu' \
    || true

apt-get autoremove -y

echo "==> 2/4: arm64 ports sources kaldır"
rm -f "$ARM64_SOURCES"

echo "==> 3/4: ubuntu.sources yedekten geri yükle"
if [[ -f "$BACKUP" ]]; then
    cp "$BACKUP" "$UBUNTU_SOURCES"
    echo "    Geri yüklendi."
else
    echo "    Yedek bulunamadi; ubuntu.sources elle düzenlenmeli."
fi

echo "==> 4/4: dpkg arm64 mimarisini kaldır"
dpkg --remove-architecture arm64 || true
apt-get update

echo
echo "Teardown tamam. Host cross-compile öncesi durumuna döndü."
