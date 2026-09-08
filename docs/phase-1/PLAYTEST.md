# First Delivery acceptance run

Status: **not executed in Unreal**. Use the pinned installed engine and actual desktop GPU.

## Build and launch

- Run host checks, compile Editor, generate smoke map and sandbox, compile Game, package Linux Development.
- Launch `Scripts/launch_game.sh` with the editor closed. Confirm 1280 × 720, low settings and Intel hardware Vulkan (not llvmpipe/lavapipe software rendering) in the log.
- Confirm the smoke map still opens separately. Run the `NammaCity.Delivery.Rules` automation test.

## Movement and interaction

- Spawn behind the character, facing down Safehouse Lane. Try diagonal movement, turn the camera, sprint/release, jump/land, ramp and sidewalk transitions.
- Back the camera into building walls and trees; it should shorten its boom rather than pass through solid geometry.
- Approach Namma Tea, aim the crosshair at its counter within 2.6 m, press E once; the parcel appears and the objective changes. E repeatedly must not duplicate it.
- Try E before pickup at Corner Stores, from too far away, and through a wall; none should advance the delivery.
- Follow the markers via Market Court to Corner Stores; E completes the delivery once. R returns to the start with the initial objective and no parcel.
- Pause while sprinting/jumping, release the buttons, resume; movement must not remain stuck. R from pause resets a partial delivery; Escape resumes; Q exits only while paused.
- In editor, move the pawn below Z=-500 cm or outside ±6200 cm: it returns to its starting transform with movement stopped. Repeat while carrying and verify the delivery remains possible.
- Restart and finish repeatedly over ten minutes; record crashes, soft locks, camera clipping and input issues.

## Capture performance and visuals

1. Use `Scripts/profile_game.sh`. It uncaps FPS for measuring the 33.3 ms budget independently of the shipping 30 fps limiter.
2. Warm up for at least 30 seconds, then walk/jump/sprint the complete route repeatedly for ten measured minutes.
3. Open the console and run `csvprofile stop` before exiting. CSV output is under the packaged game's `Saved/Profiling/CSV`; traces are local profiling artifacts.
4. Run `python3 Scripts/summarize_frame_times.py /path/to/capture.csv --column FrameTime`. If the pinned engine uses a different header, supply its actual millisecond frame-time column. Do not substitute a GPU-only column.
5. Record peak RSS from the running game's `/proc/<pid>/status` (`VmHWM`), memory pressure/swap change, game/render/GPU thread timings from Insights, and actor counts using engine diagnostics. Report missing telemetry explicitly.
6. Pass requires p95 ≤33.3 ms for ≥600 measured seconds, stable gameplay, no crash/soft lock, and hardware Vulkan rendering. The 3 GiB RSS value is an initial optimization envelope.
7. Capture a normal 720p window screenshot at spawn and Market Court, plus a short route recording. Link actual captures from STATUS.md; keep large recordings/traces outside Git.

If the GPU cannot launch the simple scene, retain source work and record the exact driver/error. A successful NullRHI generation or host test does not satisfy the playability gate.
