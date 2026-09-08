#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
python3 Scripts/validate_repository.py
python3 -m unittest discover -s Tests -p 'test_*.py' -v
TEST_BINARY="$(mktemp /tmp/namma-delivery-test.XXXXXX)"
trap 'rm -f "$TEST_BINARY"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror Tests/delivery_state_test.cpp -o "$TEST_BINARY"
"$TEST_BINARY"
echo "Host checks passed. These do not compile or launch Unreal."
