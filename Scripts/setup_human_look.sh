#!/usr/bin/env bash
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
command -v blender >/dev/null || { echo "Blender is required to build the human source mesh" >&2; exit 2; }
# The FBX skeletal-mesh exporter builds material-baking data from a temporary
# skeletal mesh component, which asserts unless the component has render state.
# Commandlets skip render state under -NullRHI, so ask for offscreen rendering.
EDITOR=("$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_FILE" -run=pythonscript
        -unattended -nop4 -nosplash -RenderOffscreen -vulkan -AllowCommandletRendering
        -stdout -FullStdOutLogOutput)
"${EDITOR[@]}" -script="$PROJECT_ROOT/Scripts/export_human_reference.py"
blender -b --factory-startup -noaudio --python-exit-code 1 \
    --python "$PROJECT_ROOT/Scripts/build_human_look.py" -- \
    --reference "$PROJECT_ROOT/Saved/HumanLook/Manny.fbx" --output "$PROJECT_ROOT/Art/Characters/Production"
blender -b --factory-startup -noaudio "$PROJECT_ROOT/Art/Characters/Production/NammaMan.blend"     --python-exit-code 1 --python "$PROJECT_ROOT/Scripts/test_human_art.py"
python3 "$PROJECT_ROOT/Scripts/render_human_preview.py" "$PROJECT_ROOT/Art/Characters/Production/review-mesh.json"     "$PROJECT_ROOT/Art/Characters/Production/human-look-preview.png"
"${EDITOR[@]}" -script="$PROJECT_ROOT/Scripts/import_human_look.py"
