#!/usr/bin/env python3
"""Turn tracked boxing/kickboxing footage into strike motion templates.

Input: the .pose.npz files written by Scripts/track_reference_pose.py.
Output: Art/Motion/strikes.json (the templates and their provenance) and
Source/NammaCity/NammaStrikeData.h (the same data as constexpr tables for the
game). Both are regenerated wholesale; do not edit them by hand.

How a strike becomes a template:

1. Punches are wrist-speed peaks and kicks are ankle-lift peaks, each widened
   to the window in which the limb leaves guard and returns. Windows that span
   a cut, an undetected frame, or (for punches) a raised leg are discarded.
2. Each window gets its own stance frame: forward is where the strike lands
   (the striking limb's horizontal direction from its root at impact), up is
   gravity, right follows from the right-handed world. Presenters turn between
   talking and demonstrating, and MediaPipe's idea of where a body faces is
   poor in three-quarter views, so no facing is ever guessed.
3. MediaPipe estimates camera depth far worse than the image plane. When the
   fighter faces the camera, forward is depth, so each limb's forward offset
   is rebuilt from rigid segment lengths and the trusted in-plane offset.
4. Events are labelled jab/cross and front/roundhouse kick from the limb
   path, then mirrored so the striking limb is the orthodox one (jab and
   front kick lead with the left; cross and roundhouse come from the right).
   A jab and a cross are told apart by how far the shoulders turn, not by
   guessing which foot is in front. Hook- and uppercut-shaped motions are
   recognised and discarded, see STRIKES below.
5. Limb positions are stored relative to their own root joint (shoulder or
   hip) in units of that limb's length, so the same curve fits any skeleton;
   torso motion is stored as angles relative to the guard at the window start.
   The elbow and knee are kept because the in-game IK uses them as pole
   targets: the elbow goes where the boxer's elbow went.
6. Events of one kind are time-warped so their impact frames coincide, then
   the per-sample median is taken, which keeps the shape of a typical strike
   and discards the odd mis-tracked one.

Usage: Scripts/extract_strike_motion.py [POSE.npz ...] [--frames DIR]
  --frames DIR   also write a contact sheet per detected event for review
  NAMMA_STRIKE_DEBUG=1 prints every candidate and why it was kept or dropped
"""
import argparse
import json
import os
import sys
from pathlib import Path

import numpy as np

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = PROJECT / 'Saved' / 'MotionReference'
JSON_OUT = PROJECT / 'Art' / 'Motion' / 'strikes.json'
HEADER_OUT = PROJECT / 'Source' / 'NammaCity' / 'NammaStrikeData.h'
SAMPLES = 24
DEBUG = bool(os.environ.get('NAMMA_STRIKE_DEBUG'))

# MediaPipe Pose landmark indices.
NOSE, L_EAR, R_EAR = 0, 7, 8
L_SHOULDER, R_SHOULDER, L_ELBOW, R_ELBOW, L_WRIST, R_WRIST = 11, 12, 13, 14, 15, 16
L_HIP, R_HIP, L_KNEE, R_KNEE, L_ANKLE, R_ANKLE = 23, 24, 25, 26, 27, 28
L_HEEL, R_HEEL, L_TOE, R_TOE = 29, 30, 31, 32
MIRROR = list(range(33))
for a, b in [(1, 4), (2, 5), (3, 6), (7, 8), (9, 10), (11, 12), (13, 14), (15, 16), (17, 18),
             (19, 20), (21, 22), (23, 24), (25, 26), (27, 28), (29, 30), (31, 32)]:
    MIRROR[a], MIRROR[b] = b, a
SHOULDERS, ELBOWS, WRISTS = [L_SHOULDER, R_SHOULDER], [L_ELBOW, R_ELBOW], [L_WRIST, R_WRIST]
HIPS, KNEES, ANKLES = [L_HIP, R_HIP], [L_KNEE, R_KNEE], [L_ANKLE, R_ANKLE]
UP = np.array([0.0, -1.0, 0.0])  # MediaPipe world space: x right, y down, z toward the camera

# Hooks and uppercuts are recognised only to be excluded: in the tutorials
# available a frontal hook is indistinguishable from an arm held out while the
# coach talks, and raising the hands into guard reads as an uppercut. Their
# shapes are still classified so they never pollute the jab or the cross.
STRIKES = ['jab', 'cross', 'front_kick', 'roundhouse_kick']
KICKS = {'front_kick', 'roundhouse_kick'}
# Which limb throws each strike in the orthodox (left-lead) template.
LEAD = {'jab': True, 'cross': False, 'front_kick': True, 'roundhouse_kick': False}


def unit(v):
    n = np.linalg.norm(v, axis=-1, keepdims=True)
    return v / np.where(n < 1e-9, 1.0, n)


