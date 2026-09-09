"""Repair and persist shader usage on reused sandbox assets (Unreal Python)."""
import unreal


def ensure_instanced(material):
    if not material:
        raise RuntimeError("Cannot repair a missing material")
    if not material.get_editor_property("used_with_instanced_static_meshes"):
        material.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(material)
    # Force-save even if automatic usage detection already set the in-memory flag.
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Could not persist instancing usage: {material.get_path_name()}")
    return material


def repair_sandbox_materials():
    for path in unreal.EditorAssetLibrary.list_assets("/Game/NammaCity/Materials/Sandbox", recursive=True):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.Material):
            ensure_instanced(asset)
    unreal.log("NAMMA_SANDBOX_MATERIALS_OK: instancing usage saved")
