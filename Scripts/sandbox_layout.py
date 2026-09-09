"""Shared metre-based geometry for the Unreal generator and concept diagram."""
import json
import math
import random
from pathlib import Path

DATA_PATH = Path(__file__).parent / "data/player_sandbox.json"


def load_layout():
    return json.loads(DATA_PATH.read_text())


# Bengaluru's avenue trees as (name, crown radius, fork height, crown squash,
# primary limbs) in metres, widest first. The rain tree (Samanea saman) lining the
# older roads is the wide, flat-topped one; gulmohar is narrower and more domed;
# the sapling is a recent planting that still fits a tight footpath.
TREE_PROFILES = [
    ("rain", 7.0, 4.0, 0.42, 6),
    ("gulmohar", 5.4, 3.4, 0.55, 5),
    ("sapling", 3.4, 2.7, 0.72, 4),
]

WALL_M = 59.0   # Boundary wall centreline. A crown may hang a metre past it.
CROWN_ENVELOPE = 1.4  # Furthest foliage, as a multiple of the profile's radius.
# Bearing from a trunk towards its sunlit side. SANDBOX_Sun is spawned with
# unreal.Rotator(-50, -30, 0), which Unreal reads as roll -50, pitch -30, yaw 0, so
# the light travels towards (0.87, 0, -0.5) and every crown is lit from the +X side.
SUN_BEARING = 180.0


def _leaning(base, azimuth, tilt, length):
    """Place a Z-aligned primitive `length` metres long leaning `tilt` degrees off
    vertical towards `azimuth`, and report where its far end lands.

    Unreal composes an instance rotation as pitch about world Y and then yaw about
    world Z, which sends the part's own +Z axis to (-sinP*cosY, -sinP*sinY, cosP);
    a pitch of -tilt with the azimuth as yaw therefore leans it the way we mean.
    Returning the tip too keeps a limb and the foliage it carries on one piece of
    arithmetic rather than two that can drift apart.
    """
    tilt_r, azimuth_r = math.radians(tilt), math.radians(azimuth)
    step = (math.sin(tilt_r) * math.cos(azimuth_r),
            math.sin(tilt_r) * math.sin(azimuth_r), math.cos(tilt_r))
    centre = [base[i] + step[i] * length / 2 for i in range(3)]
    tip = [base[i] + step[i] * length for i in range(3)]
    return centre, tip, -tilt, azimuth


def tree_reach(x, y, data):
    """How far a crown may spread here without growing through a shopfront or off
    the map. Municipal pruning back from a building is what real avenues look like,
    so a cramped corner simply gets a smaller tree."""
    limits = [WALL_M + 1.0 - max(abs(x), abs(y))]
    for building in data["buildings"]:
        bx, by = building["center"]
        width, depth, _height = building["size"]
        limits.append(max(abs(x - bx) - width / 2, abs(y - by) - depth / 2))
    return min(limits)


