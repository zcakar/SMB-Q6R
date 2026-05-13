#!/usr/bin/env bash
# Install the smb-q6r systemd --user unit on the pendant so the UI launches
# automatically when the Tronlong XFCE session starts.
#
# Idempotent: re-running just updates the unit file and reloads systemd.
#
# Usage:  scripts/install-autostart.sh
set -euo pipefail

HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"

SSH_OPTS=(
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o PreferredAuthentications=password
    -o PubkeyAuthentication=no
    -o LogLevel=ERROR
)

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UNIT_FILE="$PROJECT_ROOT/scripts/systemd/smb-q6r.service"

if [ ! -f "$UNIT_FILE" ]; then
    echo "Unit file missing: $UNIT_FILE" >&2
    exit 1
fi

echo "==> Copying unit file to pendant"
sshpass -p '' scp "${SSH_OPTS[@]}" "$UNIT_FILE" "${USER}@${HOST}:/tmp/smb-q6r.service"

echo "==> Installing into ~/.config/systemd/user/"
sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" 'bash -s' <<'REMOTE_EOF'
set -e
mkdir -p ~/.config/systemd/user
mv /tmp/smb-q6r.service ~/.config/systemd/user/smb-q6r.service

# Enable lingering so the user manager starts at boot even before login.
# (Tronlong is auto-logged into XFCE on this image; this is a belt-and-braces
# step that lets the unit also work for headless SSH-only sessions.)
echo "" | sudo -S loginctl enable-linger Tronlong

systemctl --user daemon-reload
systemctl --user enable smb-q6r.service
systemctl --user restart smb-q6r.service

sleep 1
echo
echo "==> Status"
systemctl --user --no-pager status smb-q6r.service | head -12
REMOTE_EOF

echo
echo "Done. Reboot to confirm auto-start works from a cold boot:"
echo "  ssh ${USER}@${HOST} 'sudo reboot'"
