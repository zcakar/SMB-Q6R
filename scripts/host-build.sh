#!/usr/bin/env bash
# Native host build — for offline UI development on the dev workstation.
#
# Builds smb_q6r against the host's amd64 Qt5 (5.15.13 on Noble), drops the
# Docker arm64 toolchain entirely. Every Lavichip device file is missing on
# the host, so the controllers fall back to in-memory simulator mode and the
# QML UI shows a yellow "SIMULATOR" banner with mode/deadman injectors.
#
# Usage:
#   scripts/host-build.sh           — configure + build
#   scripts/host-build.sh run       — build + launch
#   scripts/host-build.sh clean     — wipe build-host/
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-host"

case "${1:-}" in
    clean)
        echo "==> Removing $BUILD_DIR"
        rm -rf "$BUILD_DIR"
        exit 0
        ;;
esac

cd "$PROJECT_ROOT"

if ! command -v cmake >/dev/null; then
    echo "cmake gerekli: 'sudo apt install cmake ninja-build qtbase5-dev qtdeclarative5-dev'"
    exit 1
fi

echo "==> Configure (host Qt)"
cmake -S . -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DSMB_Q6R_BUILD_VENDOR_DEMOS=OFF

echo "==> Build"
cmake --build "$BUILD_DIR" -j"$(nproc)"

ls -lh "$BUILD_DIR/smb_q6r"

if [ "${1:-}" = "run" ]; then
    echo
    echo "==> Launching smb_q6r in simulator mode"
    cd "$BUILD_DIR"
    exec ./smb_q6r
fi
