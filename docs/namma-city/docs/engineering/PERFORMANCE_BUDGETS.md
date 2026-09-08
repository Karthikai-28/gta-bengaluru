# Performance Budgets

Set budgets early; revise after profiling.

## Initial PC target

Choose one development target such as 1080p/60 fps on your actual machine and measure everything against it.

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

## Benchmark route

Create a deterministic route through:

1. Dense market
2. Large junction
3. Park
4. High-speed road
5. Rain
6. Police chase
7. Combat encounter

Record frame time, game thread, render thread, GPU time, memory, actor count, traffic count, and crowd count.
