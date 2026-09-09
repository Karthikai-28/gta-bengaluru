"""Unreal: export bind skeleton for the original human's production skin weights."""
from pathlib import Path
import unreal

root=Path(__file__).resolve().parents[1]
output=root/'Saved/HumanLook/Manny.fbx'
output.parent.mkdir(parents=True,exist_ok=True)
mesh=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
if not mesh: raise RuntimeError('Run setup_human_character first')
task=unreal.AssetExportTask()
task.object=mesh
task.filename=str(output)
task.automated=True
task.prompt=False
task.replace_identical=True
task.exporter=unreal.SkeletalMeshExporterFBX()
task.options=unreal.FbxExportOption()
task.options.set_editor_property('ascii',False)
if not unreal.Exporter.run_asset_export_task(task) or not output.is_file():
    raise RuntimeError('Could not export Manny reference skeleton')
unreal.log('NAMMA_HUMAN_REFERENCE_OK')
