# Vehicle Physics Conventions — Unreal Engine / Chaos

## Units

- Linear dimensions in source documents: **mm**; Unreal transforms: **cm**.
- Mass: **kg**.
- Speed: **km/h** in design docs, converted internally as required.
- Power: **kW**.
- Torque: **Nm**.
- Angles: **degrees**.

## Fidelity tiers

| Tier | Vehicles | Simulation |
|---|---|---|
| Tier A | Player / mission hero | Full drivetrain, suspension, tyres, damage, audio, doors, interior |
| Tier B | Nearby AI | Full wheel physics, simplified powertrain/damage |
| Tier C | Mid-distance AI | Lower-frequency physics and simplified collisions |
| Tier D | Distant traffic | Kinematic/lane simulation until promoted |

## Required handling tests

- launch and acceleration
- sustained maximum speed
- 100–0 braking (or class-relevant braking speed)
- constant-radius skidpad
- lane change
- slalom
- kerb strike
- pothole strike
- speed-breaker at multiple speeds
- dry/wet asphalt
- gravel/dirt for applicable vehicles
- rollover threshold for high-CG vehicles
- collision at low/medium/severe energy
- wheel loss and disabled-state recovery

## Bengaluru-specific test tracks

Create synthetic test scenes representing:

- narrow residential roads
- dense market roads
- flyovers
- multilane ring-road traffic
- steep parking ramps
- potholes and repaired asphalt seams
- speed breakers
- flooded lane sections
- metro-pillar corridors
- construction detours
