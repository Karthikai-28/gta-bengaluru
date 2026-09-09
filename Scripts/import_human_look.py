"""Unreal: import the original human on the existing, unmodified Manny skeleton."""
from pathlib import Path
import json
import math
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
# A mis-scaled or re-posed skin still imports. Hold it to Manny's rig exactly, so
# the existing animation, IK and physics asset apply to it unchanged.
height=lambda asset: 2*asset.get_imported_bounds().box_extent.z
if abs(height(mesh)-height(reference))>30:
    raise RuntimeError(f'Human is {height(mesh):.0f} cm tall; Manny is {height(reference):.0f} cm')
def reference_pose(asset):
    actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor,unreal.Vector())
    part=actor.skeletal_mesh_component; part.set_skeletal_mesh_asset(asset)
    # Parent-bone space, not component space: a scaled root with metre children
    # matches Manny at rest yet collapses as soon as an animation replaces root.
    pose={str(part.get_bone_name(i)):part.get_bone_transform(part.get_bone_name(i),unreal.RelativeTransformSpace.RTS_PARENT_BONE_SPACE)
          for i in range(part.get_num_bones())}
    actor.destroy_actor(); return pose
ours,theirs=reference_pose(mesh),reference_pose(reference)
if set(ours)!=set(theirs): raise RuntimeError(f'Bone set differs from Manny: {sorted(set(ours)^set(theirs))}')
for name,bone in theirs.items():
    if (bone.translation-ours[name].translation).length()>0.5 or bone.rotation.angular_distance(ours[name].rotation)>math.radians(0.5) \
            or (bone.scale3d-ours[name].scale3d).length()>0.01:
        raise RuntimeError(f'Bind pose of {name} differs from Manny: {ours[name]} vs {bone}')
if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False):
    raise RuntimeError('Could not save human mesh')
for path in unreal.EditorAssetLibrary.list_assets('/Game/NammaCity/Characters/Human',recursive=True):
    material=unreal.load_asset(path)
    if isinstance(material,unreal.Material):
        material.set_editor_property('used_with_skeletal_mesh',True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False):
            raise RuntimeError(f'Could not save {path}')
unreal.log(f'NAMMA_HUMAN_LOOK_OK: {height(mesh):.0f} cm original human skin on the existing animation and physics rig')
