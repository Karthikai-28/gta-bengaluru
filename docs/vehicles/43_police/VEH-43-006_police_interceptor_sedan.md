# VEH-43-006 — Police Interceptor Sedan

## 1. Document Status

| Field | Value |
|---|---|
| Project | Namma City / GTA Bengaluru working design |
| Category | Police and Law Enforcement Vehicles |
| Vehicle ID | `VEH-43-006` |
| Reference name | Police Interceptor Sedan |
| Vehicle archetype | Vehicle |
| Specification status | **Prototype game-design baseline** |
| OEM accuracy | Approximate envelope; verify manufacturer/year/variant before any factual publication |
| Branding status | Reference-only; use fictional brand/model in commercial build unless licensed |

## 2. Design Intent

Police Interceptor Sedan represents the **police and law enforcement vehicles** class in Bengaluru traffic. The vehicle must be recognizable by silhouette, proportions, stance, lighting signature and motion, while the shipped game should use an original visual identity unless the real brand is licensed. It should support believable AI traffic, player use where applicable, damage, audio, customization and scalable LODs.

### Bengaluru usage

- **Primary zones:** dynamically weighted by district, road class and time of day.
- **Traffic role:** ambient traffic, player acquisition, mission support, parked-world population and faction/service use where appropriate.
- **Spawn rarity:** tune from Common to Legendary by trim and district.
- **Weather behavior:** tyre grip, visibility and braking distance reduce during Bengaluru monsoon conditions.
- **AI behavior:** derived from vehicle mass, acceleration, width, turning circle and service role rather than a single generic driving profile.

## 3. Exterior Design Package

### Required model views

- Front orthographic
- Rear orthographic
- Left/right profile
- Top and underbody plan
- 3/4 front hero view
- 3/4 rear hero view
- Wheel/tyre detail
- Lighting detail
- Cabin/interior reference where enterable
- Engine/battery bay reference where damageable

### Primary visual features

- Preserve category-correct wheelbase-to-body ratio and track width.
- Use physically plausible panel gaps, glass thickness, tyre sidewall, mirrors, lamps and underbody volume.
- All player-driveable road vehicles require working doors/closures where relevant, steering animation, suspension travel, brake-light logic, indicators, headlamps, reverse lamps and damage states.
- Service/commercial variants require role-specific decals and equipment using **fictional branding**.

### In-game color palette

| Paint | Hex target | Usage |
|---|---|---|
| Bengaluru Blue | `#2E5D88` | Factory-style body color / fleet variation |
| Sand Beige | `#B8A98D` | Factory-style body color / fleet variation |
| Graphite Black | `#1E1F22` | Factory-style body color / fleet variation |
| Deep Red | `#7C1F28` | Factory-style body color / fleet variation |
| Taxi Yellow | `#E4B429` | Factory-style body color / fleet variation |
| Steel Silver | `#A7A9AC` | Factory-style body color / fleet variation |

Paint uses a metallic/solid clear-coat material family with independent dirt, rain, scratch, dent and dust masks. Commercial/service variants may override the palette.

## 4. Physical Specification Envelope

> Values below are **simulation targets** for initial Chaos physics setup, not certified factory specifications.

| Parameter | Target |
|---|---:|
| Overall length | 4365 mm |
| Overall width | 1790 mm |
| Overall height | 1639 mm |
| Wheelbase | 2582 mm |
| Ground clearance | 182 mm |
| Kerb / operating mass | 1473 kg |
| Nominal payload | 368 kg |
| Passenger capacity | 5 |
| Approx. center-of-gravity height | 623 mm |
| Nominal turning radius | 5.2 m |
| Target drag coefficient / equivalent | 0.34 |

## 5. Powertrain

| Parameter | Target |
|---|---:|
| Energy source | Petrol/Diesel/EV |
| Displacement | 1460 cc |
| Peak power target | 117 kW |
| Peak torque target | 243 Nm |
| Driven wheels / propulsion | FWD/AWD |
| Transmission | 6AT/DCT |
| Fuel/battery capacity envelope | 50–450 L |
| Driving range envelope | 500–1,200 km |
| Thermal model | simplified temperature/overheat state for sustained abuse; detailed only on hero/player vehicles |

