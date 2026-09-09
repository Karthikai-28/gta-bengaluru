#!/usr/bin/env bash
# Rebuild everything from nothing: discard all generated output, recompile both
# targets, regenerate every piece of content, repackage, and verify.
#
# Deletes only regenerated, git-ignored directories. Source, Content and Art are
# never touched. Takes roughly seven minutes on the reference workstation and
# needs about 4 GB of free disk.
#
# Usage: Scripts/rebuild_all.sh [--no-verify]

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
cd "$PROJECT_ROOT" || exit 1

VERIFY=1
for arg in "$@"; do
    case "$arg" in
        --no-verify) VERIFY=0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

step() { printf '\n=== %s (%s) ===\n' "$1" "$(date +%T)"; }

step "Discard generated output"
rm -rf Intermediate Binaries Build DerivedDataCache \
       artifacts/Linux Saved/Cooked Saved/StagedBuilds Saved/Verify
df -h "$PROJECT_ROOT" | tail -1 | awk '{print "  disk free: "$4" ("$5" used)"}'

step "Host checks"
Scripts/test_host.sh

# The editor has to exist before any generator runs: they are Python scripts
# hosted by the editor, and the maps they build reference this project's C++.
step "Editor target"
Scripts/build_editor.sh

step "Content"
for generator in create_smoke_test_map create_player_sandbox setup_human_character setup_trees setup_cycle; do
    printf -- '-- %s\n' "$generator"
    "Scripts/$generator.sh"
done

step "Game target"
Scripts/build_game.sh
step "Package"
Scripts/package_game.sh

if (( VERIFY )); then
    step "Verify"
    Scripts/verify_build.sh
else
    printf '\nRebuilt. Run Scripts/verify_build.sh to check it, or Scripts/launch_game.sh to play.\n'
fi
