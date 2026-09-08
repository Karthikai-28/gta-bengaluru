#!/usr/bin/env bash

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal

EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
if [[ ! -x "$EDITOR" ]]; then
    echo "UnrealEditor is missing or not executable: $EDITOR" >&2
    exit 2
fi

exec "$EDITOR" "$PROJECT_FILE" -log