def smooth(x, width=5):
    """Box smoothing along axis 0, NaN-aware (gaps are bridged linearly)."""
    x = np.array(x, dtype=np.float64)
    bad = np.isnan(x).any(axis=tuple(range(1, x.ndim))) if x.ndim > 1 else np.isnan(x)
    if bad.all():
        return x
    idx = np.arange(len(x))
    flat = x.reshape(len(x), -1)
    for k in range(flat.shape[1]):
        col = flat[:, k]
        col[bad] = np.interp(idx[bad], idx[~bad], col[~bad])
    width |= 1  # odd, so the output keeps its length
    kernel = np.ones(width) / width
    pad = width // 2
    out = np.empty_like(flat)
    for k in range(flat.shape[1]):
        out[:, k] = np.convolve(np.pad(flat[:, k], pad, mode='edge'), kernel, mode='valid')
    return out.reshape(x.shape)


KEYWORDS = {
    'jab': 'jab', 'jabs': 'jab', 'cross': 'cross', 'hook': 'hook', 'hooks': 'hook',
    'uppercut': 'uppercut', 'uppercuts': 'uppercut', 'teep': 'front_kick', 'roundhouse': 'roundhouse_kick',
    'sidekick': 'side_kick',
    # Family evidence only: a clip that never says any of these has none of them.
    'kick': 'any_kick', 'kicks': 'any_kick', 'kicking': 'any_kick', 'punch': 'any_punch', 'punches': 'any_punch',
}
BIGRAMS = {
    ('front', 'kick'): 'front_kick', ('push', 'kick'): 'front_kick', ('round', 'kick'): 'roundhouse_kick',
    ('turning', 'kick'): 'roundhouse_kick', ('side', 'kick'): 'side_kick', ('back', 'kick'): 'back_kick',
    ('spinning', 'kick'): 'back_kick', ('straight', 'right'): 'cross', ('straight', 'left'): 'jab',
}
PUNCHES = {'jab', 'cross', 'hook', 'uppercut'}  # caption topics; only jab and cross become templates


class Captions:
    """Word-timed auto-captions: which technique the coach is talking about when.

    A tutorial names the technique before demonstrating it. A window in which
    exactly one technique is named is labelled by the coach; one in which
    several are named must agree with the geometry; one with none is left to
    the geometry alone."""

    def __init__(self, path):
        import re
        self.words = []
        if not path.is_file():
            return
        text = path.read_text(encoding='utf-8', errors='replace')
        for stamp, word in re.findall(r'<(\d\d:\d\d:\d\d\.\d\d\d)><c>\s*([^<]+)</c>', text):
            h, m, sec = stamp.split(':')
            self.words.append((int(h) * 3600 + int(m) * 60 + float(sec), word.strip().lower().strip('.,!?')))

    def topics(self, t0, t1):
        found = set()
        for i, (t, word) in enumerate(self.words):
            if not t0 <= t <= t1:
                continue
            if word in KEYWORDS:
                found.add(KEYWORDS[word])
            if i + 1 < len(self.words) and (word, self.words[i + 1][1]) in BIGRAMS:
                found.add(BIGRAMS[(word, self.words[i + 1][1])])
        return found

    def teaches(self, family):
        """Whether the whole clip ever names a technique of this family. A jab
        tutorial with well-captioned speech that never says "kick" has no
        kicks in it, whatever a lunge looks like to the geometry."""
        if len(self.words) < 50:
            return True
        return any((t in PUNCHES or t == 'any_punch') == (family == 'punch') for t in self.topics(-1, 1e9))

    def resolve(self, kind, family, t0, t1):
        """The final label for a candidate the geometry called `kind` (or None
        when the geometry is unsure but the motion is a plausible strike)."""
        if not self.teaches(family):
            return None, {'nothing of this kind in the whole clip'}
        named = {t for t in self.topics(t0, t1) if not t.startswith('any_') and (t in PUNCHES) == (family == 'punch')}
        if len(named) == 1:
            only = next(iter(named))
            return (only if only in STRIKES else None), named
        if not named:
            return kind, named
        return (kind if kind in named else None), named


def yaw_of(v):
    """Heading of a stance-frame vector about up, degrees, positive to the right."""
    return np.degrees(np.arctan2(v[..., 1], v[..., 0]))


def wrap(degrees):
    return (degrees + 180) % 360 - 180


