# Phase 0 foundation status

Updated 2026-09-08.

## Completed in the repository

- Unreal C++ project and runtime/editor targets scaffolded.
- Source, configuration, content, test, and automation folders created.
- Git repository initialized on `main`; ignore rules, Git LFS configuration, and initial snapshot established.
- C++, asset naming, branching, and review conventions documented.
- Engine-independent repository validator added.
- Incremental external backup script and daily user timer installer added.
- Deterministic Unreal Python smoke-map generator added.
- Host baseline recorded; active target revised to 720p/30 and a 120 × 120 m Phase 1A block.
- Hosted repository-validation CI and self-hosted Unreal compile CI defined.

## Machine-dependent work remaining

- Provision internal-SSD space, install an Epic precompiled UE5 build, and run `Scripts/configure_engine.py` to set `UE_ROOT` and pin its actual version.
- Hardware prerequisite completed: user-local vulkaninfo installed and Intel RPL-P / Mesa 23.2.1 hardware Vulkan enumerated outside the sandbox. Actual Unreal renderer launch remains open.
- Generate and launch `L_SmokeTest` in Unreal Editor.
- Compile both Editor and Game targets.
- Package and launch a Development build.
- Configure `NAMMA_BACKUP_ROOT`, run one backup, and enable the daily timer.
- Register a current self-hosted GitHub Actions runner labeled `linux` and `unreal` if compile CI is desired. The workflow's Node 24-based actions require a recent runner.

## Exit gate

Phase 0 is not complete until a clean checkout builds, launches, packages, and is version controlled. The repository-owned pieces are in place; engine, renderer, backup destination, and CI-runner verification remain open.

## Phase 1A source progress

The [first-delivery sandbox source and generators](../phase-1/STATUS.md) are now present. This does not close the Phase 0 build/launch/package gate. No engine version, binary map, playable capture or performance result has been verified.