def tree(x, y, reach, rng):
    """One street tree as (shape, colour, collision, centre, size, pitch, yaw) parts.

    Grown the way a tree is instead of drawn as a ball on a stick: a flared bole
    that tapers and leans, a low fork into primary limbs, near horizontal
    secondaries carrying the crown outwards, and foliage sitting on those limb ends
    in three tones so the canopy has a lit top, a shaded heart and a broken
    outline. Only the bole collides; everything above it is ridden under.
    """
    # An avenue is mostly its largest tree, so the widest profile that still fits
    # is the likeliest; the narrower ones fill in and keep the row from repeating.
    limit = reach / CROWN_ENVELOPE
    fits = [p for weight, p in zip((5, 2, 1), [p for p in TREE_PROFILES if p[1] <= limit])
            for _ in range(weight)] or TREE_PROFILES[-1:]
    _name, full_spread, full_fork, flat, limbs = rng.choice(fits)
    spread = min(full_spread * rng.uniform(0.85, 1.05), limit)
    fork_z = full_fork * (0.86 + 0.14 * spread / full_spread)
    bole_r = 0.15 + 0.05 * spread
    lean, lean_azimuth = rng.uniform(1.5, 5.0), rng.uniform(0, 360)

    # Street trees stand in an opened pit of earth rather than flush in the paving.
    yield ("Cylinder", "gravel", False, [x, y, 0.05], [bole_r * 7, bole_r * 7, 0.10], 0, 0)
    yield ("Cylinder", "bark", True, [x, y, 0.22], [bole_r * 3, bole_r * 3, 0.44], 0, 0)

    # Bole in tapering segments, each leaning a little further than the one below,
    # so the silhouette curves instead of standing like a pipe.
    base, segments = [x, y, 0.30], 3
    for i in range(segments):
        length = (fork_z - 0.30) / segments
        radius = bole_r * (1 - 0.38 * (i + 0.5) / segments)
        centre, base, pitch, yaw = _leaning(base, lean_azimuth, lean * (i + 1) / segments, length)
        yield ("Cylinder", "bark", True, centre, [radius * 2, radius * 2, length], pitch, yaw)

    fork, tips = base, []
    first_azimuth = rng.uniform(0, 360)
    for i in range(limbs):
        azimuth = first_azimuth + 360 * i / limbs + rng.uniform(-15, 15)
        tilt = rng.uniform(34, 52)
        length = spread * rng.uniform(0.52, 0.68)
        radius = bole_r * rng.uniform(0.48, 0.64)
        centre, tip, pitch, yaw = _leaning(fork, azimuth, tilt, length)
        yield ("Cylinder", "bark", False, centre, [radius * 2, radius * 2, length], pitch, yaw)
        # A rain tree's limbs flatten towards horizontal near their ends, and that
        # near level spread is what gives the species its umbrella.
        if rng.random() < 0.75:
            reach_out = spread * rng.uniform(0.28, 0.42)
            centre, tip, pitch, yaw = _leaning(
                tip, azimuth + rng.uniform(-22, 22),
                min(tilt + rng.uniform(16, 32), 82), reach_out)
            yield ("Cylinder", "bark", False, centre,
                   [radius * 1.4, radius * 1.4, reach_out], pitch, yaw)
        tips.append(tip)

    # Foliage: a clump on every limb end, shaded masses filling the heart of the
    # crown, and small lit clumps on top to break the outline against the sky. The
    # clump's own bearing picks its tone, so each crown is bright on the sunward
    # flank and dark on the other instead of being one flat green ball.
    for tip in tips:
        size = spread * rng.uniform(0.62, 0.86)
        bearing = math.degrees(math.atan2(tip[1] - y, tip[0] - x))
        away = abs((bearing - SUN_BEARING + 180) % 360 - 180)
        tone = "leaflight" if away < 60 else "leafdark" if away > 120 else "leaf"
        yield ("Sphere", tone, False,
               [tip[0], tip[1], tip[2] + size * flat * 0.15], [size, size, size * flat], 0, 0)
    crown_z = sum(tip[2] for tip in tips) / len(tips)
    crown_top = max(tip[2] for tip in tips) + spread * flat * 0.45
    for _ in range(2):
        size = spread * rng.uniform(0.95, 1.15)
        yield ("Sphere", "leaf", False,
               [x + rng.uniform(-0.6, 0.6), y + rng.uniform(-0.6, 0.6),
                crown_z + rng.uniform(-0.3, 0.5)], [size, size, size * flat * 0.95], 0, 0)
    for _ in range(3):
        size = spread * rng.uniform(0.40, 0.60)
        azimuth = math.radians(rng.uniform(0, 360))
        radius = spread * rng.uniform(0.1, 0.5)
        yield ("Sphere", "leaflight", False,
               [x + radius * math.cos(azimuth), y + radius * math.sin(azimuth),
                crown_top - size * flat * 0.5], [size, size, size * flat], 0, 0)


def tree_geometry(data):
    """Every street tree in the layout. Kept apart from geometry() so the trees can
    be regrown inside a finished map without rebuilding the street around them."""
    for x, y in data["trees"]:
        yield from tree(x, y, tree_reach(x, y, data), random.Random(f"namma-tree/{x},{y}"))


