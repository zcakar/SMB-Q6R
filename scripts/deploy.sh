#!/usr/bin/env bash
# Deploy SMB-Q6R + vendor reference demos to the teach pendant.
#
# Default behaviour:
#   * scp build-arm64/smb_q6r.stripped → /home/Tronlong/smb-q6r/smb_q6r
#   * scp build-arm64/vendor/*.stripped → /home/Tronlong/vendor-demos/
#   * kill any currently-running smb_q6r on device
#   * launch smb_q6r with DISPLAY=:0
#
# Flags:
#   --no-run            copy only, don't launch smb_q6r
#   --no-vendor         skip vendor demos, only copy smb_q6r
#   --run-vendor=NAME   launch vendor_NAME instead of smb_q6r (e.g. led, button)
#   --kill-only         just kill running smb_q6r and vendor demos, no copy
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN_LOCAL="$PROJECT_ROOT/build-arm64/smb_q6r.stripped"
VENDOR_DIR_LOCAL="$PROJECT_ROOT/build-arm64/vendor"

HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"

REMOTE_APP_DIR="/home/${USER}/smb-q6r"
REMOTE_VENDOR_DIR="/home/${USER}/vendor-demos"
REMOTE_APP="${REMOTE_APP_DIR}/smb_q6r"

SSH_OPTS=(
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o PreferredAuthentications=password
    -o PubkeyAuthentication=no
    -o LogLevel=ERROR
)

SSH() { sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" "$@"; }
SCP() { sshpass -p '' scp "${SSH_OPTS[@]}" "$@"; }

skip_run=0
skip_vendor=0
run_vendor=""
kill_only=0
for arg in "$@"; do
    case "$arg" in
        --no-run) skip_run=1 ;;
        --no-vendor) skip_vendor=1 ;;
        --run-vendor=*) run_vendor="${arg#*=}"; skip_vendor=0 ;;
        --kill-only) kill_only=1 ;;
        *) echo "Bilinmeyen argüman: $arg"; exit 1 ;;
    esac
done

echo "[deploy] target: ${USER}@${HOST}"

# Always stop running instances before redeploy / launch.
# Use 'killall' (matches basename only) not 'pkill -f' (which matches the
# whole cmdline and accidentally kills the remote bash that's invoking it,
# since our path appears as a literal argument in that bash's cmdline).
SSH "killall smb_q6r 2>/dev/null; for v in vendor_led vendor_button vendor_matrixkeys vendor_pwm vendor_rotaryencoder vendor_wheel vendor_backlight; do killall \$v 2>/dev/null; done; true"

if [[ $kill_only -eq 1 ]]; then
    echo "[deploy] kill-only; done."
    exit 0
fi

# --- Copy smb_q6r ---
if [[ -f "$BIN_LOCAL" ]]; then
    sz=$(stat -c%s "$BIN_LOCAL")
    echo "[deploy] smb_q6r ($((sz/1024)) KB) -> ${REMOTE_APP}"
    SSH "mkdir -p ${REMOTE_APP_DIR}"
    SCP "$BIN_LOCAL" "${USER}@${HOST}:${REMOTE_APP}"
    SSH "chmod +x ${REMOTE_APP}"
else
    echo "[deploy] uyarı: $BIN_LOCAL yok; smb_q6r atlanıyor"
fi

# --- Copy vendor demos ---
if [[ $skip_vendor -eq 0 ]] && [[ -d "$VENDOR_DIR_LOCAL" ]]; then
    vendor_bins=( "$VENDOR_DIR_LOCAL"/*.stripped )
    if [[ -f "${vendor_bins[0]}" ]]; then
        total=$(du -bc "${vendor_bins[@]}" | tail -1 | awk '{print $1}')
        echo "[deploy] vendor demos ($((total/1024)) KB, ${#vendor_bins[@]} files) -> ${REMOTE_VENDOR_DIR}/"
        SSH "mkdir -p ${REMOTE_VENDOR_DIR}"
        for b in "${vendor_bins[@]}"; do
            base=$(basename "$b" .stripped)
            SCP "$b" "${USER}@${HOST}:${REMOTE_VENDOR_DIR}/${base}"
        done
        SSH "chmod +x ${REMOTE_VENDOR_DIR}/vendor_*"
    fi
fi

if [[ $skip_run -eq 1 ]]; then
    echo "[deploy] copy only — done."
    exit 0
fi

# --- Launch ---
if [[ -n "$run_vendor" ]]; then
    cmd="${REMOTE_VENDOR_DIR}/vendor_${run_vendor}"
    label="vendor_${run_vendor}"
else
    cmd="${REMOTE_APP}"
    label="smb_q6r"
fi

echo "[deploy] launching ${label} (DISPLAY=:0)"
SSH "DISPLAY=:0 nohup ${cmd} >/tmp/${label}.log 2>&1 & disown; sleep 1; tail -5 /tmp/${label}.log; ps -C $(basename ${cmd}) -o pid,etime 2>/dev/null"
