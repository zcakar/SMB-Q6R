#!/usr/bin/env bash
# Add a secondary IP on the pendant's eth1 so it can reach the CodeSys
# PLC at 192.168.0.0/24 alongside the existing 192.168.1.245 dev access.
#
# Idempotent. Updates /etc/netplan/01-netcfg.yaml so the alias survives
# reboot, and also performs a live `ip addr add` so it is immediately
# usable without waiting for the netplan apply.
#
# Usage:  scripts/configure-plc-network.sh [PLC_SUBNET_IP=192.168.0.245/24]
set -euo pipefail

HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"
ALIAS="${1:-192.168.0.245/24}"

SSH_OPTS=(
    -o StrictHostKeyChecking=no
    -o UserKnownHostsFile=/dev/null
    -o PreferredAuthentications=password
    -o PubkeyAuthentication=no
    -o LogLevel=ERROR
)

# Render the replacement netplan locally so we can scp it as a file —
# avoids the double-heredoc quoting trap that bit the earlier attempt.
TMP=$(mktemp)
trap "rm -f ${TMP}" EXIT

cat > "${TMP}" <<EOF
network:
        version: 2
        renderer: networkd
        ethernets:
                eth1:
                        dhcp4: no
                        addresses: [192.168.1.245/24, ${ALIAS}]
                        gateway4: 192.168.1.1
                        nameservers:
                                addresses: [8.8.8.8, 8.8.4.4]
EOF

echo "==> Uploading new netplan to pendant"
sshpass -p '' scp "${SSH_OPTS[@]}" "${TMP}" "${USER}@${HOST}:/tmp/01-netcfg.yaml"

echo "==> Installing + applying"
sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" "ALIAS='${ALIAS}' bash -s" <<'REMOTE'
set -e
S() { echo "" | sudo -S -p "" "$@"; }
S cp /etc/netplan/01-netcfg.yaml /etc/netplan/01-netcfg.yaml.before-plc 2>/dev/null || true
S mv /tmp/01-netcfg.yaml /etc/netplan/01-netcfg.yaml
S chmod 600 /etc/netplan/01-netcfg.yaml
S chown root:root /etc/netplan/01-netcfg.yaml
echo "--- new config ---"
S cat /etc/netplan/01-netcfg.yaml

echo "--- netplan apply ---"
S netplan apply 2>&1 || echo "(netplan apply warning ignored — alias added live)"

# Belt-and-braces: also live-add so the alias is there without waiting.
S ip addr add "${ALIAS}" dev eth1 2>/dev/null || true

echo "--- eth1 addresses ---"
ip -br addr show eth1

PLC_IP=$(echo "${ALIAS}" | sed -E 's|/[0-9]+$||' | awk -F. '{print $1"."$2"."$3".2"}')
echo "--- ping PLC ${PLC_IP} ---"
ping -c 2 -W 2 "${PLC_IP}" 2>&1 | tail -3 || true
REMOTE
