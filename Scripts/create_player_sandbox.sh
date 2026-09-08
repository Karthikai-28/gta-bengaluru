#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"
[[ -x "$EDITOR" ]] || EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
rm -f "$PROJECT_ROOT/Saved/SandboxGeneration.complete"
"$EDITOR" "$PROJECT_FILE" -run=pythonscript \
    -script="$PROJECT_ROOT/Scripts/create_player_sandbox.py" \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput
[[ -f "$PROJECT_ROOT/Content/NammaCity/Maps/L_PlayerSandbox.umap" && -f "$PROJECT_ROOT/Saved/SandboxGeneration.complete" ]] || {
    echo "Sandbox map missing after generator; inspect Unreal logs." >&2
    exit 1
}
