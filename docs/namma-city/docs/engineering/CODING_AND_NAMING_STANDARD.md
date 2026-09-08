# Coding and Naming Standard

## C++

- Prefer clear ownership and RAII.
- Keep UObject lifetime rules explicit.
- Use Unreal reflection only where needed.
- Avoid expensive work in Tick when event-driven logic works.
- Keep gameplay tuning out of hard-coded constants.
- Add logging categories per major system.

## Unreal assets

Suggested prefixes:

```text
BP_ Blueprint actor
WBP_ Widget Blueprint
DA_ Data Asset
DT_ Data Table
M_ Material
MI_ Material Instance
T_ Texture
SM_ Static Mesh
SK_ Skeletal Mesh
A_ Animation
S_ Sound
NS_ Niagara System
L_ Level
```

## Branching

- `main`: always playable
- `develop`: integration if needed
- `feature/<id>-<name>`: feature work
- `fix/<id>-<name>`: fixes

Prefer small merges with reproducible test steps.
