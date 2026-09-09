# Playable cycle

The repair-shop cycle remains in the world. A second cycle is placed near the safehouse start at (-44.1, -33.7) metres, facing along the street. Each spawn chooses a new saturated frame color; `ColorSeed` can lock a particular color for reproducible tests. Wheels, tires, saddle and drivetrain retain their material colors.

## Implementation plan and scope

1. Audit the existing force model, contact integration, drivetrain coordinates, possession and rider IK.
2. Correct the physics integration and mechanical synchronization before adding presentation.
3. Add continuous mounting/dismounting, safe exits, correct input handover, and impact momentum.
4. Build open wheels, visible spokes, gear teeth, fixed-length chain routing and randomized frame paint.
5. Run host numerical tests, full Unreal world tests, keyboard-input gameplay tests, and a packaged Vulkan render.

## Controls

Approach a cycle and press **E**. **W** pedals, **A/D** steers, **Shift** increases rider power. **S** applies the rear brake; **Space** applies the front brake. Use the mouse wheel or **1–6** to change gears. Release W to freewheel. Slow below 5 km/h and press **E** to get off. **X** bails out into ragdoll; X again resets the human. Escape pauses; R restarts from pause.

Mounting and dismounting blend the body over 0.45 seconds. Hands follow the actual rotating handlebar grips. Feet follow opposing pedals, driven by crank angle rather than a time-based animation. The hips stay on the saddle, the torso folds forward, and the left foot plants on the road at rest. Foot/knee and hand/elbow IK keeps anatomical limb lengths fixed. A blocked exit leaves the rider aboard; dismount requires a walkable surface and a clear capsule path. There is no unchecked fallback teleport into nearby geometry.

## Physics and calculations

The existing SI-unit bicycle solver is retained and corrected. It integrates at **240 Hz**, including suspension, contact queries, motion and collision sweeps. Up to 24 substeps cover a 100 ms frame; longer stalls drop excess time instead of multiplying displacement by an unsimulated full-frame interval. Unreal's Chaos scene supplies ground/wall collision queries and receives collision impulses for movable objects; the human uses Chaos rigid-body ragdoll after a crash.

This is a custom force-based bicycle movement component with an assisted balance controller, **not** a Chaos Vehicles chassis or an unconstrained rigid-body bicycle. The wheels and chain are articulated visuals driven by solved angular state, not separate colliding bodies. The chain uses an ideal tensioner path rather than individually simulated chain-link joints. Those choices keep one human and two cycles feasible on the project's integrated GPU.

| Calculation | Model |
| --- | --- |
| Total mass | 13 kg bicycle + 75 kg rider; rider mass removed when parked |
| Wheel geometry | 1.048 m wheelbase, 0.350 m rolling radius, 0.170 m crank |
| Gear ratio | 46 chainring teeth / selected 24, 22, 20, 18, 16 or 14 tooth sprocket |
| Development | `2π × wheel radius × gear ratio` (4.21–7.23 m/crank revolution) |
| Pitch radius | `12.7 mm × teeth / 2π`, continuous-chain approximation |
| Drive | crank torque × drivetrain efficiency / gear ratio |
| Rider effort | 140 Nm torque envelope; 310 W sustained / 520 W sprint, pedal-leverage ripple and cadence rolloff |
| Wheel dynamics | tire/brake/drive torque with thin-hoop inertia `I = m r²` |
| Tire grip | load-dependent slip curve, separate front/rear surface friction, per-wheel friction circles |
| Resistance | aerodynamic `½ρCdAv²`, rolling resistance and `mg sin(grade)` |
| Load transfer | acceleration plus grade moments about the axle contacts |
| Balance | `yaw rate = v tan(steer)/wheelbase`, desired lean from centripetal acceleration, grip-limited steering |
| Impacts | pre-impact velocity retained for rider ejection; reduced-mass impulses transferred to movable bodies |

Wheels roll at their own solved angular rates, including locking and skidding. The cassette follows chain speed; it no longer spins with the rear wheel while freewheeling. The right pedal's top-dead-center definition agrees with the crank transform. Starting does not snap the pedals through a quarter turn. Gear changes wait for a moving chain. Chainstay rotation is orthonormal, chain wraps follow the selected sprocket, and an even, fixed link count is preserved across shifts by a lower-run tensioner path.

The tire/contact model is intentionally simplified: wheel support is a sphere sweep rather than a deforming tire, balance is assisted, and mount transitions are procedural rather than motion-captured. The current cassette shift interpolates effective radius while the chain crosses gears; this is not discrete tooth-contact mechanics.

## Reproduce and test

```sh
Scripts/test_host.sh
Scripts/build_editor.sh
Scripts/setup_cycle.sh
Scripts/test_human.sh
Scripts/playtest_cycle.sh --editor
Scripts/package_game.sh
Scripts/playtest_cycle.sh
Scripts/launch_game.sh
```

Host tests cover chain geometry and closure, power/torque, braking, grip, gearing, coast-down, gradients, determinism and randomized numerical stress. Unreal tests cover world contacts, rider pose, mounting/dismounting, blocked exits, frame-rate comparisons, bumps, skid/endo behavior, and wall impacts. The opt-in Development console command `Namma.Cycle.Playtest` drives real Enhanced Input key events, captures a riding screenshot and exits with a pass/fail status. It is never active during ordinary play or in Shipping builds.

Engine reference: [Epic physics substepping](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine). The custom bicycle loop explicitly substeps its own forces; merely enabling Chaos substepping would not substep ordinary pawn Tick logic.
