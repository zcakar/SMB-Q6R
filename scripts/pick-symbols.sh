#!/usr/bin/env bash
# Launch the SMB-Q6R PLC symbol picker (host-side Qt Widgets tool).
#
# Two modes selectable inside the picker UI:
#
#   1) Live PLC               — host connects directly via open62541
#   2) CodeSys Symbol XML     — offline parsing of CodeSys IDE export
#
# Usage:
#   scripts/pick-symbols.sh                              — open with live
#                                                          default endpoint
#   scripts/pick-symbols.sh --live opc.tcp://host:4840   — custom endpoint
#   scripts/pick-symbols.sh --xml path/to/symbols.xml    — open with XML
#                                                          tab pre-filled
#   scripts/pick-symbols.sh --xml ... --device 'MAT LC-C07'
#                                                       — override device
#
# Outputs:
#   symbols/symbols.json
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

DEFAULT_LIVE_URL="${SMB_Q6R_PLC:-opc.tcp://192.168.0.2:4840}"

ARGS=( "--out" "$ROOT/symbols/symbols.json" "--live" "$DEFAULT_LIVE_URL" )

while (( "$#" )); do
    case "$1" in
        --live)
            if [[ "${2:-}" =~ ^opc\.tcp:// ]]; then
                ARGS+=( "--live" "$2" ); shift 2
            else
                shift  # default already in ARGS
            fi
            ;;
        --xml)
            if [[ -n "${2:-}" ]]; then
                ARGS+=( "--xml" "$2" ); shift 2
            else
                echo "--xml requires a path argument"; exit 1
            fi
            ;;
        --device|-d)
            if [[ -n "${2:-}" ]]; then
                ARGS+=( "--device" "$2" ); shift 2
            else
                echo "--device requires a name argument"; exit 1
            fi
            ;;
        -h|--help)
            sed -n '2,22p' "$0"; exit 0
            ;;
        *)
            echo "unknown arg: $1"; exit 1
            ;;
    esac
done

BUILD_DIR="$ROOT/build-host/symbol-picker"
mkdir -p "$BUILD_DIR"
if [[ ! -f "$BUILD_DIR/symbol_picker" ]]; then
    echo "[pick-symbols] first-time build → $BUILD_DIR"
    cmake -S "$ROOT/tools/symbol-picker" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release >/dev/null
fi
cmake --build "$BUILD_DIR" -j"$(nproc)" >/dev/null

mkdir -p "$ROOT/symbols"
echo "[pick-symbols] output: $ROOT/symbols/symbols.json"
exec "$BUILD_DIR/symbol_picker" "${ARGS[@]}"
