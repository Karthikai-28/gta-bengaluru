# Vehicle Specification Schema

Every vehicle document in this repository uses the same specification structure so art, gameplay, physics, AI, audio and QA can work from a shared contract.

## Required sections

1. Document status and reference/legal status
2. Design intent and Bengaluru usage
3. Exterior design package and colors
4. Dimensions, mass, payload, capacity, center of gravity and aerodynamic target
5. Powertrain, displacement/battery, power, torque, drivetrain, transmission and range
6. Performance and mobility
7. Chassis, rigidity, structural integrity and deformation
8. Suspension and ride
9. Wheels, tyres, brakes, ABS/TCS/ESC and puncture states
10. Steering and handling
11. Damage/health model
12. Terrain and weather response
13. Occupants, cargo and interactions
14. Audio
15. Lighting/electrical
16. AI traffic profile
17. Spawn/economy
18. Customization
19. Unreal Engine / Chaos implementation
20. Animation
21. LOD/performance budget
22. Cameras
23. VFX
24. Gameplay balance
25. Acceptance criteria
26. OEM/reference verification checklist

## Important data rule

Keep **two layers of values** throughout development:

- **Reference values** — verified values for a specific real-world vehicle/model year/variant, used only as research input.
- **Simulation values** — values intentionally tuned for Namma City gameplay and Chaos physics.

Never silently present simulation values as factual manufacturer specifications.
