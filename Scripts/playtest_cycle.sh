#!/usr/bin/env bash
# Opt-in keyboard-driven test; saves /tmp/namma-human-stand.png, /tmp/namma-cycle-ride.png, /tmp/namma-human-jab.png and /tmp/namma-human-kick.png.
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
ARGS=(-RenderOffscreen -unattended -nocrashreports -benchmark -fps=30 -seconds=25
      -windowed -ResX=1280 -ResY=720 -vulkan -stdout '-ExecCmds=Namma.Cycle.Playtest')
if [[ "${1:-}" == "--editor" ]]; then
    shift
    exec timeout 120s "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_FILE" -game "${ARGS[@]}" "$@"
fi
exec timeout 120s "$PROJECT_ROOT/Scripts/launch_game.sh" "${ARGS[@]}" "$@"
