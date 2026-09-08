# Phase 1A — First Delivery

Updated 2026-09-08. **Implemented source, not yet an engine-verified playable.**

## In the repository

- Third-person native character and collision-aware spring-arm camera; Enhanced Input keyboard/mouse mappings.
- Jog, sprint, jump; primitive visible body and carried parcel; out-of-bounds return to the starting transform.
- Reusable native interaction interface: prompt, availability, execution. Camera visibility trace plus 2.6 m player-distance check.
- Delivery rules: awaiting pickup → carrying → complete. Duplicate pickup/delivery rejected; restart resets state. Recovery preserves the parcel to avoid stranding an active delivery.
- Native game mode owns delivery state and broadcasts objective changes. HUD displays objective, parcel status, active destination distance, crosshair and interaction prompt.
- Pause, resume, restart and quit controls; restart allowed from pause or completion.
- Deterministic map/material generator, six fictional shop shells, instanced props, courtyard ramp, trees and decorative auto-rickshaw. One source layout also generates the annotated SVG.
- Low-spec render settings, two-worker build limits, installed-engine pin/configuration tool, read-only workstation/ZIP-size checks, map generation, Linux packaging, launch and profiling scripts.
- Host-side delivery-rule and layout checks, plus a native Unreal automation test for delivery rules.

## Verification boundary

Host checks can validate delivery transitions and layout geometry. They cannot establish Unreal API compatibility, input behavior, generated materials, GPU support, collision feel or packaged performance.

No `UE_ROOT` or actual engine version has been configured. `Config/UnrealVersion.json` is deliberately created only by selecting a real installed build. No `.umap`, `.uasset`, package, engine screenshot or measured performance claim has been fabricated.

Current blockers: Epic authenticated engine download/install and verification that the selected build fits the remaining 45.76 GiB. Host Intel hardware Vulkan enumeration has now passed, and a user-local vulkaninfo diagnostic is installed. [Storage review](STORAGE_REVIEW.md) records the completed, user-authorized cache cleanup. [Desktop setup](../namma-city/docs/software/UBUNTU_22_04_SETUP.md) closes the prerequisites.

## Next engine session

1. Configure and pin installed UE5; verify hardware Vulkan.
2. Compile editor, generate both maps, compile game, package and launch.
3. Run `NammaCity.Delivery.Rules` in Session Frontend's Automation tab.
4. Execute [PLAYTEST.md](PLAYTEST.md), fix engine/API or gameplay failures, then record actual results and screenshots.
5. Mark Phase 1A playable only after all runtime gates pass. Keep Phase 0 open until its build/package/launch and operational requirements are satisfied.

## Later Phase 1B

Gamepad, mantle/vault, health/damage/death, general inventory and broader movement polish. No driving, combat, police, crowds, mission framework, or saves in Phase 1A.

## Host verification completed

- `Scripts/test_host.sh`: passed repository validation, three layout tests, and the compiled engine-independent delivery-rule test (100 repeat cycles plus invalid transitions).
- `python3 -m compileall -q Scripts Tests`: passed Python syntax checks, including the editor generator; Unreal imports/APIs were not executed.
- `shellcheck Scripts/*.sh` and `git diff --check`: passed.
- Annotated SVG rendered and visually inspected; it is a concept plan, not a rendered Unreal scene.
- `Scripts/check_workstation.py --desktop` and `Scripts/build_editor.sh`: correctly stopped with missing-engine/graphics-prerequisite messages. No compile was attempted by Unreal.
- Centralized Graphify code/document structure refreshed and merged into the shared brain. Graph extraction describes source structure; runtime verification remains governed by this status file.

Follow-up: hardware Vulkan enumeration passed outside the sandbox, but no engine archive, version pin, compile, package or runtime performance result exists yet. Epic sign-in/download access is required to continue installation.
