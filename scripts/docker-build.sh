#!/usr/bin/env bash
# Cross-compile SMB-Q6R inside the Focal-arm64 builder container.
#
# Usage:
#   scripts/docker-build.sh                 — incremental build
#   scripts/docker-build.sh clean           — remove build dir first
#   scripts/docker-build.sh rebuild-image   — also rebuild the Docker image
#
# Output: build-arm64/smb_q6r (aarch64 ELF, Focal glibc 2.31 compatible)
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="smb-q6r-builder:focal-arm64"
BUILD_DIR="$PROJECT_ROOT/build-arm64"

# --- Pick docker invocation (rootless or sudo) ---
if docker info >/dev/null 2>&1; then
    DOCKER="docker"
elif sudo -n docker info >/dev/null 2>&1; then
    DOCKER="sudo docker"
else
    echo "docker daemon erişilemiyor. 'sudo systemctl start docker' veya kullanıcıyı 'docker' grubuna ekleyin."
    exit 1
fi

# --- Image present? ---
need_build_image=0
if [[ "${1:-}" == "rebuild-image" ]]; then
    need_build_image=1
    shift
fi
if ! $DOCKER image inspect "$IMAGE" >/dev/null 2>&1; then
    need_build_image=1
fi

if [[ $need_build_image -eq 1 ]]; then
    echo "==> Builder image yok ya da yeniden istek; oluşturuluyor..."
    $DOCKER build -t "$IMAGE" -f "$PROJECT_ROOT/docker/Dockerfile" "$PROJECT_ROOT/docker/"
fi

# --- Optional clean ---
if [[ "${1:-}" == "clean" ]]; then
    echo "==> build-arm64/ temizleniyor"
    rm -rf "$BUILD_DIR"
fi

# --- Run cross-compile ---
echo "==> Cross-compile (Docker / Focal-arm64 / Qt 5.12.8)"
$DOCKER run --rm \
    -v "$PROJECT_ROOT":/src:rw \
    -u "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    "$IMAGE" \
    "cd /src && \
     cmake -B build-arm64 -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -S . && \
     cmake --build build-arm64 -j\$(nproc)"

echo
echo
echo "==> Build çıktısı:"
file "$BUILD_DIR/smb_q6r"
ls -lh "$BUILD_DIR/smb_q6r" "$BUILD_DIR/smb_q6r.stripped" 2>/dev/null
echo
echo "Deploy: scp $BUILD_DIR/smb_q6r.stripped Tronlong@192.168.1.245:/home/Tronlong/smb-q6r/smb_q6r"
