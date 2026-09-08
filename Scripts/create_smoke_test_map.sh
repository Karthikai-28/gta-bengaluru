#!/usr/bin/env bash

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal

EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
GENERATOR="$PROJECT_ROOT/Scripts/create_smoke_test_map.py"
if [[ ! -x "$EDITOR" ]]; then
    echo "UnrealEditor is missing or not executable: $EDITOR" >&2
    exit 2
fi

exec "$EDITOR" "$PROJECT_FILE" \
    -ExecutePythonScript="$GENERATOR" \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput
