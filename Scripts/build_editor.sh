#!/usr/bin/env bash

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal

BUILD_SCRIPT="$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
if [[ ! -x "$BUILD_SCRIPT" ]]; then
    echo "Unreal build script is missing or not executable: $BUILD_SCRIPT" >&2
    exit 2
fi

exec "$BUILD_SCRIPT" NammaCityEditor Linux Development "$PROJECT_FILE" -WaitMutex -MaxParallelActions=2