### Power delivery tuning

- Torque curve must match class identity: commuter vehicles progressive; EVs immediate; diesels strong low-RPM; performance vehicles high-output and traction-limited.
- Add drivetrain inertia and shift interruption rather than instant velocity changes.
- Use per-gear torque multiplication for manual/automatic ICE drivetrains.
- EVs use motor torque map, inverter limit, battery state-of-charge and regenerative braking parameters.

## 6. Performance

| Parameter | Target |
|---|---:|
| Top speed | 197 km/h |
| 0–100 km/h | 8.6 s |
| Reverse speed | 15–25 km/h |
| Acceleration rating | 46/100 — Moderate |
| Braking rating | 80/100 — Very High |
| Mobility score | 70/100 — High |
| Stability score | 86/100 — Very High |
| Off-road capability | 35/100 — Moderate |

**Game rule:** real road speed limits and AI risk models are separate from physical top speed. NPCs normally use only a fraction of maximum capability.

## 7. Chassis, Rigidity and Structural Integrity

### Structure

Purpose-built vehicle structure.

### Rigidity target

- **Global structural integrity score:** **74/100 (High)**.
- Torsional stiffness is represented through suspension compliance and deformation thresholds rather than a full finite-element simulation.
- Chassis attachment points: front suspension, rear suspension, powertrain, wheels, doors/closures, bumpers, glass and cargo body must be independent damage groups where applicable.
- Passenger survival cell deforms less than external crush regions.

### Deformation behavior

- Cosmetic deformation level: **Moderate**.
- Low-speed impact: scratches, cracked lamps, bent trim, minor bumper/panel displacement.
- Medium impact: wheel alignment damage, door/closure jams, radiator/battery cooling degradation, glass breakage.
- Severe impact: suspension collapse, wheel detachment, propulsion failure, fuel/battery hazard state, cabin intrusion cap and immobilization.
- Rollover: roof/body crush appropriate to class, glass breakage, fluid/battery effects and recovery logic.

## 8. Suspension and Ride

- **Architecture:** road suspension with reinforced springs/dampers.
- Tune spring rate, damping, anti-roll stiffness and travel from mass and intended use.
- Player vehicle receives higher-frequency suspension simulation than distant AI traffic.
- Wet-road coefficient modifies tyre longitudinal and lateral friction.
- Kerb, pothole, speed-breaker and uneven-road response is important for Bengaluru authenticity.

### Initial Chaos suspension targets

| Parameter | Guideline |
|---|---|
| Ride character | Balanced road |
| Suspension travel | Medium |
| Anti-roll stiffness | Medium |
| Damping | speed-sensitive; prevent pogo/oscillation |
| Bottom-out | progressive bump-stop response |

## 9. Wheels, Tyres and Brakes

| Parameter | Target |
|---|---|
| Wheel/tyre package | 215/60 R17 reinforced |
| Brakes | four-wheel disc ABS |
| ABS | enabled where class/era supports it; can be absent on legacy vehicles |
| Traction control | class dependent |
| Stability control | class dependent |
| Puncture model | pressure loss → grip/drag change → rim damage |
| Detachment | severe suspension/wheel damage only |

### Tyre states

1. Normal/dry
2. Wet
3. Low pressure
4. Punctured
5. Blown tyre
6. Rim-only / wheel destroyed where supported

## 10. Steering and Handling

- **Handling score:** **76/100 (High)**.
- Speed-sensitive steering: reduce steering angle and response at high speed.
- Ackermann/steering geometry approximated per class.
- Understeer/oversteer bias comes from drivetrain, CG, wheelbase and tyre balance.
- High-CG vehicles carry explicit rollover risk.
- Two-wheelers use lean-angle, counter-steer and rider-body-assist models.
- Heavy vehicles have delayed yaw response and long braking distances.

## 11. Damage and Health Model

| Subsystem | Base health / behavior |
|---|---|
| Whole-vehicle health | 888 HP simulation baseline |
| Engine | torque reduction → rough/limp mode → shutdown |
| Cooling | leak/overheat progression |
| Transmission | missed/slow shifts → gear loss → lockout |
| Steering | alignment pull → reduced steering range |
| Front suspension | camber/toe error → collapse/detachment |
| Rear suspension | instability → collapse/detachment |
| Wheels/tyres | puncture → blowout → detachment |
| Brakes | increased stopping distance / asymmetric braking |
| Lighting | per-lamp breakage |
| Glass | crack → shatter |
| Fuel/battery | leak, electrical fault or thermal-event state; avoid instant Hollywood explosion by default |
| Doors/closures | dent, jam, detach |