class Clip:
    """One tracked video with the per-window analysis the templates need."""

    def __init__(self, path: Path):
        data = np.load(path, allow_pickle=True)
        self.name = path.stem.replace('.pose', '')
        self.fps = float(data['fps'])
        self.captions = Captions(path.with_name(self.name + '.en.vtt'))
        world = data['world'].astype(np.float64)
        if world.ndim != 3 or len(world) == 0:
            raise ValueError(f'{self.name}: no tracked frames')
        vis = data['visibility']
        image = data['image'].astype(np.float64)
        # Landmarks the model could not see are still estimated; only drop the
        # ones it has no idea about. Hips out of frame are a common estimate.
        world[vis < 0.1] = np.nan
        # Edited footage: a jump of the torso or a change of shoulder width in
        # the image is a cut or a zoom, and no strike may span one.
        torso_image = image[:, [L_SHOULDER, R_SHOULDER], :2].mean(axis=1)
        width = np.linalg.norm(image[:, L_SHOULDER, :2] - image[:, R_SHOULDER, :2], axis=1)
        jump = np.linalg.norm(np.diff(torso_image, axis=0), axis=1) > 0.12
        zoom = np.abs(np.diff(width)) > 0.35 * np.maximum(width[:-1], 1e-3)
        detected = np.isfinite(torso_image).all(axis=1)
        self.cut = ~detected
        self.cut[1:] |= (jump | zoom) & detected[1:] & detected[:-1]
        self.world = smooth(world, width=max(3, int(round(self.fps / 6))))
        # Segment lengths: MediaPipe limbs are not rigid, so take a high
        # percentile of the 3D length as the true one.
        self.segment = {}
        for parent, child in ((L_SHOULDER, L_ELBOW), (L_ELBOW, L_WRIST), (R_SHOULDER, R_ELBOW), (R_ELBOW, R_WRIST),
                              (L_HIP, L_KNEE), (L_KNEE, L_ANKLE), (R_HIP, R_KNEE), (R_KNEE, R_ANKLE)):
            length = np.linalg.norm(self.world[:, child] - self.world[:, parent], axis=1)
            self.segment[(parent, child)] = float(np.nanpercentile(length, 97))
        self.arm = [self.segment[(L_SHOULDER, L_ELBOW)] + self.segment[(L_ELBOW, L_WRIST)],
                    self.segment[(R_SHOULDER, R_ELBOW)] + self.segment[(R_ELBOW, R_WRIST)]]
        self.leg = [self.segment[(L_HIP, L_KNEE)] + self.segment[(L_KNEE, L_ANKLE)],
                    self.segment[(R_HIP, R_KNEE)] + self.segment[(R_KNEE, R_ANKLE)]]
        self.lift = [self.relative(L_ANKLE, L_HIP, self.leg[0]) @ UP, self.relative(R_ANKLE, R_HIP, self.leg[1]) @ UP]
        self.shoulder_width = float(np.nanpercentile(np.linalg.norm(self.world[:, R_SHOULDER] - self.world[:, L_SHOULDER], axis=1), 97))
        self.hip_width = float(np.nanpercentile(np.linalg.norm(self.world[:, R_HIP] - self.world[:, L_HIP], axis=1), 97))

    def relative(self, joint, root, length):
        return (self.world[:, joint] - self.world[:, root]) / length

    def speed(self, rel):
        return np.linalg.norm(np.gradient(rel, axis=0) * self.fps, axis=1)

    @staticmethod
    def peaks(signal, threshold, min_gap):
        found = []
        for i in range(1, len(signal) - 1):
            if signal[i] >= threshold and signal[i] >= signal[i - 1] and signal[i] > signal[i + 1]:
                if found and i - found[-1] < min_gap:
                    if signal[i] > signal[found[-1]]:
                        found[-1] = i
                    continue
                found.append(i)
        return found

    @staticmethod
    def window(quiet_before, quiet_after, peak, max_len):
        """Frames around a peak bounded by the limb being settled on both sides,
        or None if it never settles within max_len frames of the peak."""
        start = peak
        while start > 0 and peak - start < max_len and not quiet_before[start - 1]:
            start -= 1
        end = peak
        while end < len(quiet_after) - 1 and end - peak < max_len and not quiet_after[end + 1]:
            end += 1
        if peak - start >= max_len or end - peak >= max_len:
            return None
        return start, end

    @staticmethod
    def strike_window(rel, speed, peak, max_len):
        """Frames around a speed peak: the fast core of the motion, widened
        back to the last quiet frame and forward to the first quiet frame at
        which the limb is back near where it started. None if it never is."""
        quiet = speed < max(0.5, 0.15 * speed[peak])
        fast = speed > 0.3 * speed[peak]
        core_start = peak
        while core_start > 0 and fast[core_start - 1]:
            core_start -= 1
        core_end = peak
        while core_end < len(speed) - 1 and fast[core_end + 1]:
            core_end += 1
        start = core_start
        while start > 0 and peak - start < max_len and not quiet[start]:
            start -= 1
        if peak - start >= max_len:
            return None
        # Prefer the first quiet frame at which the limb is back near where it
        # started; failing that, the first sustained quiet after the core. A
        # coach often pauses in the landed position rather than returning to
        # guard, and the template's tail is bent back to guard anyway.
        back = np.linalg.norm(rel - rel[start], axis=1) < 0.25
        first_quiet = None
        end = core_end + 1
        while end < len(speed) - 1 and end - peak < max_len:
            if quiet[end] and quiet[end - 1]:
                if back[end]:
                    return start, end
                if first_quiet is None:
                    first_quiet = end
            end += 1
        return (start, first_quiet) if first_quiet is not None else None

    def usable(self, start, end, joints):
        if start <= 0 or end >= len(self.world) - 1 or self.cut[start:end + 1].any():
            return False
        return np.isfinite(self.world[start:end + 1][:, joints]).all()

    def local(self, start, end, aim):
        """Stance-frame coordinates for a window.

        Forward is where the strike lands: the horizontal direction of the
        striking limb from its root at impact. That is the direction the game
        aims a strike, and it needs no guess about where the fighter faces.
        Returns (body, frontal): body[f, joint] = (forward, right, up) in metres
        relative to the hip centre at the window start."""
        forward = aim - (aim @ UP) * UP
        if np.linalg.norm(forward) < 0.05 or not np.isfinite(forward).all():
            return None
        forward = unit(forward)
        # Facing the camera, the depth component is noise around the truth:
        # snap to the camera axis and let the lifting below rebuild depth.
        if abs(forward[2]) > 0.7:
            forward = np.array([0.0, 0.0, np.sign(forward[2])])
        right = np.cross(forward, UP)  # world space is right-handed
        basis = np.stack([forward, right, UP])
        frontal = float(abs(forward[2]))
        origin = np.nanmean(self.world[start, [L_HIP, R_HIP]], axis=0)
        body = (self.world[start:end + 1] - origin) @ basis.T
        if frontal > 0.7:
            self.lift_depth(body)
        self.last_forward = np.round(forward, 2)
        return body, frontal

    def lift_depth(self, body):
        """Facing the camera, forward is depth. Rebuild each limb's forward offset
        from rigid segment lengths instead: the in-plane offset is trusted, and
        the remainder of the segment lies along forward, in front of the parent
        joint unless the model is confident it is behind."""
        # Torso first: a turned shoulder or hip line foreshortens in the image,
        # and which end comes forward is the one thing depth is trusted for.
        for a, b, width in ((L_SHOULDER, R_SHOULDER, self.shoulder_width), (L_HIP, R_HIP, self.hip_width)):
            offset = body[:, b] - body[:, a]
            planar = np.linalg.norm(offset[:, 1:], axis=1)
            depth = np.sqrt(np.maximum(width * width - planar * planar, 0.0))
            middle = 0.5 * (body[:, a, 0] + body[:, b, 0])
            sign = np.sign(offset[:, 0] + 1e-9)
            body[:, a, 0] = middle - 0.5 * sign * depth
            body[:, b, 0] = middle + 0.5 * sign * depth
        for (parent, child), length in self.segment.items():
            offset = body[:, child] - body[:, parent]
            planar = np.linalg.norm(offset[:, 1:], axis=1)
            depth = np.sqrt(np.maximum(length * length - planar * planar, 0.0))
            sign = np.where(offset[:, 0] < -0.35 * length, -1.0, 1.0)
            body[:, child, 0] = body[:, parent, 0] + sign * depth

    def punches(self):
        events = []
        self.tally = {}
        legs_up = (np.nan_to_num(self.lift[0], nan=-1) > -0.7) | (np.nan_to_num(self.lift[1], nan=-1) > -0.7)
        for side in (0, 1):
            shoulder, elbow, wrist = SHOULDERS[side], ELBOWS[side], WRISTS[side]
            rel = self.relative(wrist, shoulder, self.arm[side])
            speed = self.speed(rel)  # arm lengths per second
            extension = np.nan_to_num(np.linalg.norm(rel, axis=1))
            speed = np.nan_to_num(speed)
            rel_clean = np.nan_to_num(rel)
            for peak in self.peaks(speed, 2.5, int(self.fps * 0.25)):
                self.tally['punch peaks'] = self.tally.get('punch peaks', 0) + 1
                window = self.strike_window(rel_clean, speed, peak, int(self.fps * 1.0))
                if window is None:
                    self.tally['punch never settles'] = self.tally.get('punch never settles', 0) + 1
                    continue
                start, end = window
                if end - start < self.fps * 0.15 or not self.usable(start, end, [shoulder, elbow, wrist, L_HIP, R_HIP, NOSE]):
                    self.tally['punch unusable frames'] = self.tally.get('punch unusable frames', 0) + 1
                    continue
                if legs_up[start:end + 1].any():
                    self.tally['punch during kick'] = self.tally.get('punch during kick', 0) + 1
                    continue  # an arm swinging during a kick is not a punch
                # Impact: where the fist is farthest from where it started. Full
                # extension from the shoulder would pick a dropped hand for a
                # hook or an uppercut, whose arm stays bent, so the gate is on
                # travel, not on straightening.
                travel = np.linalg.norm(rel_clean[start:end + 1] - rel_clean[start], axis=1)
                impact = int(np.argmax(travel))
                if impact <= 0 or impact >= end - start or travel[impact] < 0.3:
                    self.tally['punch too small'] = self.tally.get('punch too small', 0) + 1
                    continue
                local = self.local(start, end, self.world[start + impact, wrist] - self.world[start + impact, shoulder])
                if local is None:
                    continue
                body, frontal = local
                fist = (body[:, wrist] - body[:, shoulder]) / self.arm[side]
                elbow_rel = (body[:, elbow] - body[:, shoulder]) / self.arm[side]
                ext = np.linalg.norm(fist, axis=1)
                path = fist[impact] - fist[0]
                forward, lateral, rise = path
                # How far the elbow sits off the shoulder-to-fist line, sideways:
                # a hook's upper arm is across the strike, a jab's is under it.
                elbow_out = abs(elbow_rel[impact, 1]) * (1 if side == 0 else 1)
                # A cross turns the rear shoulder through; a jab barely rotates.
                line = body[:, R_SHOULDER] - body[:, L_SHOULDER]
                turn = abs(wrap(yaw_of(line[impact]) - yaw_of(line[0])))
                if rise > 0.3 and rise > abs(forward) and fist[impact, 2] > -0.35 and elbow_rel[impact, 2] < fist[impact, 2]:
                    kind = 'uppercut'
                elif elbow_out > 0.3 and elbow_rel[impact, 2] > -0.4 and ext[impact] < 0.9:
                    kind = 'hook'
                elif forward > 0.25 and ext[impact] > 0.82 and abs(rise) < 0.3:
                    kind = 'cross' if turn > 18 else 'jab'
                else:
                    kind = None
                # Lands in front, at least chest high, and ended up further out
                # than it started: anything else is a hand moving, not a punch.
                plausible = fist[impact, 0] > 0.35 and fist[impact, 2] > -0.45
                labelled, named = self.captions.resolve(kind, 'punch', start / self.fps - 8, end / self.fps + 2)
                plausible = plausible and ext[impact] > 0.72 and labelled in STRIKES
                if labelled != kind and DEBUG:
                    print(f'      coach says {sorted(named)}: {kind} -> {labelled if plausible else None}')
                kind = labelled if plausible else None
                if DEBUG:
                    print(f'  punch {self.name} {"LR"[side]} t={(start + impact) / self.fps:6.2f} frontal {frontal:.2f} '
                          f'ext {ext[0]:.2f}->{ext[impact]:.2f} path f/r/u {forward:+.2f}/{lateral:+.2f}/{rise:+.2f} '
                          f'at {np.round(fist[impact], 2)} elbow {np.round(elbow_rel[impact], 2)} out {elbow_out:.2f} turn {turn:.0f} '
                          f'win {(end - start) / self.fps:.2f}s -> {kind}')
                if kind:
                    events.append(dict(kind=kind, side=side, start=start, impact=start + impact, end=end,
                                       body=body, frontal=frontal))
        return events

    def kicks(self):
        events = []
        for side in (0, 1):
            hip, knee, ankle = HIPS[side], KNEES[side], ANKLES[side]
            rel = self.relative(ankle, hip, self.leg[side])
            lift = np.nan_to_num(rel @ UP, nan=-1)  # -1 standing, 0 at hip height
            speed = np.nan_to_num(self.speed(rel))
            for peak in self.peaks(lift, -0.62, int(self.fps * 0.4)):
                around = lift[max(0, peak - int(self.fps * 2)):peak + int(self.fps * 2)]
                quiet = lift < around.min() + 0.15  # the foot is back on this fighter's floor
                self.tally['kick peaks'] = self.tally.get('kick peaks', 0) + 1
                window = self.window(quiet, quiet, peak, int(self.fps * 1.5))
                if window is None:
                    self.tally['kick never settles'] = self.tally.get('kick never settles', 0) + 1
                    continue
                start, end = window
                if end - start < self.fps * 0.25 or not self.usable(start, end, [hip, knee, ankle, L_HIP, R_HIP, NOSE]):
                    self.tally['kick unusable frames'] = self.tally.get('kick unusable frames', 0) + 1
                    continue
                reach = np.linalg.norm(rel[start:end + 1], axis=1) * (lift[start:end + 1] > -0.75)
                impact = int(np.argmax(reach))
                if impact <= 3 or impact >= end - start:
                    continue
                local = self.local(start, end, self.world[start + impact, ankle] - self.world[start + impact, hip])
                if local is None:
                    continue
                body, frontal = local
                foot = (body[:, ankle] - body[:, hip]) / self.leg[side]
                # Thrust kicks push the foot out along the line it ends on; a
                # roundhouse swings it across that line. Judge the whole
                # extension, not the instant the foot stops.
                velocity = np.gradient(foot, axis=0) * self.fps
                radial = unit(foot[impact])
                v = velocity[:impact + 1]
                fast = np.linalg.norm(v, axis=1) > 0.3 * np.linalg.norm(v, axis=1).max()
                v = v[fast]
                thrust = np.abs(v @ radial).sum() / max(np.linalg.norm(v, axis=1).sum(), 1e-6)
                hip_line = body[:, R_HIP] - body[:, L_HIP]
                turn = abs(wrap(yaw_of(hip_line[impact]) - yaw_of(hip_line[0])))
                if thrust < 0.55 and foot[impact, 2] > -0.8:
                    kind = 'roundhouse_kick'
                elif thrust >= 0.6 and foot[impact, 2] > -0.8:
                    kind = 'front_kick'
                else:
                    kind = None  # side kicks and anything unclear
                plausible = foot[impact, 2] > -0.8
                labelled, named = self.captions.resolve(kind, 'kick', start / self.fps - 8, end / self.fps + 2)
                if labelled != kind and DEBUG:
                    print(f'      coach says {sorted(named)}: {kind} -> {labelled if plausible else None}')
                kind = labelled if plausible else None
                if DEBUG:
                    print(f'  kick  {self.name} {"LR"[side]} t={(start + impact) / self.fps:6.2f} fwd {self.last_forward} '
                          f'foot {np.round(foot[impact], 2)} thrust {thrust:.2f} turn {turn:.0f} '
                          f'win {(end - start) / self.fps:.2f}s -> {kind}')
                if kind:
                    events.append(dict(kind=kind, side=side, start=start, impact=start + impact, end=end,
                                       body=body, frontal=frontal))
        return events

    def sample(self, event):
        """Per-frame template values over the event window, mirrored to orthodox."""
        side, other = event['side'], 1 - event['side']
        frames = event['body']
        arm, leg = list(self.arm), list(self.leg)
        if (side == 0) != LEAD[event['kind']]:
            frames = frames[:, MIRROR].copy()
            frames[..., 1] *= -1
            side, other = other, side
            arm, leg = arm[::-1], leg[::-1]

        def limb(joint, root, length):
            return (frames[:, joint] - frames[:, root]) / length

        centre = frames[:, [L_HIP, R_HIP]].mean(axis=1)
        torso = unit(frames[:, [L_SHOULDER, R_SHOULDER]].mean(axis=1) - centre)
        shoulder_yaw = wrap(yaw_of(frames[:, R_SHOULDER] - frames[:, L_SHOULDER]))
        hip_yaw = wrap(yaw_of(frames[:, R_HIP] - frames[:, L_HIP]))
        shoulder_yaw = wrap(shoulder_yaw - shoulder_yaw[0])
        hip_yaw = wrap(hip_yaw - hip_yaw[0])
        lean_forward = np.degrees(np.arctan2(torso[:, 0], torso[:, 2]))
        lean_side = np.degrees(np.arctan2(torso[:, 1], torso[:, 2]))
        if event['kind'] in KICKS:
            # Hips relative to the planted foot: kicks shift the pelvis over the support leg.
            pelvis = (centre - frames[:, ANKLES[other]]) / leg[other]
        else:
            pelvis = np.zeros_like(centre)
            floor = np.minimum(frames[:, L_ANKLE, 2], frames[:, R_ANKLE, 2])
            pelvis[:, 2] = (centre[:, 2] - floor) / leg[side]
        pelvis = np.nan_to_num(pelvis - pelvis[0])
        head = frames[:, NOSE] - 0.5 * (frames[:, L_EAR] + frames[:, R_EAR])
        head_yaw = wrap(yaw_of(head) - shoulder_yaw)
        out = dict(
            fist=limb(WRISTS[side], SHOULDERS[side], arm[side]),
            elbow=limb(ELBOWS[side], SHOULDERS[side], arm[side]),
            guard_fist=limb(WRISTS[other], SHOULDERS[other], arm[other]),
            guard_elbow=limb(ELBOWS[other], SHOULDERS[other], arm[other]),
            foot=limb(ANKLES[side], HIPS[side], leg[side]),
            knee=limb(KNEES[side], HIPS[side], leg[side]),
            pelvis=pelvis,
            pelvis_yaw=hip_yaw,
            spine_twist=wrap(shoulder_yaw - hip_yaw),
            lean_forward=lean_forward - lean_forward[0],
            lean_side=lean_side - lean_side[0],
            head_yaw=np.nan_to_num(head_yaw - head_yaw[0]),
        )
        return {k: np.nan_to_num(v) for k, v in out.items()}


