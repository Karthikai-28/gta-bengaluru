import math
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from sandbox_layout import geometry, load_layout


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


if __name__ == '__main__':
    unittest.main()
