#!/usr/bin/env bash
# Install device permission rules on the teach pendant so that the SMB-Q6R
# application can drive the Lavichip hardware without running as root.
#
# This script SSHes to the pendant and runs sudo there. It is idempotent.
#
# After it runs you must LOG OUT of the X session (or reboot) for the new
# group memberships to take effect. The udev rule changes take effect on
# next device hot-add or after `udevadm trigger`.
#
# Usage:  scripts/device-permissions.sh
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

# Build the remote script content. It will be transferred via SSH stdin then
# executed via sudo bash. The user will be prompted for the sudo password
# interactively at the SSH session.
REMOTE_SCRIPT=$(cat <<'EOF'
set -euo pipefail

echo "==> Installing /etc/udev/rules.d/99-smb-q6r.rules"
sudo tee /etc/udev/rules.d/99-smb-q6r.rules > /dev/null <<'UDEV'
# SMB-Q6R Teach Pendant hardware access rules.
# Grant the plugdev group read/write access to the Lavichip kernel char
# devices and the matrix-keypad / rotary evdev nodes. The Tronlong default
# user is already in plugdev and input groups (the latter added by this
# install script).

# Lavichip vendor char devices (LEDs, mode switch, enable switch, buzzer)
KERNEL=="leds",       MODE="0660", GROUP="plugdev", TAG+="uaccess"
KERNEL=="buttons",    MODE="0660", GROUP="plugdev", TAG+="uaccess"
KERNEL=="buttonstop", MODE="0660", GROUP="plugdev", TAG+="uaccess"
KERNEL=="pwm",        MODE="0660", GROUP="plugdev", TAG+="uaccess"

# Backlight sysfs is also root-owned by default
ACTION=="add", SUBSYSTEM=="backlight", \
    RUN+="/bin/sh -c 'chgrp plugdev /sys/class/backlight/$kernel/brightness && chmod g+w /sys/class/backlight/$kernel/brightness'"
UDEV

echo "==> Adding Tronlong to the 'input' group (matrix keys + wheel)"
sudo usermod -a -G input "$USER"

echo "==> Reloading udev"
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=misc
sudo udevadm trigger --subsystem-match=input

# Backlight sysfs needs a kick because it was set up before the rule existed
sudo chgrp plugdev /sys/class/backlight/backlight/brightness 2>/dev/null || true
sudo chmod g+w /sys/class/backlight/backlight/brightness 2>/dev/null || true

# Char devices: also fix their permissions in place
sudo chmod 0660 /dev/leds /dev/buttons /dev/buttonstop /dev/pwm 2>/dev/null || true
sudo chgrp plugdev /dev/leds /dev/buttons /dev/buttonstop /dev/pwm 2>/dev/null || true

echo "==> Verification"
ls -la /dev/leds /dev/buttons /dev/buttonstop /dev/pwm /dev/input/event0 /dev/input/event1
echo
echo "Tronlong grupları (yeniden login sonrası geçerli olacak):"
id "$USER" | tr ',' '\n' | grep -E "input|plugdev"

echo
echo "===================================================================="
echo " Tamam. Şimdi X oturumundan ÇIKIŞ yapıp tekrar girmek ya da REBOOT"
echo " etmek gerekir — 'input' grubu üyeliği o zaman aktive olur."
echo " Hızlı yol: 'sudo reboot' (cihaz 30 sn'de geri açılır)"
echo "===================================================================="
EOF
)

echo "Cihazda izin kurulumu yapılıyor. Sudo şifresi sorulacak."
echo

sshpass -p '' ssh "${SSH_OPTS[@]}" -t "${USER}@${HOST}" "USER=${USER} bash -s" <<< "$REMOTE_SCRIPT"
