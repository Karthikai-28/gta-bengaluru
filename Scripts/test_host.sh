#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
python3 Scripts/validate_repository.py
python3 -m unittest discover -s Tests -p 'test_*.py' -v
TEST_DIR="$(mktemp -d /tmp/namma-host-tests.XXXXXX)"
trap 'rm -rf "$TEST_DIR"' EXIT
for TEST in delivery_state bicycle_physics human_vitals punch_motion; do
    "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -O2 "Tests/${TEST}_test.cpp" -o "$TEST_DIR/$TEST"
    "$TEST_DIR/$TEST"
done
echo "Host checks passed. These do not compile or launch Unreal."
