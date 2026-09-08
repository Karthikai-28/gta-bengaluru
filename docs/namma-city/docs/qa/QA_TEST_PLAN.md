# QA Test Plan

## Smoke suite

- Boot to menu
- Start new game
- Spawn in district
- Walk/run/jump
- Enter/exit each vehicle
- Save/load
- Complete one mission
- Trigger police response
- Escape pursuit
- Quit cleanly

## Traffic tests

- 30-minute soak
- Blocked intersection recovery
- Disabled vehicle obstruction
- Dense motorcycle traffic
- Bus stop behavior
- Emergency vehicle interaction

## NPC tests

- Crowd spawn/despawn
- Rain transition
- Gunfire reaction
- Vehicle near miss
- Witness reporting
- Nav obstruction

## Mission tests

For every objective:

- Normal completion
- Player death
- Target death
- Vehicle destroyed
- Leaving mission area
- Save/load during allowed checkpoint
- Retry after failure

## World tests

- No fall-through holes
- Correct collision
- No floating props
- No blocked critical paths
- Streaming traversal at maximum vehicle speed
