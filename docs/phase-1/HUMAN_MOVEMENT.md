# Human movement foundation

The player uses Epic's bundled UE 5.8 Manny skeletal mannequin, authored idle/walk/run/jump/fall/landing animation, and the template's Control Rig foot IK. This is an articulated mannequin, not a finished human character model or a biomechanical human simulation.

`ABP_NammaHuman` is a copy of the template animation blueprint with `UNammaHumanAnimInstance` as its native parent. Its animation proxy evaluates the original graph, then applies crouch and right-hand reach. Crouching lowers the pelvis by 60 cm and solves both hip–knee–ankle chains against the original planted feet. Limb lengths cannot stretch. Pose interpolation uses `1 - exp(-12 * dt)`. Actor/physics state is read on the game thread before animation evaluation.

Walking uses Unreal CharacterMovement collision sweeps, floor/slope tests, acceleration, braking, and gravity. Chaos handles movable objects and the mannequin's physics-asset bodies and joint constraints. Ordinary locomotion is animation-driven over a movement capsule; the limbs become simulated rigid bodies in ragdoll mode. This is the conventional game-character arrangement, not an active-ragdoll balance controller.

| Parameter | Value |
| --- | --- |
| Character mass | 75 kg |
| Walk / sprint / crouch speed caps | 3.5 / 6 / 1.75 m/s |
| Acceleration / braking deceleration | 10 / 12 m/s² |
| Gravity | 9.81 m/s² |
| Jump launch velocity | 4.2 m/s |
| Calculated unobstructed jump apex / airtime | 0.90 m / 0.86 s |
| Maximum step / walkable slope | 0.35 m / 45° |
| Physics substep | Up to 1/120 s, maximum 8 substeps |
| Maximum pickup mass | 20 kg |

The jump numbers follow `h = v²/(2g)` and `t = 2v/g`, assuming takeoff and landing at the same elevation with no obstruction. Substepping has a finite catch-up budget; long stalls are not guaranteed to preserve real-time simulation.

Controls: WASD move, mouse look, Shift sprint, Space jump or attempt traversal, hold C crouch, E delivery interaction, F grab/drop, X ragdoll/reset, Escape pause. Aim down towards a nearby crate before pressing F. The two ochre crates near the start weigh 12 kg and 60 kg; only the lighter one is liftable. The physics handle uses a damped constraint, preserves gravity/collision with the environment, and releases when obstructed or stretched too far. X a second time resets the character to the start; there is no get-up animation yet.

Traversal now sweeps the capsule and aborts on obstruction instead of teleporting through geometry. The existing traversal path is still a prototype without authored mantle/vault animation or hand planting. Delivery E remains an instantaneous game action. Hand IK currently follows physics pickup only. Those are the next animation/interaction gaps; this change does not claim film-quality or human-equivalent movement.

## Reproduce

1. `Scripts/build_editor.sh`
2. `Scripts/setup_human_character.sh` imports bundled resources, prepares the native-parent animation blueprint and adds test crates without regenerating the street.
3. `Scripts/test_host.sh`
4. `Scripts/test_human.sh` runs Unreal automation and writes reports to `Saved/Automation/Human`.
5. `Scripts/launch_editor.sh`, then Play. Rebuild/repackage if using the previously packaged standalone game.

The bundled `/Game/Characters/Mannequins` resources retain their original package paths and Epic licensing. They are Unreal Engine template content, not original project art. The custom animation blueprint lives under `/Game/NammaCity/Characters`.

Reference: [Epic's two-bone IK documentation](https://dev.epicgames.com/documentation/unreal-engine/animation-blueprint-two-bone-ik-in-unreal-engine).

## Verification, 2026-09-08

`Scripts/build_editor.sh` and `Scripts/test_host.sh` passed. Unreal automation passed `NammaCity.Delivery.Rules` and `NammaCity.Human.PhysicsAndPose` with zero warnings or errors. The latter checks asset loading, native animation integration, skeleton joints, physics constraints, gravity-based jump calculation, grounded crouch with planted feet, finite poses, mass-limited pickup/drop, ragdoll/reset, and fixed-length IK at singular/unreachable targets. It runs an isolated 60 Hz world; this does not establish 30 fps visual quality or validate every slope/ledge.

`Scripts/package_game.sh` completed successfully, including the Linux game build, cook, stage and archive. `Scripts/launch_game.sh` now uses the upgraded package.

Final packaged Vulkan render checked on 2026-09-09: the camera shows the whole mannequin, including both feet. [In-game screenshot](../../media/human-movement-preview.png).
