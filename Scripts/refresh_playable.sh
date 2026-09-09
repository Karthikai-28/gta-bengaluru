#!/usr/bin/env bash
# Rebuild, persist world/material changes and test both builds.
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
Scripts/test_host.sh
Scripts/build_editor.sh
Scripts/setup_human_character.sh
Scripts/setup_cycle.sh
Scripts/test_human.sh
Scripts/playtest_cycle.sh --editor
Scripts/package_game.sh
Scripts/playtest_cycle.sh
