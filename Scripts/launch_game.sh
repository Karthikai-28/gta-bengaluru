#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
# UAT archives straight into the archive directory on UE 5.8; older layouts nested
# the build under a second platform directory. Accept either.
GAME="$(find "$PROJECT_ROOT/artifacts/Linux" -maxdepth 2 -name NammaCity.sh -type f 2>/dev/null | head -1)"
[[ -n "$GAME" ]] || { echo "Package missing: run Scripts/package_game.sh first." >&2; exit 2; }
exec bash "$GAME" -windowed -ResX=1280 -ResY=720 -vulkan -log "$@"