def geometry(data):
    """Yield (shape, palette key, collision, centre, size, pitch, yaw) batches."""
    def box(color, center, size, collision=True, pitch=0):
        return ("Cube", color, collision, center, size, pitch, 0)

    yield box("ground", [0, 0, -0.5], [120, 120, 1])
    yield box("road", [0, -25, 0.015], [112, 12, 0.03], False)
    yield box("road", [25, 8, 0.015], [12, 66, 0.03], False)
    yield box("paving", [0, -33, 0.10], [112, 4, 0.20])
    yield box("paving", [0, -17, 0.10], [112, 4, 0.20])
    yield box("paving", [33, 8, 0.10], [4, 66, 0.20])
    yield box("paving", [7, -1, 0.10], [28, 28, 0.20])
    for x in range(-50, 53, 8):
        yield box("cream", [x, -25, 0.04], [3, 0.15, 0.02], False)
    for building in data["buildings"]:
        x, y = building["center"]
        w, depth, height = building["size"]
        yield box(building["color"], [x, y, height / 2], [w, depth, height])
        yield box("cream", [x, y, height + 0.15], [w + 0.4, depth + 0.4, 0.3])
        # Doors and windows face the positive Y side of the shops.
        yield box("dark", [x, y + depth / 2 + 0.03, 1.5], [2, 0.08, 3], False)
        yield box("teal", [x, y + depth / 2 + 1.1, 3.2], [w - 1, 2.2, 0.18], False)
        for dx in (-w / 3, w / 3):
            yield box("dark", [x + dx, y + depth / 2 + 0.03, 4.5], [2.1, 0.08, 1.6], False)
    # Boundaries are visible low walls; fall recovery also handles escaped bounds.
    for x in (-59, 59):
        yield box("terracotta", [x, 0, 1], [1, 120, 2])
    for y in (-59, 59):
        yield box("terracotta", [0, y, 1], [120, 1, 2])
    yield from tree_geometry(data)
    # Low practice ramp and landing in Market Court, clear of the delivery route.
    yield box("ochre", [0, 0, 0.7], [5, 3, 0.25], True, 14)
    yield box("terracotta", [3, 0, 0.75], [1.5, 3, 1.5])
    for x, y in [(-9, 19), (9, 19), (18, -5)]:
        yield box("trunk", [x, y, 0.55], [2, 0.7, 0.3])
        for dx in (-0.7, 0.7):
            yield box("dark", [x + dx, y, 0.25], [0.15, 0.6, 0.5])
    # Decorative auto-rickshaw: yellow canopy, green body, three dark wheels.
    yield box("leaf", [-28, -21, 0.75], [2.6, 1.5, 0.8])
    yield box("ochre", [-28.3, -21, 1.8], [2.1, 1.55, 0.25])
    for x, y in [(-29, -21.75), (-29, -20.25), (-26.9, -21)]:
        yield ("Sphere", "dark", True, [x, y, 0.4], [0.65, 0.35, 0.65], 0, 0)
    for start, end in zip(data["route"], data["route"][1:]):
        distance = ((end[0] - start[0]) ** 2 + (end[1] - start[1]) ** 2) ** 0.5
        count = max(1, int(distance / 4))
        for i in range(count):
            t = i / count
            yield box("gold", [start[0] + (end[0] - start[0]) * t,
                               start[1] + (end[1] - start[1]) * t, 0.215], [0.45, 0.45, 0.02], False)


# Surfaces the bicycle movement component recognises by material name. Anything else
# it treats as dry asphalt.
CYCLE_SURFACES = {"gravel", "wet", "paving", "road", "ground"}


def ramp(color, x_low, x_high, y, width, rise, thickness=0.2):
    """A pitched slab climbing from `x_low` at road level to `x_high` at `rise`.

    Unreal pitches the box about its own centre and a positive pitch raises the +X
    end, so both the centre height and the sign of the angle are solved from the two
    end points rather than eyeballed. The low end's top surface lands at z = 0.
    """
    length = abs(x_high - x_low)
    angle = math.atan2(rise, length)
    span = math.hypot(length, rise)
    centre_z = span / 2 * math.sin(angle) - thickness / 2 * math.cos(angle)
    return ("Cube", color, True, [(x_low + x_high) / 2, y, centre_z],
            [span, width, thickness],
            math.copysign(math.degrees(angle), x_high - x_low), 0)


def cycle_features(data):
    """Bengaluru road-surface cases for the pedal cycle, per PHYSICS_CONVENTIONS.

    Added to the finished street by Scripts/setup_cycle.py rather than folded into
    geometry(), so the existing generated map keeps its contract and is not rebuilt.
    """
    def box(color, center, size, collision=True, pitch=0):
        return ("Cube", color, collision, center, size, pitch, 0)

    road_y = -25.0
    # East strip. Painted road/metal cover: the low-grip case section 12 calls out as
    # dangerous for a leaned two-wheeler.
    yield box("wet", [34.0, road_y, 0.035], [3.0, 10.0, 0.05])
    # Two speed breakers, mild and severe.
    yield box("cream", [37.5, road_y, 0.05], [0.6, 11.0, 0.10])
    yield box("cream", [41.0, road_y, 0.08], [0.6, 11.0, 0.16])
    # Resurfaced asphalt either side of an unrepaired hole: the gap is the pothole.
    yield box("paving", [45.1, road_y, 0.06], [2.2, 11.0, 0.12])
    yield box("paving", [48.5, road_y, 0.06], [3.0, 11.0, 0.12])
    # Loose gravel at the far end.
    yield box("gravel", [52.5, road_y, 0.04], [4.0, 11.0, 0.08])

    # West strip: a 6 per cent climb, a flat span and a matching descent, which is the
    # flyover gradient case. Clear of the safehouse and of the walking route.
    yield ramp("paving", -54.0, -46.5, road_y, 8.0, 0.45)   # climb
    yield box("paving", [-43.0, road_y, 0.35], [7.0, 8.0, 0.2])
    yield ramp("paving", -32.0, -39.5, road_y, 8.0, 0.45)   # descent, +X end lower
