# Namma City — Game Development Documentation

An original open-world action game inspired by Bengaluru's culture, streets, traffic, technology, neighborhoods, weather, and everyday urban life.

> Goal: build a polished, scalable open-world game without copying GTA assets, names, characters, missions, UI, branding, or proprietary systems.

## Current implementation

[Phase 1A: First Delivery](../../phase-1/STATUS.md) is the active source milestone: a 120 × 120 m stylized block targeting 720p/30 on the existing machine. Engine compile, launch and profiling remain unverified. The broader systems below describe future design intent.

## Development principle

Do **not** start by building all of Bengaluru. Build a highly polished vertical slice first: one compact district with walking, driving, traffic, NPCs, combat, police response, missions, weather, UI, save/load, and a small story arc.

## Repository map

- [Pipeline](pipeline/PIPELINE.md)
- [Software stack](software/SOFTWARE_STACK.md)
- [Ubuntu setup](software/UBUNTU_22_04_SETUP.md)
- [Backlog](planning/BACKLOG.md)
- [Phase plan](planning/PHASE_PLAN.md)
- [Milestones and gates](planning/MILESTONES.md)
- [World overview](world/WORLD_OVERVIEW.md)
- [Bengaluru areas](world/AREAS_AND_LANDMARKS.md)
- [World generation](world/WORLD_GENERATION.md)
- [NPC system](npcs/NPC_SYSTEM.md)
- [NPC archetypes](npcs/NPC_ARCHETYPES.md)
- [Vehicles](vehicles/VEHICLE_SYSTEM.md)
- [Vehicle catalogue](vehicles/VEHICLE_CATALOGUE.md)
- [Player controller](player/PLAYER_CONTROLLER.md)
- [Combat](combat/COMBAT_SYSTEM.md)
- [Weapons](combat/WEAPONS_AND_AMMO.md)
- [Police / heat](systems/POLICE_AND_HEAT.md)
- [Traffic](systems/TRAFFIC_SYSTEM.md)
- [Mission system](missions/MISSION_SYSTEM.md)
- [Story](story/STORY_BIBLE.md)
- [Economy](systems/ECONOMY_AND_PROGRESSION.md)
- [Phone / UI](ui/PHONE_AND_UI.md)
- [Audio](audio/AUDIO_AND_RADIO.md)
- [Animation](animation/ANIMATION.md)
- [Assets](art/ASSET_PIPELINE.md)
- [Save system](systems/SAVE_SYSTEM.md)
- [Networking future notes](systems/MULTIPLAYER_FUTURE.md)
- [Performance](engineering/PERFORMANCE_BUDGETS.md)
- [Project architecture](engineering/PROJECT_ARCHITECTURE.md)
- [Testing](qa/QA_TEST_PLAN.md)
- [Legal/licensing](legal/DATA_AND_LICENSING.md)
- [Risk register](planning/RISK_REGISTER.md)

- [Game design principles](design/GAME_DESIGN_PRINCIPLES.md)
- [Camera](camera/CAMERA_SYSTEM.md)
- [Interiors](world/INTERIORS.md)
- [Destruction and props](world/DESTRUCTION_AND_PROPS.md)
- [VFX](vfx/VFX.md)
- [Coding/naming standard](engineering/CODING_AND_NAMING_STANDARD.md)
- [CI and builds](engineering/CI_AND_BUILDS.md)
- [Debug and telemetry](engineering/DEBUG_AND_TELEMETRY.md)
- [Third-party license register](legal/THIRD_PARTY_LICENSES.md)

## Prototype target

**Namma City 0.1 Vertical Slice**

- Approx. 2 × 2 km Bengaluru-inspired district
- 1 playable character
- Third-person movement
- 1 motorcycle, 1 car, 1 auto-rickshaw
- 30–50 traffic vehicles active around the player
- 50–100 lightweight pedestrian agents nearby
- Basic melee + 2 firearm classes
- 3-level heat system
- 3–5 missions
- Day/night cycle
- Rain
- Map + phone
- Garage
- Save/load
- One polished chase sequence
- Stable target frame rate on the development machine

## Recommended engine

Unreal Engine 5 using a hybrid of **C++ for reusable systems** and **Blueprints for mission logic, tuning, and rapid iteration**.
