# Phase-Wise Development Plan

## Phase 0 — Foundation

- Install toolchain on Ubuntu 22.04
- Create Git repository and LFS rules
- Create Unreal C++ project
- Establish naming conventions
- Create coding standard
- Create folder structure
- Configure automated backup
- Create smoke-test map
- Establish performance baseline
- Build a minimal CI compile job

**Exit gate:** clean project builds, launches, packages, and is version controlled.

## Phase 1 — Player sandbox

- Third-person controller
- Camera system
- Walk/jog/sprint
- Jump/mantle/vault
- Basic interaction component
- Health/damage/death
- Basic inventory
- Pickup system
- Debug HUD
- Keyboard/mouse controls
- Gamepad controls

**Exit gate:** enjoyable movement in a greybox map.

## Phase 2 — Vehicle sandbox

- Car physics
- Motorcycle prototype
- Auto-rickshaw prototype
- Enter/exit vehicle
- Vehicle camera
- Handbrake
- Lights/horn
- Vehicle health
- Basic damage
- Vehicle reset/recovery
- Handling data assets

**Exit gate:** three distinct vehicles are fun to drive.

## Phase 3 — World vertical slice

- Import one small Bengaluru road area
- Generate road meshes
- Sidewalks
- Intersections
- Procedural buildings
- Hero landmarks
- Street furniture
- World Partition
- HLOD
- NavMesh
- Traffic graph
- Pedestrian graph

**Exit gate:** complete 2 × 2 km district streams reliably.

## Phase 4 — Population

- Pedestrian spawning
- NPC archetypes
- Basic schedules
- Crowd locomotion
- Reactions to vehicles
- Reactions to danger
- Conversation barks
- Traffic spawning
- Lane following
- Intersection rules
- Overtake/lane change
- Motorcycle filtering behavior

**Exit gate:** city feels alive without missions.

## Phase 5 — Combat

- Weapon framework
- Aim system
- Hitscan/projectile framework
- Recoil/spread
- Reloading
- Ammo inventory
- Melee
- Cover prototype
- Explosive damage framework
- Combat NPC behavior
- Weapon wheel

**Exit gate:** one polished combat encounter.

## Phase 6 — Police and consequences

- Witness perception
- Crime event system
- Dispatch
- Search/investigation
- Chase
- Heat levels
- Roadblocks
- Arrest/death handling
- Heat cooldown
- Vehicle identification

**Exit gate:** complete crime → pursuit → escape loop.

## Phase 7 — Mission engine

- Objective framework
- Trigger volumes
- Mission states
- Checkpoints
- Failure/retry
- Dialogue hooks
- Rewards
- Mission markers
- Cutscene hooks
- 3–5 vertical-slice missions

## Phase 8 — Presentation

- Main menu
- Pause menu
- Phone UI
- Map/minimap
- Subtitles
- Settings
- Graphics presets
- Audio sliders
- Key rebinding
- Accessibility options
- Save/load UI

## Phase 9 — Atmosphere

- Day/night
- Rain
- Wet roads
- Traffic density by time
- Ambient audio
- Radio prototype
- Local signage
- Kannada/English environmental text where appropriate
- Dynamic news/world reactions

## Phase 10 — Vertical-slice polish

- Performance pass
- LOD/HLOD pass
- Collision audit
- Animation polish
- Mission polish
- Audio polish
- Bug fixing
- Playtest sessions
- Packaging
- Demo build

## Phase 11 — Scale-up

Only after the vertical slice is genuinely fun:

- Add districts
- Expand story
- Add vehicles
- Add interiors
- Add side activities
- Improve procedural generation
- Add advanced crowd simulation
- Add business/property systems
- Add more weather
- Add cinematic story missions
