# Vehicle Implementation Backlog

Do **not** build all vehicles at once. Use the 677-file catalog as a long-term design universe and promote vehicles into production in waves.

## Wave 0 — Physics prototypes

- [ ] one bicycle
- [ ] one petrol scooter
- [ ] one electric scooter
- [ ] one commuter motorcycle
- [ ] one sport motorcycle
- [ ] one Bengaluru auto
- [ ] one hatchback
- [ ] one sedan
- [ ] one compact SUV
- [ ] one large SUV
- [ ] one EV car
- [ ] one mini truck
- [ ] one heavy truck
- [ ] one city bus
- [ ] one police/emergency vehicle

Goal: prove the controller, wheel counts, two-wheeler lean, three-wheeler stability, heavy-vehicle inertia and electric/ICE drivetrains before scaling art production.

## Wave 1 — First playable district

Target **35–40 visually distinct base vehicles**. Prioritize scooters, motorcycles, autos, hatchbacks, compact SUVs, taxis, delivery vehicles, mini trucks and city buses.

## Wave 2 — Bengaluru identity

Add water tankers, municipal vehicles, tech-park cabs/buses, EV diversity, vintage Indian cars, construction traffic and complete police/fire/ambulance coverage.

## Wave 3 — Enthusiast/progression content

Add tuner cars, off-road vehicles, superbikes, luxury cars, sports cars, supercars and deep customization.

## Wave 4 — Outer world and special systems

Add agriculture, industrial machinery, airport ground equipment, rail, rotorcraft, aircraft, watercraft and mission-only special vehicles.

## Per-vehicle task template

For `<VEH-ID>`:

- [ ] `VREF` reference/data verification
- [ ] `VDES` original production design
- [ ] `VBLK` blockout
- [ ] `VMOD` production model
- [ ] `VMAT` UV/materials
- [ ] `VRIG` rig/articulation
- [ ] `VPHY` Chaos physics
- [ ] `VDMG` damage model
- [ ] `VAUD` audio
- [ ] `VVFX` VFX
- [ ] `VAI` traffic/AI integration
- [ ] `VLOD` optimization/LODs
- [ ] `VUI` UI/catalog/localization
- [ ] `VQAT` QA test suite
- [ ] `VREL` release/legal sign-off
