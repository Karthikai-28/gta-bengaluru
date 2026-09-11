# Health, recovery and melee combat

Implemented in source; engine compilation and gameplay verification are pending the workspace-credit approval block. The already-packaged game does not contain these features.

- 100 health and 100 stamina, visible on foot and on the cycle. Health persists through get-up; explicit respawn restores it.
- Left mouse: punch. Each attack costs 15 stamina, has a 100 ms wind-up, a 75 ms extension, a 110 ms recoil, one hit check at 165 ms, and a 0.5 second total cooldown. Alternating hands use anatomical two-bone IK with a 98% reach limit, tucked elbow poles, forearm-aligned wrists, geometric finger closure and torso/clavicle rotation. The opposite hand stays in guard.
- Right mouse: hold guard. Hits within the forward guard cone lose 80% of their damage if sufficient stamina is available. A guard spends 0.8 stamina per incoming damage point; exhausted guards fail. Stamina regenerates at 22 per second outside attacks, guard, recovery and ragdoll.
- Unblocked punches deal 18 damage. Rapid blows accumulate stagger; enough stagger causes a nonfatal knockdown. A fist-sized hit sweep uses the same arm reach and strike phase as the pose, and stops at walls and the first blocking target. There are no weapons or ranged attacks in this version.
- Normal falls apply game damage above 7 m/s; cycle ejections use a lower 3 m/s threshold. Both use `min(100, 3 * (speed - threshold)^2)` above that threshold. Ordinary jumps do not damage health. This is gameplay tuning, not an injury simulation.
- After a nonfatal ragdoll, X requests getting up locally. Automatic recovery starts after 1.5 seconds, provided the body has settled and a standing capsule fits. The character rises through a 0.8 second procedural crouch/leg-IK pose. This is not a motion-captured prone/supine get-up animation. Blocked standing space leaves the character down instead of teleporting through a wall.
- Once recovered, approach the same cycle and press E to ride again. Recovery does not heal or reset the delivery.
- At zero health, the character remains down. R respawns. The pause-menu delivery restart also resets the sparring partner.
- A passive sparring partner spawns about 9.5 metres ahead of PlayerStart on clear ground. They retaliate when punched, stop engaging downed characters and riders, and have a limited pursuit radius. Movement uses collision-swept direct pursuit; it does not yet navigate around complex obstacles.

Host tests cover health bounds, stamina, blocking/exhaustion, lethal damage, stagger and impact thresholds. Native Unreal tests cover a reachable punch, cooldown, get-up location, death/respawn and local recovery/remount. Native tests have been written but not run for this change. Run the existing build/verification workflow after the workspace execution block is cleared.
