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

require_unreal() {
    if [[ -z "${UE_ROOT:-}" ]]; then
        echo "UE_ROOT is not set. Copy .env.example to .env and set the engine path." >&2
        exit 2
    fi

    UE_ROOT="$(realpath "$UE_ROOT")"
    if [[ ! -d "$UE_ROOT/Engine" ]]; then
        echo "UE_ROOT does not contain an Engine directory: $UE_ROOT" >&2
        exit 2
    fi
}