def warp(values, impact, impact_fraction):
    """Resample an event to SAMPLES points with its impact at impact_fraction."""
    n = len(values) - 1
    t = np.linspace(0, 1, SAMPLES)
    frame = np.where(t <= impact_fraction, impact * t / impact_fraction,
                     impact + (n - impact) * (t - impact_fraction) / (1 - impact_fraction))
    idx = np.arange(n + 1)
    if values.ndim == 1:
        return np.interp(frame, idx, values)
    return np.stack([np.interp(frame, idx, values[:, k]) for k in range(values.shape[1])], axis=1)


def build_templates(clips, frames_dir=None):
    events = {kind: [] for kind in STRIKES}
    for clip in clips:
        for event in clip.punches() + clip.kicks():
            event['clip'] = clip
            events[event['kind']].append(event)
        if DEBUG:
            print(f'  {clip.name}: {len(clip.captions.words)} caption words, ' + ', '.join(f'{k} {v}' for k, v in sorted(clip.tally.items())))
    templates = {}
    for kind in STRIKES:
        found = events[kind]
        if not found:
            print(f'  {kind}: no events found', file=sys.stderr)
            continue
        durations = np.array([(e['end'] - e['start']) / e['clip'].fps for e in found])
        fractions = np.array([(e['impact'] - e['start']) / (e['end'] - e['start']) for e in found])
        # Coaches demonstrate slowly as often as at speed; the game wants the
        # quicker demonstrations, within what a real strike takes.
        duration = float(np.clip(np.percentile(durations, 30), 0.6, 1.0) if kind in KICKS
                         else np.clip(np.percentile(durations, 30), 0.3, 0.6))
        impact_fraction = float(np.clip(np.median(fractions), 0.2, 0.7))
        stacks = {}
        for e in found:
            sampled = e['clip'].sample(e)
            # A cross thrown after a jab starts with the other hand still out;
            # the guard hand's curve comes from demonstrations that began with
            # that hand actually in guard.
            in_guard = np.linalg.norm(sampled['guard_fist'][0]) < 0.6
            for key, values in sampled.items():
                if key.startswith('guard') and not in_guard:
                    continue
                stacks.setdefault(key, []).append((e['frontal'], warp(values, e['impact'] - e['start'], impact_fraction)))
        # Each camera angle is trusted in one horizontal axis: a side view
        # measures forward and up in the image plane but sideways is depth,
        # a front view the reverse (its forward was rebuilt from limb lengths).
        # So the sideways component of every curve is the median over frontal
        # demonstrations and the rest over side-on ones, when enough exist.
        template = {}
        for key, entries in stacks.items():
            everything = np.stack([v for _, v in entries])
            median = np.median(everything, axis=0)
            if median.ndim == 2:
                side_on = [v for f, v in entries if f < 0.7]
                frontal = [v for f, v in entries if f >= 0.7]
                if len(side_on) >= 2:
                    median[:, [0, 2]] = np.median(np.stack(side_on), axis=0)[:, [0, 2]]
                if len(frontal) >= 2:
                    median[:, 1] = np.median(np.stack(frontal), axis=0)[:, 1]
            template[key] = median
        # No frontal demonstration showed the other hand in guard: its sideways
        # position is then depth from a side view, and unusable. Take the
        # mirror of the striking hand's own guard instead, which every event
        # does show, and hold it there.
        if sum(f >= 0.7 for f, _ in stacks.get('guard_fist', [])) < 2:
            for key, own in (('guard_fist', 'fist'), ('guard_elbow', 'elbow')):
                template[key][:, 1] = -template[own][0, 1]
        # Sample 0 is the guard the game holds between strikes, so the
        # retraction must land back on it. Only the tail after impact is bent
        # towards the start; the strike itself is left as tracked.
        t = np.linspace(0, 1, SAMPLES)
        tail = np.clip((t - impact_fraction) / (1 - impact_fraction), 0, 1)
        tail = tail * tail * (3 - 2 * tail)
        for key, v in template.items():
            template[key] = v - (tail[:, None] if v.ndim == 2 else tail) * (v[-1] - v[0])
        strike = np.linalg.norm(template['foot' if kind in KICKS else 'fist'], axis=1)
        templates[kind] = dict(
            kick=kind in KICKS,
            lead=LEAD[kind],
            duration=round(duration, 3),
            impact_fraction=round(impact_fraction, 3),
            reach=round(float(strike.max()), 3),
            events=len(found),
            sources=sorted({e['clip'].name for e in found}),
            events_detail=[dict(clip=e['clip'].name, side='LR'[e['side']], frontal=round(e['frontal'], 2),
                                start=round(e['start'] / e['clip'].fps, 2), impact=round(e['impact'] / e['clip'].fps, 2),
                                end=round(e['end'] / e['clip'].fps, 2)) for e in found],
            samples={k: np.round(v, 4).tolist() for k, v in template.items()},
        )
        print(f'  {kind}: {len(found)} events, {duration:.2f}s, impact at {impact_fraction:.0%}, '
              f'reach {strike.max():.2f}, from {", ".join(templates[kind]["sources"])}')
        if frames_dir:
            for i, e in enumerate(found):
                contact_sheet(e, frames_dir / f'{kind}-{e["clip"].name}-{i:02d}.jpg')
    return templates


