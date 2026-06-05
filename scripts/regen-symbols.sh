#!/usr/bin/env bash
# Convenience wrapper for scripts/regen-symbols.py.
#
# Reads symbols/symbols.json (picker output) and writes:
#   src/generated/plc_symbols.h        — C++ constexpr literals
#   qml/generated/PlcSymbols.qml       — QML singleton
#   qml/generated/qmldir               — singleton registration
set -euo pipefail
cd "$(dirname "$0")/.."
exec python3 scripts/regen-symbols.py "$@"
