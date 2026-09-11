#!/usr/bin/env bash
# Rebuild the strike motion templates from reference footage.
#
# Fetches the tutorial clips listed in Art/Motion/reference_clips.json (with
# their auto-captions), transcodes anything OpenCV cannot decode, tracks the
# fighter's 3D skeleton through every clip, and reduces the strikes to
# Art/Motion/strikes.json and Source/NammaCity/NammaStrikeData.h.
#
# The videos stay in Saved/MotionReference (ignored by git). Only the derived
# joint trajectories are committed. Needs yt-dlp (a current one: YouTube
# rejects builds older than a few months), ffmpeg, and Python with mediapipe,
# opencv and numpy.
#
# Usage: Scripts/build_strike_motion.sh [--review]
#   --review   also write a contact sheet per detected strike to
#              Saved/MotionReference/review for eyeballing the labels

set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
export PATH="$HOME/.local/bin:$PATH"
REVIEW=()
for arg in "$@"; do
    case "$arg" in
        --review) REVIEW=(--frames Saved/MotionReference/review) ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done
for tool in yt-dlp ffmpeg python3; do
    command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }
done

DIR=Saved/MotionReference
mkdir -p "$DIR"
mapfile -t IDS < <(python3 -c "import json;print('\n'.join(c['id'] for c in json.load(open('Art/Motion/reference_clips.json'))['clips']))")

echo "== Fetch"
for id in "${IDS[@]}"; do
    if [[ ! -f "$DIR/$id.mp4" ]]; then
        # Best stream up to 1080p, then re-encode to H.264: YouTube serves AV1
        # and VP9, which the system OpenCV cannot decode.
        yt-dlp -q --no-warnings -f "bv*[height<=1080]+ba/b[height<=1080]/b" --merge-output-format mp4 \
            -o "$DIR/$id.raw.%(ext)s" "https://www.youtube.com/watch?v=$id"
        raw="$(ls "$DIR/$id".raw.* | head -1)"
        ffmpeg -v error -y -i "$raw" -vf "scale='min(1280,iw)':-2" -c:v libx264 -preset veryfast -crf 20 -an "$DIR/$id.mp4"
        rm -f "$raw"
    fi
    if [[ ! -f "$DIR/$id.en.vtt" ]]; then
        # Captions label the technique being taught; a rate limit here is not fatal.
        yt-dlp -q --no-warnings --write-auto-subs --sub-langs en --skip-download -o "$DIR/%(id)s" \
            "https://www.youtube.com/watch?v=$id" || echo "  no captions for $id"
    fi
done

echo "== Track"
TODO=()
for id in "${IDS[@]}"; do
    [[ -f "$DIR/$id.pose.npz" ]] || TODO+=("$DIR/$id.mp4")
done
if (( ${#TODO[@]} )); then
    python3 Scripts/track_reference_pose.py "${TODO[@]}" --jobs 6 2>&1 | grep -E "frames ->|Error|error"
fi

echo "== Extract"
rm -rf "$DIR/review"
python3 Scripts/extract_strike_motion.py "${REVIEW[@]}" 2>&1 | grep -v -i "warning"

echo "== Check"
python3 Scripts/validate_repository.py
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -O2 Tests/strike_motion_test.cpp -o "$TMP/strike" && "$TMP/strike"
echo "Templates rebuilt. Recompile the game (Scripts/build_changes.sh) to use them."