### Collision materials

Use differentiated response for:

- sheet metal / plastic
- structural steel/aluminium/composite
- glass
- rubber tyres
- lamps
- cargo
- road furniture
- pedestrian/soft-body contacts
- water and vegetation

## 12. Mobility and Terrain

| Surface | Behavior |
|---|---|
| Dry asphalt | baseline grip |
| Wet asphalt | reduced peak friction; longer braking |
| Painted road/metal cover when wet | significantly lower motorcycle grip |
| Potholes | suspension impulse and potential wheel damage |
| Dirt | reduced road-tyre grip |
| Mud | increased rolling resistance; possible bogging |
| Grass | low/medium grip |
| Gravel | lateral slip and braking penalty |
| Standing water | drag + aquaplaning threshold |
| Kerbs | wheel/suspension impulse |
| Speed breakers | chassis/suspension impact based on speed and clearance |

## 13. Occupants, Cargo and Interaction

- Capacity: **5 occupant(s)** nominal.
- Seat sockets and entry/exit points are authored per vehicle.
- Player enter/exit animation uses obstruction checks.
- Cargo mass affects CG and acceleration for commercial vehicles.
- Doors, boot/tailgate, hood/bonnet and cargo doors are interactable where applicable.
- Seatbelt/helmet logic may influence ejection/ragdoll severity in simulation settings.

## 14. Audio Design

Required layers where applicable:

- idle / low / mid / high load propulsion
- intake/exhaust or electric motor/inverter whine
- gearshift and transmission clunk
- tyre roll by road surface
- suspension impacts
- brake squeal/ABS
- horn specific to vehicle class
- wind noise
- body creaks/rattles for old vehicles
- collision layers by material and severity
- rain interaction

## 15. Lighting and Electrical

- low/high beam
- DRL if appropriate
- brake lights
- reverse lights
- indicators/hazard lights
- cabin/instrument lighting
- emergency/service beacons for role variants
- damageable bulbs/LED modules
- battery/electrical-fault state

## 16. AI Traffic Profile

### Base driving personality

- Desired speed derived from road limit, traffic density and archetype.
- Gap acceptance scales inversely with vehicle size.
- Lane-change aggressiveness depends on service role and district.
- Two-wheelers/autos may filter through gaps where legal/gameplay rules permit.
- Heavy vehicles preserve wider turning radius and longer braking margin.
- Emergency vehicles receive siren-aware traffic yielding logic.

### Bengaluru behaviors to support

- dense mixed traffic
- frequent two-wheeler filtering
- autos stopping for passengers
- buses approaching stops
- delivery riders with destination urgency
- water tankers and trucks with high inertia
- sudden rain-driven traffic slowdown
- junction negotiation with imperfect lane discipline

## 17. Spawn and Economy

| Parameter | Design rule |
|---|---|
| Base availability | district/time/faction dependent |
| Purchase price | balance against performance, rarity and progression |
| Repair cost | proportional to class, part damage and luxury tier |
| Insurance/recovery | optional player progression system |
| Resale | condition, mileage and modifications influence value |
| Fuel/charge cost | economy sink; can be simplified by game mode |

## 18. Customization

Potential slots, where class permits:

- paint / wraps
- wheels / tyres
- suspension height
- brake package
- engine/motor tune
- transmission tune
- exhaust or EV audio profile
- bumpers/body kit
- lights
- mirrors
- roof/cargo accessories
- interior trim
- number plate style
- fleet/faction livery
- armour package for designated vehicles

Customization must not create impossible collision geometry without updating collision and wheel placement.

## 19. Unreal Engine / Chaos Vehicle Implementation

### Required assets

- skeletal or articulated vehicle mesh
- physics asset
- wheel meshes
- collision primitives / simplified convex hulls
- Chaos vehicle configuration
- material instances
- lights
- audio MetaSounds
- damage masks/morphs or modular damaged meshes
- interior/cockpit asset if required

### Physics variables to expose

```text
Mass
CenterOfMassOffset
DragCoefficient
DownforceCoefficient
EngineOrMotorTorqueCurve
MaxRPM / MotorRPM
TransmissionRatios
FinalDriveRatio
DifferentialType
WheelRadius
WheelWidth
WheelMass
CorneringStiffness
FrictionMultiplier
MaxSteerAngle
BrakeTorque
HandbrakeTorque
SpringRate
DampingRatio
SuspensionTravel
RollbarScaling
ABS/TCS/ESC toggles
```

## 20. Animation Requirements

- steering wheel/handlebar
- front wheel steering
- wheel rotation
- suspension compression/rebound
- gear selector / rider shift where visible
- rider/driver hands and feet
- door/hood/boot/tailgate
- mirrors where dynamic
- wipers
- fans/rotors/propellers where relevant
- damage detachment bones where supported

## 21. LOD and Performance Budget

| Level | Intended use |
|---|---|
| Hero / LOD0 | player vehicle, photo mode, very close NPC |
| LOD1 | close traffic |
| LOD2 | normal traffic |
| LOD3 | distant traffic |
| HLOD/impostor | very distant world population where applicable |

### Budget principles

- Interior complexity only when visible/enterable.
- Distant AI reduces suspension, damage and drivetrain simulation frequency.
- Share material families, tyres, glass, lights and generic interior parts where visually acceptable.
- Nanite suitability must be tested per moving/deforming vehicle asset; do not assume a static-world workflow.

## 22. Camera Requirements

Player-capable variants should support:

- third-person chase near
- third-person chase far
- cinematic low chase
- hood/bonnet or handlebar view
- first-person cockpit/rider view where implemented
- reverse camera
- photo mode
- mission cinematic sockets

Camera spring and FOV scale with acceleration and vehicle size.

## 23. VFX

- tyre smoke dependent on slip and surface
- dust and gravel
- wet tyre spray
- exhaust smoke/heat shimmer where appropriate
- EV electrical/thermal effects only when damaged
- sparks on metal contact
- glass shards
- debris by panel material
- fire/smoke for sustained critical damage
- water splash
- brake heat for performance vehicles where visible

## 24. Gameplay Balance Summary

| Attribute | Score |
|---|---:|
| Acceleration | 46/100 |
| Top-speed class | 49/100 |
| Braking | 80/100 |
| Handling | 76/100 |
| Stability | 86/100 |
| Mobility | 70/100 |
| Off-road | 35/100 |
| Structural integrity | 74/100 |
| Mass/inertia class | Very Low |

## 25. Acceptance Criteria

This vehicle is production-ready only when:

- [ ] silhouette and scale pass comparison review
- [ ] wheels contact ground correctly at rest and through suspension travel
- [ ] mass and center of gravity are validated
- [ ] acceleration and top speed fall within the intended game envelope
- [ ] braking distance is believable for class and weather
- [ ] turning circle works in Bengaluru street widths intended for this class
- [ ] no sustained high-speed physics instability
- [ ] AI can spawn, navigate, stop, park and despawn it safely
- [ ] collisions do not generate explosive impulses or tunnelling
- [ ] damage states are monotonic and recover/save correctly
- [ ] lights and indicators work
- [ ] audio has no RPM/load discontinuities
- [ ] wet-road handling has been tested
- [ ] LOD transitions are acceptable
- [ ] player enter/exit cannot clip through common obstacles
- [ ] localization and UI name are present
- [ ] final commercial name/branding is legally cleared

## 26. Open Data / Verification Tasks

Before final asset lock, verify for the chosen model year/variant:

- exact dimensions and wheelbase
- kerb and gross mass
- engine displacement or battery/motor specification
- power and torque curve
- gear ratios/final drive
- tyre sizes and wheel diameter
- brake package
- suspension type/travel
- turning radius
- top speed and acceleration
- production colors and trims
- interior/cargo/passenger packaging
- public-road legality and license/branding requirements

---

**Design principle:** use the real vehicle only as a reference point. The production vehicle should have an original brand, body details and interior unless an explicit licensing agreement permits the real design/trademark.
