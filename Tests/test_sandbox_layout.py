import math
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from sandbox_layout import CYCLE_SURFACES, cycle_features, geometry, load_layout, ramp


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
        for shape, color, collision, center, size, pitch in props:
            self.assertIn(color, data['palette'])
            self.assertTrue(all(math.isfinite(n) for n in center + size + [pitch]))
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
        for shape, color, _collision, center, size, pitch in self.features:
            self.assertEqual(shape, 'Cube')
            self.assertIn(color, self.data['palette'])
            self.assertTrue(all(math.isfinite(n) for n in center + size + [pitch]))
            self.assertTrue(all(n > 0 for n in size))
            self.assertLess(max(abs(center[0]), abs(center[1])), 58)

    def test_features_sit_on_the_road_and_miss_the_walking_route(self):
        for _shape, _color, _collision, center, size, _pitch in self.features:
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
        named = {color for _s, color, _c, _ce, _si, _p in self.features}
        self.assertTrue(named & CYCLE_SURFACES, 'no feature uses a recognised surface')
        for color in ('gravel', 'wet'):
            self.assertIn(color, named)
            self.assertIn(color, self.data['palette'])

    def test_speed_breakers_are_rideable_and_the_pothole_is_a_real_gap(self):
        heights = [size[2] for _s, color, _c, _ce, size, _p in self.features if color == 'cream']
        self.assertTrue(heights, 'no speed breakers')
        # Taller than the ground clearance would stop a 129 mm cycle dead.
        self.assertTrue(all(0.05 <= h <= 0.2 for h in heights), heights)
        patches = sorted((center[0] - size[0] / 2, center[0] + size[0] / 2)
                         for _s, color, _c, center, size, _p in self.features
                         if color == 'paving' and abs(center[0] - 46) < 6)
        self.assertEqual(len(patches), 2)
        gap = patches[1][0] - patches[0][1]
        self.assertTrue(0.3 < gap < 1.2, f'pothole gap of {gap} m is not wheel sized')

    def test_ramp_low_end_meets_the_road(self):
        for x_low, x_high, rise in [(-54.0, -46.5, 0.45), (-32.0, -39.5, 0.45), (0.0, 10.0, 1.0)]:
            _shape, _color, _collision, center, size, pitch = ramp('paving', x_low, x_high, 0, 4, rise)
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


if __name__ == '__main__':
    unittest.main()
