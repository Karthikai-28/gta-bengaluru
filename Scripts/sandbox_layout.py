"""Shared metre-based geometry for the Unreal generator and concept diagram."""
import json
from pathlib import Path

DATA_PATH = Path(__file__).parent / "data/player_sandbox.json"


def load_layout():
    return json.loads(DATA_PATH.read_text())


def geometry(data):
    """Yield (shape, palette key, collision, centre, size, pitch) batches."""
    def box(color, center, size, collision=True, pitch=0):
        return ("Cube", color, collision, center, size, pitch)

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
    for x, y in data["trees"]:
        yield ("Cylinder", "trunk", True, [x, y, 2], [0.6, 0.6, 4], 0)
        yield ("Sphere", "leaf", False, [x, y, 5.1], [5, 5, 5], 0)
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
        yield ("Sphere", "dark", True, [x, y, 0.4], [0.65, 0.35, 0.65], 0)
    for start, end in zip(data["route"], data["route"][1:]):
        distance = ((end[0] - start[0]) ** 2 + (end[1] - start[1]) ** 2) ** 0.5
        count = max(1, int(distance / 4))
        for i in range(count):
            t = i / count
            yield box("gold", [start[0] + (end[0] - start[0]) * t,
                               start[1] + (end[1] - start[1]) * t, 0.215], [0.45, 0.45, 0.02], False)