def contact_sheet(event, out):
    """Six video frames across the event with the tracked skeleton drawn, for review."""
    import cv2
    clip = event['clip']
    video = DEFAULT_INPUT / f'{clip.name}.mp4'
    if not video.is_file():
        return
    image = np.load(DEFAULT_INPUT / f'{clip.name}.pose.npz')['image']
    cap = cv2.VideoCapture(str(video))
    picks = np.linspace(event['start'], event['end'], 6).astype(int)
    tiles = []
    for f in picks:
        cap.set(cv2.CAP_PROP_POS_FRAMES, int(f))
        ok, frame = cap.read()
        if not ok:
            continue
        h, w = frame.shape[:2]
        for a, b in [(11, 13), (13, 15), (12, 14), (14, 16), (11, 12), (23, 24), (11, 23), (12, 24),
                     (23, 25), (25, 27), (24, 26), (26, 28)]:
            pa, pb = image[f, a], image[f, b]
            if np.isnan(pa).any() or np.isnan(pb).any():
                continue
            colour = (0, 255, 0) if a % 2 else (0, 0, 255)
            cv2.line(frame, (int(pa[0] * w), int(pa[1] * h)), (int(pb[0] * w), int(pb[1] * h)), colour, 3)
        label = 'IMPACT' if f == picks[np.argmin(abs(picks - event['impact']))] else f'{f}'
        cv2.putText(frame, label, (10, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (255, 255, 0), 3)
        tiles.append(cv2.resize(frame, (480, int(480 * h / w))))
    cap.release()
    if tiles:
        out.parent.mkdir(parents=True, exist_ok=True)
        cv2.imwrite(str(out), np.concatenate(tiles, axis=1))


def cpp_name(kind):
    return ''.join(part.title() for part in kind.split('_'))


def write_header(templates, out: Path):
    lines = ['// Generated by Scripts/extract_strike_motion.py from tracked reference footage.',
             '// Do not edit: rerun the script. See Art/Motion/strikes.json for provenance.',
             '#pragma once', 'namespace NammaHuman::StrikeData', '{',
             f'constexpr int SampleCount={SAMPLES};',
             'struct FSample', '{',
             '    float Fist[3], Elbow[3], GuardFist[3], GuardElbow[3], Foot[3], Knee[3], Pelvis[3];',
             '    float PelvisYaw, SpineTwist, LeanForward, LeanSide, HeadYaw;', '};',
             'struct FClip', '{',
             '    const char* Name; bool bKick; bool bLead; float Duration, ImpactFraction, Reach;',
             '    FSample Samples[SampleCount];', '};']
    order = [k for k in STRIKES if k in templates]
    vectors = ('fist', 'elbow', 'guard_fist', 'guard_elbow', 'foot', 'knee', 'pelvis')
    angles = ('pelvis_yaw', 'spine_twist', 'lean_forward', 'lean_side', 'head_yaw')
    for kind in order:
        t = templates[kind]
        s = t['samples']
        lines.append(f'constexpr FClip {cpp_name(kind)}={{"{kind}",{"true" if t["kick"] else "false"},'
                     f'{"true" if t["lead"] else "false"},{t["duration"]}f,{t["impact_fraction"]}f,{t["reach"]}f,{{')
        for i in range(SAMPLES):
            lines.append('    {' + ','.join('{' + ','.join(f'{v:.4f}f' for v in s[k][i]) + '}' for k in vectors)
                         + ',' + ','.join(f'{s[k][i]:.3f}f' for k in angles) + '},')
        lines.append('}};')
    lines.append(f'constexpr const FClip* Clips[]={{{",".join("&" + cpp_name(k) for k in order)}}};')
    lines.append(f'constexpr int ClipCount={len(order)};')
    lines.append('}')
    out.write_text('\n'.join(lines) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('poses', nargs='*', type=Path)
    parser.add_argument('--frames', type=Path, default=None)
    parser.add_argument('--json', type=Path, default=JSON_OUT)
    parser.add_argument('--header', type=Path, default=HEADER_OUT)
    args = parser.parse_args()
    paths = args.poses or sorted(DEFAULT_INPUT.glob('*.pose.npz'))
    if not paths:
        raise SystemExit('no .pose.npz files; run Scripts/track_reference_pose.py first')
    clips = []
    for path in paths:
        try:
            clip = Clip(path)
        except ValueError as error:
            print(f'skipping: {error}', file=sys.stderr)
            continue
        print(f'{clip.name}: {len(clip.world)} frames, {int(clip.cut.sum())} unusable, '
              f'arm {clip.arm[0]:.2f}/{clip.arm[1]:.2f} m, leg {clip.leg[0]:.2f}/{clip.leg[1]:.2f} m')
        clips.append(clip)
    templates = build_templates(clips, args.frames)
    missing = [k for k in STRIKES if k not in templates]
    if missing:
        raise SystemExit(f'no reference events for: {", ".join(missing)}')
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(dict(
        generator='Scripts/extract_strike_motion.py',
        samples=SAMPLES,
        frame='stance frame: x forward, y right, z up; limb positions relative to their root joint in limb lengths; '
              'angles in degrees relative to the guard at the window start',
        clips=[dict(name=c.name, frames=len(c.world), fps=c.fps) for c in clips],
        strikes=templates), indent=1) + '\n')
    write_header(templates, args.header)
    print(f'wrote {args.json} and {args.header}')


if __name__ == '__main__':
    main()
