import math
import random
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from sandbox_layout import (CROWN_ENVELOPE, CYCLE_SURFACES, TREE_PROFILES, _leaning,
                            cycle_features, geometry, load_layout, ramp, tree, tree_reach)


class SandboxLayoutTests(unittest.TestCase):
    def test_route_is_walkable_around_buildings(self):
        data = load_layout()
        for start, end in zip(data['route'], data['route'][1:]):
            count = math.ceil(math.dist(start, end) / 0.25)
            for step in range(count + 1):
                x, y = [a + (b-a)*step/count for a,b in zip(start, end)]
                self.assertLess(max(abs(x), abs(y)), 58)
                for building in data['buildings']:
                    bx, by = building['center']; w, d, _ = building['size']
                    self.assertFalse(abs(x-bx) < w/2 + .4 and abs(y-by) < d/2 + .4,
                                     f"Route intersects {building['name']} at {(x, y)}")

    def test_objectives_are_on_route(self):
        data = load_layout()
        self.assertEqual(data['spawn'][:2], data['route'][0])
        self.assertIn(data['pickup'][:2], data['route'])
        self.assertEqual(data['delivery'][:2], data['route'][-1])

    def test_geometry_is_finite_and_batched(self):
        data = load_layout(); props = list(geometry(data))
        self.assertEqual(props, list(geometry(data)))
        batches = set()
        for shape, color, collision, center, size, pitch, yaw in props:
            self.assertIn(color, data['palette'])
            self.assertTrue(all(math.isfinite(n) for n in center + size + [pitch, yaw]))
            self.assertTrue(all(n > 0 for n in size))
            batches.add((shape, color, collision))
        self.assertLess(len(batches), 40)
        self.assertLess(len(props), 400)


class CycleLayoutTests(unittest.TestCase):
    """The cycle and its road-surface strip are added to the finished street, so they
    have to stay clear of the delivery route and inside the block."""

    def setUp(self):
        self.data = load_layout()
        self.features = list(cycle_features(self.data))

    def test_features_are_finite_and_on_the_palette(self):
        self.assertEqual(self.features, list(cycle_features(self.data)))
        for shape, color, _collision, center, size, pitch, yaw in self.features:
            self.assertEqual(shape, 'Cube')
            self.assertIn(color, self.data['palette'])
            self.assertTrue(all(math.isfinite(n) for n in center + size + [pitch, yaw]))
            self.assertTrue(all(n > 0 for n in size))
            self.assertEqual(yaw, 0)
            self.assertLess(max(abs(center[0]), abs(center[1])), 58)

    def test_features_sit_on_the_road_and_miss_the_walking_route(self):
        for _shape, _color, _collision, center, size, _pitch, _yaw in self.features:
            # The main road runs along y = -25 and is 12 m wide.
            self.assertLess(abs(center[1] + 25), 1.0, f'feature at {center} is off the road')
            self.assertLessEqual(size[1] / 2, 6.0)
            for start, end in zip(self.data['route'], self.data['route'][1:]):
                steps = math.ceil(math.dist(start, end) / 0.25)
                for step in range(steps + 1):
                    x, y = [a + (b - a) * step / steps for a, b in zip(start, end)]
                    self.assertFalse(abs(x - center[0]) < size[0] / 2 and abs(y - center[1]) < size[1] / 2,
                                     f'feature at {center} blocks the walking route at {(x, y)}')

    def test_surfaces_are_ones_the_movement_component_recognises(self):
        named = {color for _s, color, _c, _ce, _si, _p, _y in self.features}
        self.assertTrue(named & CYCLE_SURFACES, 'no feature uses a recognised surface')
        for color in ('gravel', 'wet'):
            self.assertIn(color, named)
            self.assertIn(color, self.data['palette'])

    def test_speed_breakers_are_rideable_and_the_pothole_is_a_real_gap(self):
        heights = [size[2] for _s, color, _c, _ce, size, _p, _y in self.features if color == 'cream']
        self.assertTrue(heights, 'no speed breakers')
        # Taller than the ground clearance would stop a 129 mm cycle dead.
        self.assertTrue(all(0.05 <= h <= 0.2 for h in heights), heights)
        patches = sorted((center[0] - size[0] / 2, center[0] + size[0] / 2)
                         for _s, color, _c, center, size, _p, _y in self.features
                         if color == 'paving' and abs(center[0] - 46) < 6)
        self.assertEqual(len(patches), 2)
        gap = patches[1][0] - patches[0][1]
        self.assertTrue(0.3 < gap < 1.2, f'pothole gap of {gap} m is not wheel sized')

    def test_ramp_low_end_meets_the_road(self):
        for x_low, x_high, rise in [(-54.0, -46.5, 0.45), (-32.0, -39.5, 0.45), (0.0, 10.0, 1.0)]:
            _shape, _color, _collision, center, size, pitch, _yaw = ramp('paving', x_low, x_high, 0, 4, rise)
            angle = math.radians(pitch)
            half = size[0] / 2
            # Top surface of each end, with the box pitched about its own centre.
            low = center[2] - half * abs(math.sin(angle)) + size[2] / 2 * math.cos(angle)
            high = center[2] + half * abs(math.sin(angle)) + size[2] / 2 * math.cos(angle)
            self.assertAlmostEqual(low, 0.0, places=9)
            self.assertAlmostEqual(high, rise, places=9)
            self.assertAlmostEqual(math.copysign(1, pitch), math.copysign(1, x_high - x_low))

    def test_cycle_starts_clear_of_buildings_and_the_route(self):
        x, y, z = self.data['cycle']
        self.assertGreater(z, 0.2)
        for building in self.data['buildings']:
            bx, by = building['center']
            w, d, _h = building['size']
            self.assertFalse(abs(x - bx) < w / 2 + 0.9 and abs(y - by) < d / 2 + 0.9,
                             f"cycle starts inside {building['name']}")
        for start, end in zip(self.data['route'], self.data['route'][1:]):
            steps = math.ceil(math.dist(start, end) / 0.25)
            for step in range(steps + 1):
                point = [a + (b - a) * step / steps for a, b in zip(start, end)]
                self.assertGreater(math.dist(point, [x, y]), 1.0,
                                   'cycle is parked on the walking route')
        # Close enough to CYCLE REPAIRS to read as belonging to it.
        shop = next(b for b in self.data['buildings'] if b['name'] == 'CYCLE REPAIRS')
        self.assertLess(math.dist(shop['center'], [x, y]), 20)


