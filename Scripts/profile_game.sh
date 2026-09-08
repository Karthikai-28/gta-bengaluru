#!/usr/bin/env bash
set -euo pipefail
# Run the full route for ten minutes after shader warm-up. CSV data is local under Saved/Profiling.
exec "$(dirname -- "${BASH_SOURCE[0]}")/launch_game.sh" \
    '-ExecCmds=t.MaxFPS 0,csvprofile start' -csvGpuStats -trace=cpu,gpu,frame,bookmark,memory "$@"
