"""Unreal: import the original human on the existing, unmodified Manny skeleton."""
from pathlib import Path
import json
import unreal

root=Path(__file__).resolve().parents[1]
source=root/'Art/Characters/Production'
manifest=json.loads((source/'manifest.json').read_text())
if not manifest['production_skeleton']:
    raise RuntimeError('Preview skeleton cannot replace the production character')
reference=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
options=unreal.FbxImportUI()
options.set_editor_property('automated_import_should_detect_type',False)
options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.set_editor_property('import_as_skeletal',True)
options.set_editor_property('import_mesh',True)
options.set_editor_property('import_animations',False)
options.set_editor_property('import_materials',True)
options.set_editor_property('import_textures',False)
options.set_editor_property('skeleton',reference.get_editor_property('skeleton'))
options.set_editor_property('create_physics_asset',False)
options.set_editor_property('physics_asset',unreal.load_asset('/Game/Characters/Mannequins/Rigs/PA_Mannequin'))
options.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False)
options.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
task=unreal.AssetImportTask()
task.filename=str(source/'SK_NammaMan.fbx')
task.destination_path='/Game/NammaCity/Characters/Human'
task.destination_name='SK_NammaMan'
task.automated=True
task.replace_existing=True
task.save=True
task.options=options
task.factory=unreal.FbxFactory()
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh=unreal.load_asset('/Game/NammaCity/Characters/Human/SK_NammaMan')
if not isinstance(mesh,unreal.SkeletalMesh): raise RuntimeError('Human skeletal import failed')
if mesh.get_editor_property('skeleton') != reference.get_editor_property('skeleton'):
    raise RuntimeError('Human import did not retain Manny skeleton')
if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False):
    raise RuntimeError('Could not save human mesh')
for path in unreal.EditorAssetLibrary.list_assets('/Game/NammaCity/Characters/Human',recursive=True):
    material=unreal.load_asset(path)
    if isinstance(material,unreal.Material):
        material.set_editor_property('used_with_skeletal_mesh',True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False):
            raise RuntimeError(f'Could not save {path}')
unreal.log('NAMMA_HUMAN_LOOK_OK: original human skin uses existing animation and physics rig')
