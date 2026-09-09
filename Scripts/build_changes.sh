#!/usr/bin/env bash
# Compile what changed, repackage, and verify. This is the everyday loop.
#
# Unreal Build Tool decides for itself what needs recompiling, so this is safe
# to run after any C++ edit. It does NOT regenerate maps or content: if you
# changed a generator under Scripts/ or want a guaranteed-clean tree, use
# Scripts/rebuild_all.sh instead.
#
# Usage: Scripts/build_changes.sh [--no-package] [--no-verify]
#   --no-package  build both targets only (implies --no-verify)
#   --no-verify   build and package, but skip the checks

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
cd "$PROJECT_ROOT" || exit 1

PACKAGE=1
VERIFY=1
for arg in "$@"; do
    case "$arg" in
        --no-package) PACKAGE=0; VERIFY=0 ;;
        --no-verify) VERIFY=0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

step() { printf '\n=== %s (%s) ===\n' "$1" "$(date +%T)"; }

step "Editor target"
Scripts/build_editor.sh
step "Game target"
Scripts/build_game.sh

if (( PACKAGE )); then
    step "Package"
    Scripts/package_game.sh
fi

if (( VERIFY )); then
    step "Verify"
    Scripts/verify_build.sh
else
    printf '\nBuilt. Run Scripts/verify_build.sh to check it, or Scripts/launch_game.sh to play.\n'
fi
