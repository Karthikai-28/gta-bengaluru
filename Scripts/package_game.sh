#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
for map in L_SmokeTest L_PlayerSandbox; do
    [[ -f "$PROJECT_ROOT/Content/NammaCity/Maps/$map.umap" ]] || {
        echo "Missing $map. Build the editor and run both map generators first." >&2
        exit 2
    }
done
exec "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
    -project="$PROJECT_FILE" -noP4 -platform=Linux -clientconfig=Development \
    -build -cook -stage -pak -package -archive \
    -archivedirectory="$PROJECT_ROOT/artifacts/Linux" \
    '-map=/Game/NammaCity/Maps/L_PlayerSandbox+/Game/NammaCity/Maps/L_SmokeTest' \
    '-UbtArgs=-MaxParallelActions=2' -unattended -utf8output
