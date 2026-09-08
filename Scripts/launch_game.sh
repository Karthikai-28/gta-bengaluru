#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
GAME="$PROJECT_ROOT/artifacts/Linux/Linux/NammaCity.sh"
[[ -f "$GAME" ]] || { echo "Package missing: run Scripts/package_game.sh first." >&2; exit 2; }
exec bash "$GAME" -windowed -ResX=1280 -ResY=720 -vulkan -log "$@"
