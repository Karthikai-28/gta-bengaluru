# Unreal Project Architecture

## C++ modules/subsystems

Suggested high-level modules:

```text
NammaCore
NammaPlayer
NammaVehicles
NammaTraffic
NammaAI
NammaCombat
NammaPolice
NammaWorld
NammaMissions
NammaEconomy
NammaUI
NammaSave
NammaDebug
```

Do not create all modules on day one. Split when boundaries become real.

## Data-driven design

Prefer Primary Data Assets / Data Tables for:

- Vehicles
- Weapons
- NPC archetypes
- Items
- Missions
- Shops
- District metadata

## Event architecture

Use explicit gameplay events/delegates for:

- Crime committed
- NPC witnessed crime
- Mission objective completed
- Vehicle destroyed
- Player entered district
- Weather changed
- Time-of-day threshold

This reduces direct coupling between systems.

## Debug tools

Every complex system should have visual debugging:

- Traffic lanes
- NPC paths
- Police search radius
- Mission triggers
- Streaming cells
- Spawn volumes
- Interaction traces
