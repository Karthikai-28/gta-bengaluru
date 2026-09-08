# Vehicle System

## Principles

- Arcade-realistic handling
- Easy to learn, hard to master
- Motorcycles receive first-class treatment
- Traffic vehicles and player vehicles share the same base data model where possible
- Use fictional manufacturers and model names

## Vehicle data asset

```text
vehicle_id
class
mass
wheelbase
engine_power
torque_curve
gear_ratios
brake_force
handbrake_force
steering_curve
traction
center_of_mass
aero_drag
damage_resistance
fuel_or_energy_model(optional)
traffic_behavior_profile
```

## Player functionality

- Enter/exit
- Camera modes
- Horn
- Headlights
- Handbrake
- Reverse
- Vehicle weapon support only if a mission/game system needs it
- Passenger seats later
- Vehicle radio
- Damage feedback
- Recovery/reset

## Damage components

- Body health
- Engine health
- Individual wheels
- Doors
- Glass
- Lights
- Cosmetic deformation where feasible

## Motorcycle-specific systems

- Rider lean
- Low-speed stability assist
- Wheel slip
- Crash/ejection
- Lane filtering
- Helmet support
- Different handling on wet roads
