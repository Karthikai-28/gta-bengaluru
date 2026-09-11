# Strike motion from reference footage

The player's punches and kicks are not hand-animated and not hand-tuned
curves. They are motion templates measured from public boxing and
kickboxing tutorials, reduced to a form the game's procedural IK can follow on
any skeleton.

## What is here

- `reference_clips.json`: the YouTube tutorials used, by id and title. The
  videos are fetched on demand into `Saved/MotionReference` and never
  committed.
- `strikes.json`: the templates. For each of jab, cross, front kick and
  roundhouse kick: duration, impact fraction, reach, how many
  demonstrations it was measured from and where, and 24 samples of the
  striking fist and elbow (or foot and knee), the guard hand and elbow, the
  pelvis shift, and the pelvis yaw, spine twist, forward and side lean and
  head yaw.
- `../../Source/NammaCity/NammaStrikeData.h`: the same data as `constexpr`
  tables, generated, consumed by `NammaStrikeMotion.h`.

## How a template is made

`Scripts/build_strike_motion.sh` runs the whole chain:

1. `Scripts/track_reference_pose.py` runs MediaPipe Pose (the heavy model) on
   every frame and keeps the 33 world landmarks, image landmarks and
   visibilities.
2. `Scripts/extract_strike_motion.py` finds strikes as wrist-speed peaks and
   ankle-lift peaks, widens each to the window in which the limb leaves guard
   and returns, and discards windows that cross a cut or an undetected frame.
3. Each window gets its own stance frame. Forward is where the strike lands
   (the striking limb's horizontal direction from its root at impact), up is
   gravity, right follows from the right-handed world. Nothing guesses which
   way the fighter faces: MediaPipe's notion of that is poor in three-quarter
   views and presenters turn between talking and demonstrating.
4. Facing the camera, forward is camera depth, which MediaPipe estimates far
   worse than the image plane. Those windows have each limb's forward offset
   rebuilt from rigid segment lengths and the trusted in-plane offset.
5. Events are labelled from geometry (thrust versus arc for kicks, shoulder
   turn for jab versus cross, rise for uppercuts, elbow offset for hooks),
   then checked against the tutorial's word-timed captions: a window in which
   the coach names exactly one technique takes that label, several must agree
   with the geometry, and a clip that never mentions punching contributes no
   punches (nor kicks, the other way round). Hook- and uppercut-shaped
   motions are recognised only to be discarded: in this footage a frontal
   hook is indistinguishable from an arm held out while the coach talks, and
   raising the hands into guard reads as an uppercut. Both were tried and
   the contact sheets showed exactly those, so they are not shipped.
6. Limb positions are stored relative to their root joint in units of that
   limb's length, so the same curve fits Manny or any other rig. Events of one
   kind are time-warped so their impacts coincide and the per-sample median
   is taken.

Pass `--review` to get a contact sheet per detected strike under
`Saved/MotionReference/review`; the labels are worth eyeballing after adding
a clip.

## What the game does with it

`NammaHumanAnimInstance.cpp` places the fist and elbow (or foot and knee)
relative to the shoulder (or hip) of the pose being evaluated, scaled by that
limb's length, and solves two-bone IK with the tracked elbow or knee as the
pole target. The elbow therefore goes where the boxer's elbow went, which is
what fixed the elbows drifting across the chest: the old pole vectors were
hand-tuned offsets whose sign was wrong for Manny's component space, where
the character's left is +X.

`NammaCombatBrain.h` decides which template to play from one attack key, in
the manner of GTA's melee: a jab, cross, jab combination in punching range
with a heavier finisher, kicks when only a leg reaches, a push kick to shove
a guard away, a kick for a downed opponent.

## Limits

Monocular pose estimation. Depth is reconstructed, not measured; a single
demonstration is noisy, and the medians are only as good as the number of
clean demonstrations found (see `events` per strike in `strikes.json`). The
durations are the quicker third of the demonstrations, clamped to what a
real strike takes, because coaches demonstrate slowly as often as at speed.
