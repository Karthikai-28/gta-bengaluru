#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
PROJECT_FILE="$PROJECT_ROOT/NammaCity.uproject"
export PROJECT_ROOT PROJECT_FILE

if [[ -f "$PROJECT_ROOT/.env" ]]; then
    # This file is local-only and is expected to contain trusted shell assignments.
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi

# Use a locally built Mesa when one is configured, for hosts whose distribution
# driver is too old for UE 5.8. See docs/phase-1/LINUX_GPU_DRIVER.md and
# Scripts/build_local_mesa.sh. Only this project's processes are affected; the
# desktop session keeps using the system driver.
if [[ -n "${MESA_PREFIX:-}" ]]; then
    _namma_icd="$MESA_PREFIX/share/vulkan/icd.d/intel_icd.x86_64.json"
    if [[ -f "$_namma_icd" ]]; then
        # VK_DRIVER_FILES is the current spelling; VK_ICD_FILENAMES is kept for
        # Vulkan loaders older than 1.3.207, which is what Ubuntu 22.04 ships.
        export VK_DRIVER_FILES="$_namma_icd"
        export VK_ICD_FILENAMES="$_namma_icd"
        export LD_LIBRARY_PATH="$MESA_PREFIX/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    else
        echo "MESA_PREFIX is set but no Vulkan ICD was found at $_namma_icd" >&2
        echo "Run Scripts/build_local_mesa.sh or unset MESA_PREFIX." >&2
        exit 2
    fi
    unset _namma_icd
fi

require_unreal() {
    if [[ -z "${UE_ROOT:-}" ]]; then
        echo "UE_ROOT is not set. Run python3 Scripts/configure_engine.py /absolute/engine/path." >&2
        exit 2
    fi

    UE_ROOT="$(realpath "$UE_ROOT")"
    if [[ ! -d "$UE_ROOT/Engine" ]]; then
        echo "UE_ROOT does not contain an Engine directory: $UE_ROOT" >&2
        exit 2
    fi
    python3 "$PROJECT_ROOT/Scripts/configure_engine.py" "$UE_ROOT" --check
}
