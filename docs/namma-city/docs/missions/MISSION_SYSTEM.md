# Mission System

## Design

Mission content should be data-driven and built on reusable objective types.

## Objective types

- Go to location
- Talk to NPC
- Follow NPC
- Enter vehicle
- Drive to location
- Chase target
- Lose pursuers
- Collect item
- Deliver item
- Defend area
- Escape area
- Search area
- Photograph target
- Interact with terminal/object

## Mission state

```text
NotStarted
Active
Checkpointed
Failed
Completed
Aborted
```

## Mission definition

```text
mission_id
title
prerequisites
start_trigger
objectives
checkpoints
fail_conditions
rewards
next_missions
story_flags
```

## Vertical-slice missions

### Mission 1 — Homecoming
Tutorial movement, garage, first drive.

### Mission 2 — Rush Hour
Motorcycle delivery that teaches traffic filtering and navigation.

### Mission 3 — Missing Signal
Investigate a missing friend/contact and introduce AstraGrid-style corporate mystery.

### Mission 4 — Under the Flyover
Vehicle pursuit transitions to foot chase.

### Mission 5 — Last Known Position
Introduces the full witness/police/search system.
