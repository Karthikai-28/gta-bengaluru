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

## Engine archive inspected

The supplied Unreal 5.8.2 ZIP is 37.08 GiB compressed and 71.68 GiB unpacked. With 45.76 GiB free, the unpacked engine cannot fit. No full download or installation was started. See [capacity check](ENGINE_INSTALL.md). Checking other mounted storage was rejected by automatic approval review because workspace credits are exhausted.

Latest rescan (10:20 UTC): 111.8 GiB free, enough for the unpacked engine and about 40 GiB remaining afterward. The supplied Epic URL now returns HTTP 403 / Request has expired; a fresh signed link is the immediate blocker. See [installation capacity history](ENGINE_INSTALL.md).

## Runtime gates executed — 2026-09-08

The engine blockers recorded above are resolved and now historical. UE 5.8.2 is
installed and pinned, and the Vulkan crash that prevented any frame from
rendering is fixed by a locally built Mesa 26.2.2 (see
[Linux GPU driver setup](LINUX_GPU_DRIVER.md)).

| Gate | Result |
|---|---|
| Editor compiles | pass |
| Both maps generated | pass — `L_SmokeTest`, `L_PlayerSandbox` |
| Game runs, renders, hardware Vulkan | pass — Intel Iris Xe (RPL-P), Mesa 26.2.2, not llvmpipe |
| `NammaCity.Delivery.Rules` in-engine | **pass** — `Result={Success}`, first in-engine run |
| Linux Development package | **pass** — 1.1 GiB in `artifacts/Linux`, build/cook/stage/package/archive all clean |
| Packaged game launches and renders | pass |
| Movement, interaction, delivery transition | pass — walked to the pickup, `E` took the parcel, objective advanced to the delivery leg |
| Ten-minute route test, p95 ≤ 33.3 ms | **pass (scripted route)** — see below |

Two defects were found and fixed while running these:

- The pawn spawned with `MovementMode = MOVE_None`, leaving it inert — no
  walking, jumping or gravity. Corrected at `BeginPlay`, which logs when it
  fires. The warning also appears in the packaged build, so the underlying cause
  is not editor-specific and is still unexplained.
- `Scripts/launch_game.sh` looked for the packaged launcher one directory too
  deep and always reported "Package missing". Never seen before because the
  project had not been packaged.

Remaining for M1A: the ten-minute route test in [PLAYTEST.md](PLAYTEST.md) and
the p95 frame-time capture via `Scripts/profile_game.sh`. Those are the only
gates left before Phase 1A can be called playable.

### Frame-time capture — 2026-09-08

Packaged Development build, 1280 × 720 windowed, `t.MaxFPS 0` so the 33.3 ms
budget is measured independently of the shipping 30 fps limiter. Captured with
`-csvCaptureFrames`, because `csvprofile stop` needs the console and this build
binds no console key (see the note below).

| Metric | Value | Gate |
|---|---|---|
| Measured window | 680.7 s, 124,684 frames | ≥ 600 s — pass |
| Mean frame time | 5.46 ms | — |
| p95 frame time | **7.12 ms** | ≤ 33.3 ms — **pass**, 4.7× headroom |
| Peak RSS (`VmHWM`) | 1049 MB | 3 GiB envelope — pass |
| Renderer | Intel Iris Xe (RPL-P), Mesa 26.2.2 | hardware Vulkan — pass |

No crash, soft lock or driver fault across the run.

**What this measurement is not.** The route was driven by a scripted input loop
(walk, sprint, jump, turn, strafe on repeat), not by a person playing. It
exercises movement, camera motion and the render path under continuous load, and
it satisfies the numeric gate, but it does not cover camera clipping against
geometry, ramp and sidewalk transitions, interaction edge cases, or how the
controls feel. The PLAYTEST.md acceptance items still need a human pass.

**Console key is unbound.** PLAYTEST.md step 3 says to open the console and run
`csvprofile stop`, but no `ConsoleKeys` is set in `Config/DefaultInput.ini`, so
the console cannot be opened in the packaged build. Either bind a console key or
change that step to the `-csvCaptureFrames` method used here.

## Human movement priority — 2026-09-08

Replaced the primitive player with a skeletal mannequin, authored locomotion and foot IK, native crouch/hand IK, Chaos ragdoll, and mass-limited physics pickup. Added two physics exercise crates near the start. Editor build, host checks, and Unreal physics/pose automation pass; the standalone Linux package has been rebuilt and launched with Vulkan. See [human movement details, calculations and remaining animation gaps](HUMAN_MOVEMENT.md). Earlier performance measurements above predate this character change.
