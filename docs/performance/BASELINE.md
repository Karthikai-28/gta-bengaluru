# Workstation baseline and active constraints

Updated 2026-09-08. This is the development and first-playable target machine; an upgrade is not assumed.

| Resource | Observed | Consequence |
|---|---|---|
| OS | Ubuntu 22.04.5 LTS, kernel 6.8 | Native Linux Development package |
| CPU | Intel Core i7-1360P, 12 cores / 16 logical threads | Two parallel compile actions initially |
| Memory | 15 GiB usable, 15 GiB swap | Small scene; close heavy applications manually before editor/profiling |
| Memory pressure at inspection | About 11 GiB used, 2.6 GiB available, 8.1 GiB swap used | Snapshot only; remeasure before running Unreal |
| GPU | PCI Intel integrated graphics, device a7a0; no discrete GPU observed | Intel RPL-P hardware Vulkan 1.3.255 enumerated; Unreal compatibility unverified |
| Storage | NVMe; project filesystem 336 GiB, 45.76 GiB available after authorized cache cleanup | Installed engine cannot be provisioned until capacity is checked |
| Engine | No configured or discovered installation | Compile, cook, package, launch blocked |
| Graphics tools/session | User-local vulkaninfo installed; host /dev/dri and :0 accessible outside sandbox | Mesa 23.2.1, accelerated OpenGL 4.6; host Vulkan enumeration passed |

## Initial rendering and content budget

- Packaged window: **1280 × 720, capped at 30 fps**; 1080p/60 is a later stretch target.
- Profiling gate: **95th percentile frame time ≤33.3 ms** after warm-up, for ten measured minutes. Capture uncapped to exclude the 30 fps limiter's intentional 33.333 ms wait from this gate.
- One **120 × 120 m** map, one player, six building shells, simple props, no simulated population.
- SM5 Vulkan, low scalability, 256 MiB texture streaming pool starting budget. This pool is not total GPU memory; Intel shares system memory.
- Lumen, Nanite, virtual shadow maps, mesh distance fields, SSR, volumetric fog, depth of field, motion blur, bloom and auto exposure disabled.
- Fixed daylight, shared simple materials, instanced repeated props. No baked-lighting dependency in the generated first scene.
- Desired packaged process memory envelope: ≤3 GiB RSS; measure before treating it as achieved. Desktop + editor + build processes must fit available RAM without sustained swap growth.
- Keep 40 GiB working headroom beyond the installed engine as an initial project/toolchain/cache planning reserve; revise from the selected distribution and measured cache growth. This is not an Epic minimum.

## Gates and evidence

Run `python3 Scripts/check_workstation.py --engine-root /path/to/engine --desktop` from the actual desktop session. Run `Scripts/profile_game.sh`, follow [the playtest](../phase-1/PLAYTEST.md), and record the selected engine version, driver, resolution, frame-time distribution, peak RSS, actor/instance counts, and route outcome here.

| Measurement | Result |
|---|---|
| Editor / Game compile | Not run — no engine |
| Hardware renderer startup | Vulkan enumeration passed on host; actual Unreal renderer not run |
| Packaged Development launch | Not run — no engine/package |
| 10-minute route / p95 frame time | Not measured |
| Peak process RSS / GPU memory | Not measured |

Do not expand to the 2 × 2 km district, traffic, crowds, weather, MetaHumans, or streaming systems until the small scene passes. Reduce draw cost and geometry before adding scope.

## Host prerequisite follow-up — 2026-09-08

Authorized removal of the reviewed archive caches plus pip/Chrome caches reclaimed about 24 GiB, leaving 45.76 GiB free. Epic Linux downloads require account sign-in and no browser is connected to this agent, so no engine was downloaded or installed.

Ubuntu's vulkan-tools 1.3.204 package was downloaded and its diagnostic installed at `~/.local/bin/vulkaninfo` without root. A host run enumerated Intel Graphics (RPL-P), Vulkan API 1.3.255, driver Mesa 23.2.1-1ubuntu3.1~22.04.4. llvmpipe also appeared as a separate software device. The loader skipped one other ICD with a warning; Intel hardware enumeration succeeded. This does not establish Unreal renderer compatibility or performance.
