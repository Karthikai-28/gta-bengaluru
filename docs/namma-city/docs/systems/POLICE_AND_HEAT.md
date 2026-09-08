# Police and Heat System

## Core philosophy

Authorities should not automatically know the player's exact position. Crimes produce information that propagates through witnesses, cameras, patrols, and dispatch.

## Heat model

| Heat | Response |
|---:|---|
| 0 | Normal world |
| 1 | Investigation / nearby patrol |
| 2 | Active vehicle and foot pursuit |
| 3 | Coordinated search, roadblocks, stronger units |
| 4 later | Tactical escalation |
| 5 later | City-wide manhunt |

## Crime event

```text
crime_type
severity
location
time
suspect_description
vehicle_description
witness_ids
camera_ids
```

## Flow

Crime → witness/camera detection → report → dispatch → last-known-position search → pursuit → line-of-sight break → search phase → cooldown/escape.

## Escape mechanics

- Break line of sight
- Leave search radius
- Change vehicle
- Hide in valid spaces
- Avoid new witnesses
- Change appearance later

## Police AI roles

- Patrol officer
- Pursuit driver
- Roadblock unit
- Search unit
- Tactical unit later
