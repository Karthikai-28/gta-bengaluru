# Performance Budgets

Set budgets early; revise after profiling.

## Initial PC target

Active target: **1280 × 720 / 30 fps** on Ubuntu 22.04, i7-1360P, 15 GiB usable RAM and Intel integrated graphics. First scene: 120 × 120 m; no population simulation. 1080p/60 is a future stretch target, not the current gate.

Use the [authoritative workstation baseline](../../../performance/BASELINE.md) and [Phase 1A capture procedure](../../../phase-1/PLAYTEST.md). Target uncapped capture p95 ≤33.3 ms for ten measured minutes after warm-up; shipped play defaults to a 30 fps cap. Initial packaged RSS envelope: 3 GiB, subject to measurement.

Low scalability; SM5 Vulkan; no Lumen, Nanite, virtual shadow maps, mesh distance fields or expensive post-processing. Use fixed daylight, shared materials and instanced props. All performance remains unmeasured until a hardware-rendered package runs.

## CPU priorities

- Traffic AI
- Crowd AI
- Physics
- Mission scripts
- Animation
- Streaming

## Optimization strategy

- Simulation LODs for NPCs
- Traffic actor pooling
- Distance-based tick reduction
- World Partition
- HLOD
- Occlusion
- Shared materials
- Instancing
- Async asset loading
- Avoid per-frame Blueprint loops over large actor arrays

## Later full-district benchmark route

Create a deterministic route through:

1. Dense market
2. Large junction
3. Park
4. High-speed road
5. Rain
6. Police chase
7. Combat encounter

Record frame time, game thread, render thread, GPU time, memory, actor count, traffic count, and crowd count.
