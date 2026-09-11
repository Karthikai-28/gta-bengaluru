#!/usr/bin/env bash
# Run every check against whatever is currently built, and report a real verdict.
#
# The checks themselves are trustworthy; their exit codes are not. Unreal's
# editor returns 1 after -TestExit even when every automation test passed, and
# the packaged game returns 221 after RequestExitWithStatus(0). So each check is
# judged on what it wrote -- the automation report, and the playtest's own
# PASS/FAIL marker -- rather than on how the process happened to exit.
#
# Usage: Scripts/verify_build.sh [--editor-only]
#   --editor-only  skip the packaged-game playtest (needs no package)

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
cd "$PROJECT_ROOT" || exit 1

EDITOR_ONLY=0
for arg in "$@"; do
    case "$arg" in
        --editor-only) EDITOR_ONLY=1 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

LOGS="$PROJECT_ROOT/Saved/Verify"
REPORT="$PROJECT_ROOT/Saved/Automation/Human"
# The playtest writes these; clear them so a stale frame from an earlier run is
# never reported as evidence for this one.
FRAMES=(/tmp/namma-human-stand.png /tmp/namma-cycle-ride.png /tmp/namma-human-jab.png /tmp/namma-human-kick.png)
mkdir -p "$LOGS"
rm -f "${FRAMES[@]}"
FAILED=()

step() { printf '\n== %s ==\n' "$1"; }

step "Unreal automation tests"
rm -rf "$REPORT"
# Ignore the exit code; the report below is the verdict.
Scripts/test_human.sh > "$LOGS/automation.log" 2>&1 || true
if ! python3 - "$REPORT/index.json" <<'PY'
import json, sys, pathlib
path = pathlib.Path(sys.argv[1])
if not path.is_file():
    print("  no automation report was written; see Saved/Verify/automation.log")
    raise SystemExit(1)
report = json.loads(path.read_text(encoding="utf-8-sig"))
for test in report.get("tests", []):
    print(f"  {test.get('state'):8s} {test.get('fullTestPath')}")
    for entry in test.get("entries", []):
        event = entry.get("event", {})
        if event.get("type") == "Error":
            print(f"           {event.get('message')}")
passed, failed = report.get("succeeded", 0), report.get("failed", 0)
print(f"  {passed} passed, {failed} failed")
raise SystemExit(1 if failed or not passed else 0)
PY
then FAILED+=("automation tests"); fi

# The playtest logs its own verdict and saves two frames on the way through.
playtest() {
    local name="$1"; shift
    local log="$LOGS/playtest-$name.log"
    local passed
    step "Cycle playtest ($name)"
    Scripts/playtest_cycle.sh "$@" > "$log" 2>&1 || true
    passed="$(grep -c 'Cycle input check PASS' "$log" || true)"
    echo "  $passed checks passed"
    sed -n 's/.*\(Cycle input check FAIL.*\)/  \1/p' "$log"
    if ! grep -q 'NAMMA_CYCLE_PLAYTEST_PASS' "$log"; then
        # No marker at all means it crashed or never reached the end.
        grep -q 'NAMMA_CYCLE_PLAYTEST_FAIL' "$log" \
            || echo "  playtest did not finish; see Saved/Verify/playtest-$name.log"
        FAILED+=("cycle playtest ($name)")
    fi
}

playtest editor --editor
(( EDITOR_ONLY )) || playtest packaged

step "Result"
for frame in "${FRAMES[@]}"; do
    [[ -f "$frame" ]] && echo "  frame: $frame"
done
if (( ${#FAILED[@]} )); then
    printf '  FAILED: %s\n' "${FAILED[@]}"
    echo "  logs in Saved/Verify"
    exit 1
fi
echo "  All checks passed. Look at the frames above to judge how it actually looks."
