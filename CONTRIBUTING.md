# Contributing to Namma City

## Working agreement

- Keep `main` playable and use `feature/<id>-<name>` or `fix/<id>-<name>` branches.
- Put reusable and performance-sensitive systems in C++.
- Use Blueprints for composition, tuning, mission scripting, and rapid iteration.
- Prefer explicit events over direct coupling between gameplay systems.
- Put tunable vehicle, weapon, NPC, item, mission, shop, and district data in Data Assets or Data Tables.
- Avoid costly per-frame work when an event, timer, or reduced-frequency update is sufficient.
- Add a log category and visual debugging for every substantial gameplay system.

## Naming

Use Unreal's C++ conventions. Asset prefixes are mandatory:

| Asset | Prefix |
|---|---|
| Blueprint actor | `BP_` |
| Widget Blueprint | `WBP_` |
| Data Asset | `DA_` |
| Data Table | `DT_` |
| Material | `M_` |
| Material Instance | `MI_` |
| Texture | `T_` |
| Static Mesh | `SM_` |
| Skeletal Mesh | `SK_` |
| Animation | `A_` |
| Sound | `S_` |
| Niagara System | `NS_` |
| Level | `L_` |

## Before review

1. Run `python3 Scripts/validate_repository.py`.
2. Compile the Editor target with `Scripts/build_editor.sh`.
3. Launch `L_SmokeTest` and confirm there are no errors.
4. Include reproducible test steps in the change description.
