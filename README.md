# Namma City

Namma City is an original Bengaluru-inspired open-world action game. The current goal is a polished 2 × 2 km vertical slice before any city-scale expansion.

## Phase 0 quick start

1. Install Unreal Engine 5 outside this repository.
2. Copy `.env.example` to `.env` and set `UE_ROOT`.
3. Run `git lfs install` once on the workstation.
4. Run `python3 Scripts/validate_repository.py`.
5. Run `Scripts/create_smoke_test_map.sh` to generate the initial level.
6. Run `Scripts/build_editor.sh` and then `Scripts/launch_editor.sh`.

The complete design documentation starts at [docs/namma-city/docs/README.md](docs/namma-city/docs/README.md). Phase 0 status and machine-specific blockers are tracked in [docs/phase-0/STATUS.md](docs/phase-0/STATUS.md).
