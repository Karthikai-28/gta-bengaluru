#!/usr/bin/env bash
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
exec "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_FILE" \
    -run=pythonscript -script="$PROJECT_ROOT/Scripts/setup_cycle.py" \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput
