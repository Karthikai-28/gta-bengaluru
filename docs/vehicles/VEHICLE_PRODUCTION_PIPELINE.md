# Vehicle Production Pipeline

This is the required path from an entry in the vehicle design bible to a shippable Unreal Engine vehicle.

## V0 — Reference lock

- Choose the exact archetype and, where relevant, exact real-world reference model/year/variant.
- Collect legally usable orthographic, dimensional, mechanical, interior and audio references.
- Verify dimensions, mass, payload, powertrain, tyres, brakes, suspension, turning radius and packaging.
- Mark unknowns explicitly; never fill unknown factory data with guesses.
- Decide whether the production asset is fictional, generic or licensed.

**Gate:** reference/data review complete.

## V1 — Original production design

- Produce silhouette and proportion studies.
- Create a fictional manufacturer/model identity unless licensing permits a real one.
- Differentiate grille/lamps/body surfacing/interior details from unlicensed references.
- Define trim levels, paint palette, wheel families and service/fleet variants.

**Gate:** visual/design sign-off.

## V2 — Geometry blockout

- Model at correct scale.
- Lock overall dimensions, wheelbase, track, overhangs, ground clearance and seat positions.
- Establish steering pivots and suspension travel.
- Validate door/hood/tailgate sweep volumes.
- Import a blockout into Unreal Engine and validate against the character scale.

## V3 — Production modeling

- Exterior production mesh.
- Interior for enterable/hero vehicles.
- Underbody visible zones.
- Wheels, tyres, hubs, brakes and suspension components.
- Glass and lighting internals.
- Detachable/damageable panels.
- Cargo, municipal, emergency or mission equipment where applicable.

## V4 — Materials and appearance

- Shared paint/clear-coat material family.
- Glass, metal, rubber, plastic, fabric/leather and emissive materials.
- Dirt, dust, mud, rain, scratch, dent and burn masks.
- Fleet liveries and trim variants.
- Night lighting validation.

## V5 — Rigging and articulation

- root/chassis
- steering and wheel hubs
- suspension
- doors/hood/boot/tailgate
- wipers
- steering wheel/handlebar
- driver/rider/seat sockets
- damage-detachment bones
- special machinery joints/booms/tracks/rotors as applicable

## V6 — Chaos physics

- mass and inertia tensor
- center of mass
- wheel radius/width/mass
- tyre cornering stiffness and friction
- spring rate, travel and damping
- anti-roll behavior
- steering lock and speed sensitivity
- brake/handbrake torque
- engine or motor torque curve
- gear ratios/final drive/differential
- aerodynamic drag/downforce
- ABS/TCS/ESC as appropriate

**Gate:** dry/wet handling test suite passes.

## V7 — Damage and structural behavior

- cosmetic scratches/dents
- bumper/panel deformation
- glass cracking/shattering
- lamp failures
- punctures/blowouts/rim damage
- steering alignment faults
- suspension collapse/wheel detachment
- cooling/engine/motor/transmission faults
- fuel/battery critical states
- door/closure jam and detachment
- disabled state and recovery
- save/load persistence

## V8 — Audio and VFX

- propulsion load/RPM or motor/inverter layers
- gear/transmission sounds
- tyre road-noise by surface
- suspension impacts
- brake/ABS sounds
- horn/siren
- wind
- body rattles for older vehicles
- collision material layers
- dust, spray, tyre smoke, sparks, debris, critical smoke/fire

## V9 — AI and world integration

- road/lane eligibility
- speed profile
- following distance
- gap acceptance
- turn radius/parking-space requirements
- district and time spawn weights
- driver-archetype weighting
- service routes and stops
- emergency yielding behavior
- simulation promotion/demotion rules

## V10 — LOD and performance

- LOD0 hero
- LOD1 close traffic
- LOD2 normal traffic
- LOD3 distant traffic
- simplified collision
- shared materials/textures where practical
- simulation fidelity tiers
- CPU/GPU/memory budget review

## V11 — QA

Test at minimum:

- scale and wheel contact
- launch/acceleration
- top speed
- braking
- slalom and constant-radius handling
- high-speed lane change
- curb/pothole/speed-breaker impacts
- dry/wet road
- off-road surfaces where applicable
- rollover threshold
- low/medium/high-energy collision
- damage progression
- enter/exit obstruction
- save/load
- AI spawn/navigation/parking/despawn
- LOD transitions
- long traffic soak

## V12 — Release lock

- licensing/fictional naming cleared
- source/license records complete
- UI/localization complete
- data-asset version locked
- regression suite passed
- target platform performance passed
