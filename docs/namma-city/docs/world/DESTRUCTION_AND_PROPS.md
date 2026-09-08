# Destruction and World Props

## Destruction tiers

### Static
Buildings, major bridges, terrain, critical infrastructure.

### Reactive
Doors, windows, signs, fences, lamps, small barriers.

### Destructible
Crates, stalls, selected walls, breakable props, construction objects.

## Rules

- Destruction exists for gameplay readability, not universal simulation.
- Never allow destroyed mission-critical geometry to soft-lock a mission.
- Persist only meaningful destruction; reset disposable props when streamed out if necessary.
- Pool commonly destroyed objects and effects.
