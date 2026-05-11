#!/usr/bin/env bash
# Copy a built binary to the teach pendant and run it.
# Usage:
#   scripts/deploy.sh build/smb_q6r            — copy to /userfs/app/ and run
#   scripts/deploy.sh build/smb_q6r --no-run   — copy only
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <binary-path> [--no-run]"
  exit 1
fi

BIN="$1"
RUN_AFTER="${2:-}"
HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"
TARGET_DIR="/userfs/app/"

if [ ! -f "$BIN" ]; then
  echo "Error: binary not found: $BIN"
  exit 1
fi

SCP_OPTS=(
  -o StrictHostKeyChecking=no
  -o UserKnownHostsFile=/dev/null
  -o PreferredAuthentications=password
  -o PubkeyAuthentication=no
)

echo "[deploy] Copying $BIN -> ${USER}@${HOST}:${TARGET_DIR}"
sshpass -p '' scp "${SCP_OPTS[@]}" "$BIN" "${USER}@${HOST}:${TARGET_DIR}"

if [ "$RUN_AFTER" != "--no-run" ]; then
  BIN_NAME=$(basename "$BIN")
  echo "[deploy] Launching ${TARGET_DIR}${BIN_NAME} on device"
  sshpass -p '' ssh "${SCP_OPTS[@]}" -o ConnectTimeout=5 "${USER}@${HOST}" \
    "source /etc/profile.d/qt_env.sh && DISPLAY=:0 ${TARGET_DIR}${BIN_NAME}"
fi