class TreeTests(unittest.TestCase):
    """The street trees are grown from branch geometry rather than placed as a ball
    on a stick, so the arithmetic that aims a limb and the arithmetic that hangs
    foliage on its end have to agree, and the crown has to stay out of the way."""

    def setUp(self):
        self.data = load_layout()
        self.trees = {tuple(t): list(tree(t[0], t[1], tree_reach(t[0], t[1], self.data),
                                          random.Random(f'namma-tree/{t[0]},{t[1]}')))
                      for t in self.data['trees']}

    def test_leaning_matches_the_rotation_unreal_will_apply(self):
        """_leaning predicts where a limb's far end lands. If it disagrees with the
        engine's own pitch/yaw composition the foliage floats off the branches."""
        for tilt in (0, 12, 45, 82):
            for azimuth in (0, 37, 150, 271, 359):
                base = [1.5, -2.5, 3.0]
                centre, tip, pitch, yaw = _leaning(base, azimuth, tilt, 4.0)
                # FRotationTranslationMatrix with zero roll sends the part's own +Z
                # axis to this row; see Engine/Source/.../RotationTranslationMatrix.h.
                sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
                sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
                axis = (-sp * cy, -sp * sy, cp)
                for i in range(3):
                    self.assertAlmostEqual(tip[i], base[i] + axis[i] * 4.0, places=9)
                    self.assertAlmostEqual(centre[i], (base[i] + tip[i]) / 2, places=9)
                self.assertAlmostEqual(math.dist(base, tip), 4.0, places=9)

    def test_every_tree_has_a_bole_limbs_and_a_lit_and_shaded_crown(self):
        for position, parts in self.trees.items():
            kinds = {(shape, color, collision) for shape, color, collision, *_ in parts}
            self.assertIn(('Cylinder', 'bark', True), kinds, f'{position} has no bole')
            self.assertIn(('Cylinder', 'bark', False), kinds, f'{position} has no limbs')
            tones = {color for _s, color, *_ in parts if color.startswith('leaf')}
            self.assertGreaterEqual(len(tones), 2, f'{position} has a single flat crown: {tones}')
            leaning = [yaw for _s, _c, _co, _ce, _si, pitch, yaw in parts if pitch]
            self.assertGreater(len(set(leaning)), 2, f'{position} radiates nothing')

    def test_only_the_bole_collides_and_the_crown_clears_a_rider(self):
        for position, parts in self.trees.items():
            for _shape, color, collision, centre, size, _p, _y in parts:
                if collision:
                    # A bole is climbable-height at most; nothing else may block.
                    self.assertEqual(color, 'bark', f'{position} collides with {color}')
                    self.assertLess(centre[2] + size[2] / 2, 5.0)
                if color.startswith('leaf'):
                    self.assertGreater(centre[2] - size[2] / 2, 2.5,
                                       f'{position} has foliage in a rider\'s face')

    def test_crowns_miss_the_buildings_and_stay_on_the_map(self):
        for (x, y), parts in self.trees.items():
            reach = tree_reach(x, y, self.data)
            for _shape, color, _collision, centre, size, _p, _y in parts:
                if not color.startswith('leaf'):
                    continue
                out = math.dist((x, y), centre[:2]) + size[0] / 2
                self.assertLessEqual(out, reach + 1e-9,
                                     f'crown at {(x, y)} reaches {out:.2f} m past its {reach:.2f} m clearance')
                self.assertLess(max(abs(centre[0]), abs(centre[1])), 60)

    def test_crown_envelope_bounds_every_profile(self):
        """tree() sizes a crown by dividing its clearance by CROWN_ENVELOPE, so the
        constant has to be an upper bound on how far foliage actually gets."""
        for name, spread, *_rest in TREE_PROFILES:
            worst = 0.0
            for seed in range(200):
                parts = list(tree(0, 0, spread * CROWN_ENVELOPE, random.Random(seed)))
                worst = max([worst] + [math.dist((0, 0), c[:2]) + s[0] / 2
                                       for _sh, col, _co, c, s, _p, _y in parts
                                       if col.startswith('leaf')])
            self.assertLessEqual(worst, spread * CROWN_ENVELOPE,
                                 f'{name} foliage reaches {worst:.2f} m beyond the envelope')

    def test_trees_are_deterministic_and_stay_off_the_walking_route(self):
        for (x, y), parts in self.trees.items():
            self.assertEqual(parts, list(tree(x, y, tree_reach(x, y, self.data),
                                              random.Random(f'namma-tree/{x},{y}'))))
            solid = max(size[0] / 2 for _s, _c, collision, _ce, size, _p, _y in parts if collision)
            for start, end in zip(self.data['route'], self.data['route'][1:]):
                steps = math.ceil(math.dist(start, end) / 0.25)
                for step in range(steps + 1):
                    point = [a + (b - a) * step / steps for a, b in zip(start, end)]
                    self.assertGreater(math.dist(point, [x, y]), solid + 0.6,
                                       f'the bole at {(x, y)} stands on the walking route')



if __name__ == '__main__':
    unittest.main()
