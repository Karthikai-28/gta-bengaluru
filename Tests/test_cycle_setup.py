"""Regressions for saved material repair and starter placement; no Unreal needed."""
import importlib.util
import math
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import Mock, patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from sandbox_layout import load_layout, starter_cycle_pose

class CycleSetupTests(unittest.TestCase):
    def test_starter_clear_and_visible_from_current_spawn(self):
        data=load_layout(); position,yaw=starter_cycle_pose(data); x,y,z=position
        self.assertLess(math.dist(position[:2],data['spawn'][:2]),5)
        self.assertGreater(z,.65)
        for b in data['buildings']:
            bx,by=b['center'];w,d,_=b['size']
            self.assertFalse(abs(x-bx)<w/2+1 and abs(y-by)<d/2+1)
        for prop in [(-44,-31),(-44,-29.5)]:
            self.assertGreater(math.dist(position[:2],prop),1.5)
        self.assertGreater(abs(y-data['route'][0][1]),1)
        self.assertEqual(yaw,data['spawn_yaw'])
        # Both wheel contact patches remain on the same 20 cm footpath, clear
        # of its road-side kerb. A bike across this edge starts tilted/unloaded.
        for axle_y in [y,y]:
            self.assertGreater(axle_y,-35+.175)
            self.assertLess(axle_y,-31-.175)

    def test_starter_rotates_with_spawn(self):
        data=load_layout(); data['spawn']=[0,0,1.1];data['spawn_yaw']=90
        pos,yaw=starter_cycle_pose(data)
        self.assertAlmostEqual(pos[0],1.5);self.assertAlmostEqual(pos[1],4)
        self.assertEqual(yaw,90)

    def test_existing_material_usage_is_saved_even_when_already_set(self):
        unreal=types.SimpleNamespace(MaterialEditingLibrary=Mock(),EditorAssetLibrary=Mock())
        unreal.EditorAssetLibrary.save_loaded_asset.return_value=True
        with patch.dict(sys.modules,{'unreal':unreal}):
            spec=importlib.util.spec_from_file_location('material_repair_test',ROOT/'Scripts/sandbox_materials.py')
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            for flag in [False,True]:
                mat=Mock();mat.get_editor_property.return_value=flag
                self.assertIs(module.ensure_instanced(mat),mat)
                unreal.EditorAssetLibrary.save_loaded_asset.assert_called_with(mat,only_if_is_dirty=False)
                if not flag: mat.set_editor_property.assert_called_with('used_with_instanced_static_meshes',True)
            unreal.EditorAssetLibrary.save_loaded_asset.return_value=False
            with self.assertRaises(RuntimeError):module.ensure_instanced(Mock())
