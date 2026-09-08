# NPC System

## Goals

NPCs should make Bengaluru feel occupied and reactive without simulating every citizen at full fidelity.

## Simulation LODs

### LOD 0 — Full actor
Near player. Full animation, perception, collision, interaction, combat.

### LOD 1 — Lightweight actor
Nearby but not important. Reduced perception and animation updates.

### LOD 2 — Crowd proxy
Farther away. Simplified path following and no detailed interactions.

### LOD 3 — Statistical simulation
Outside active region. Store only schedule/state; no spawned actor.

## Core NPC data

```text
identity_id
archetype
age_band
wardrobe_set
voice_set
home_zone
work_zone
schedule_profile
risk_tolerance
aggression
fear
patience
vehicle_preference
phone_use_frequency
crime_reporting_probability
```

## State machine

- Idle
- Walk
- Commute
- Work
- Shop
- Socialize
- Eat
- Wait
- Use phone
- Observe incident
- Flee
- Call authorities
- Fight
- Injured
- Enter vehicle
- Drive
- Return to schedule

## Important reactions

NPCs should react differently to:

- Horns
- Near-miss vehicles
- Crashes
- Rain
- Gunfire
- Explosions
- Police presence
- Player aiming a weapon
- Traffic jams
- Nighttime
