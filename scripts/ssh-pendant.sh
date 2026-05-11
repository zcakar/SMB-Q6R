#!/usr/bin/env bash
# Quick SSH wrapper for the HN00-09Q6 teach pendant.
# Usage:
#   scripts/ssh-pendant.sh                       — interactive shell
#   scripts/ssh-pendant.sh "<remote command>"    — run command and exit
set -euo pipefail

HOST="${PENDANT_HOST:-192.168.1.245}"
USER="${PENDANT_USER:-Tronlong}"

SSH_OPTS=(
  -o StrictHostKeyChecking=no
  -o UserKnownHostsFile=/dev/null
  -o PreferredAuthentications=password
  -o PubkeyAuthentication=no
  -o ConnectTimeout=5
  -o LogLevel=ERROR
)

if [ $# -eq 0 ]; then
  exec sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}"
else
  exec sshpass -p '' ssh "${SSH_OPTS[@]}" "${USER}@${HOST}" "$@"
fi
