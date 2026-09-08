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
- 1080p/60 performance target and host baseline recorded.
- Hosted repository-validation CI and self-hosted Unreal compile CI defined.

## Machine-dependent work remaining

- Install/configure a compatible Unreal Engine 5 build and set `UE_ROOT`.
- Install Vulkan tools and verify the active GPU/driver.
- Generate and launch `L_SmokeTest` in Unreal Editor.
- Compile both Editor and Game targets.
- Package and launch a Development build.
- Configure `NAMMA_BACKUP_ROOT`, run one backup, and enable the daily timer.
- Register a current self-hosted GitHub Actions runner labeled `linux` and `unreal` if compile CI is desired. The workflow's Node 24-based actions require a recent runner.

## Exit gate

Phase 0 is not complete until a clean checkout builds, launches, packages, and is version controlled. The repository-owned pieces are in place; engine, renderer, backup destination, and CI-runner verification remain open.
