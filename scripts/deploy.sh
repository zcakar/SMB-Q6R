#!/usr/bin/env bash
# Deploy the stripped binary to the teach pendant and optionally launch it.
#
# Usage:
#   scripts/deploy.sh                  — deploy + run
#   scripts/deploy.sh --no-run         — copy only
#   scripts/deploy.sh --run-only       — skip copy, run what's there
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN_LOCAL="$PROJECT_ROOT/build-arm64/smb_q6r.stripped"
BIN_REMOTE_DIR="/home/Tronlong/smb-q6r"
BIN_REMOTE="${BIN_REMOTE_DIR}/smb_q6r"

HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"

SSH_OPTS=(
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o PreferredAuthentications=password
    -o PubkeyAuthentication=no
    -o LogLevel=ERROR
)

mode="${1:-deploy-and-run}"

if [[ "$mode" != "--run-only" ]]; then
    [[ -f "$BIN_LOCAL" ]] || { echo "Stripped binary yok: $BIN_LOCAL — önce 'scripts/docker-build.sh' çalıştırın."; exit 1; }
    size_kb=$(stat -c%s "$BIN_LOCAL")
    echo "[deploy] $(basename $BIN_LOCAL) ($((size_kb/1024)) KB) -> ${USER}@${HOST}:${BIN_REMOTE}"
    sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" "mkdir -p $BIN_REMOTE_DIR"
    sshpass -p '' scp "${SSH_OPTS[@]}" "$BIN_LOCAL" "${USER}@${HOST}:${BIN_REMOTE}"
    sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" "chmod +x $BIN_REMOTE"
fi

if [[ "$mode" == "--no-run" ]]; then
    echo "[deploy] copy only — done."
    exit 0
fi

echo "[deploy] launching on device (DISPLAY=:0, system Qt 5.12.8)"
# NOT sourcing qt_env.sh — we build against Qt 5.12 to match device system Qt
# exactly, no vendor 5.15.10 override needed. Set DISPLAY only.
sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" \
    "DISPLAY=:0 nohup $BIN_REMOTE >/tmp/smb_q6r.log 2>&1 & disown; sleep 1; tail -3 /tmp/smb_q6r.log; echo PID=\$!"
