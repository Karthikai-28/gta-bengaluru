#!/usr/bin/env bash
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
# Preserve Epic's package paths so skeleton/material/animation references resolve.
CHARACTER_SOURCE="$UE_ROOT/Templates/TemplateResources/High/Characters/Content/Mannequins"
[[ -d "$CHARACTER_SOURCE" ]] || { echo "Bundled mannequin resources missing" >&2; exit 2; }
mkdir -p "$PROJECT_ROOT/Content/Characters"
cp -rn "$CHARACTER_SOURCE" "$PROJECT_ROOT/Content/Characters/"
exec "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_FILE" \
    -run=pythonscript -script="$PROJECT_ROOT/Scripts/setup_human_character.py" \
    -unattended -nop4 -nosplash -NullRHI -stdout -FullStdOutLogOutput
