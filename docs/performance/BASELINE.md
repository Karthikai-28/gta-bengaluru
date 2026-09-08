# Phase 0 performance baseline

Captured on 2026-09-08 before Unreal Engine installation.

## Development target

- Resolution: 1920 × 1080
- Target: 60 fps
- Frame budget: 16.67 ms
- Initial priority threads: traffic AI, crowds, physics, missions, animation, and streaming

## Host snapshot

| Resource | Observed | Project recommendation | Status |
|---|---:|---:|---|
| OS | Ubuntu 22.04.5 LTS | Ubuntu 22.04 LTS | Ready |
| Logical CPU threads | 16 | 12 or more | Ready |
| Memory | 15 GiB | 32 GiB minimum | Below target |
| Primary storage | 477 GB NVMe SSD | NVMe SSD | Ready |
| Clang | 14.0.0 | Engine-compatible Clang | Verify with selected UE version |
| CMake | 3.22.1 | Installed | Ready |
| Ninja | 1.10.1 | Installed | Ready |
| Git LFS | 3.0.2 | Installed | Ready |
| Vulkan tools | Not installed | Required for renderer check | Blocked |
| Unreal Engine | Not found | UE5 source/binary install | Blocked |

## First in-engine capture

After the engine and smoke map are available, capture a Development build at 1080p with:

- frame time and FPS;
- game, render, and GPU thread time;
- resident memory;
- actor count;
- traffic and crowd counts.

Store the initial Unreal Insights trace under `Saved/Profiling/` locally and record summarized numbers here. Generated traces stay out of Git.
