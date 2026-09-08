"""Run in the compiled Unreal Editor. Existing maps are preserved unless explicitly regenerated."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sandbox_layout import geometry, load_layout
import unreal

MAP_PATH = "/Game/NammaCity/Maps/L_PlayerSandbox"
GENERATOR_TAG = "NammaSandboxGenerator"
GENERATOR_VERSION = "1"
MATERIAL_ROOT = "/Game/NammaCity/Materials/Sandbox"


def spawn(cls, xyz, label, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls, unreal.Vector(*xyz), rotation or unreal.Rotator())
    if actor is None:
        raise RuntimeError(f"Failed to spawn {label}")
    actor.set_actor_label(label)
    return actor


def material(name, rgb):
    path = f"{MATERIAL_ROOT}/M_Sandbox_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        f"M_Sandbox_{name}", MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if asset is None:
        raise RuntimeError(f"Failed to create {path}")
    color = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(*rgb))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Small constant fill keeps primitive art readable at low scalability without baked GI.
    fill = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant3Vector)
    fill.set_editor_property("constant", unreal.LinearColor(*(v * 0.12 for v in rgb)))
    unreal.MaterialEditingLibrary.connect_material_property(fill, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant)
    roughness.set_editor_property("r", 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save {path}")
    return asset


def write_completion():
    # Wrapper removes this before each run. An existing partial .umap is not success.
    marker = Path(__file__).resolve().parents[1] / "Saved/SandboxGeneration.complete"
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(GENERATOR_VERSION + "\n")


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(MAP_PATH)
        if unreal.EditorAssetLibrary.get_metadata_tag(existing, GENERATOR_TAG) != GENERATOR_VERSION:
            raise RuntimeError("Existing sandbox is incomplete or from another generator version. Inspect it and explicitly delete the generated map before retrying.")
        unreal.log(f"Preserving completed map: {MAP_PATH}. Delete it explicitly in the editor to regenerate.")
        write_completion()
        return
    prop_cls = unreal.load_class(None, "/Script/NammaCity.NammaInstancedProp")
    station_cls = unreal.load_class(None, "/Script/NammaCity.NammaDeliveryStation")
    if not prop_cls or not station_cls:
        raise RuntimeError("Build NammaCityEditor before generating the sandbox")
    data = load_layout()
    mats = {name: material(name, rgb) for name, rgb in data["palette"].items()}
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create sandbox level")
    batches = {}
    for shape, color, collision, center, size, pitch in geometry(data):
        key = (shape, color, collision)
        if key not in batches:
            actor = spawn(prop_cls, (0, 0, 0), f"SANDBOX_{shape}_{color}_{collision}")
            component = actor.get_editor_property("instances")
            component.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{shape}.{shape}"))
            component.set_material(0, mats[color])
            component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
            batches[key] = component
        transform = unreal.Transform(location=unreal.Vector(*(v * 100 for v in center)),
                                     rotation=unreal.Rotator(pitch, 0, 0).quaternion(), scale=unreal.Vector(*size))
        batches[key].add_instance(transform)
    spawn(unreal.PlayerStart, [v * 100 for v in data["spawn"]], "SANDBOX_Start", unreal.Rotator(0, data["spawn_yaw"], 0))
    for pickup, key in [(True, "pickup"), (False, "delivery")]:
        station = spawn(station_cls, [v * 100 for v in data[key]], f"SANDBOX_{key}", unreal.Rotator(0, 180, 0))
        station.configure_station(pickup)
        station.get_component_by_class(unreal.StaticMeshComponent).set_material(0, mats["gold" if pickup else "teal"])
        # Setting a property through Python does not reliably rerun construction scripts.
        label = station.get_component_by_class(unreal.TextRenderComponent)
        label.set_text("NAMMA TEA | PICKUP" if pickup else "CORNER STORES | DELIVERY")
    for building in data["buildings"]:
        x, y = building["center"]
        _, depth, _ = building["size"]
        sign = spawn(unreal.TextRenderActor, (x * 100, (y + depth / 2 + 0.2) * 100, 360),
                     "SIGN_" + building["name"], unreal.Rotator(0, 90, 0))
        text = sign.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(building["name"])
        text.set_world_size(48)
        text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    sun = spawn(unreal.DirectionalLight, (0, 0, 1500), "SANDBOX_Sun", unreal.Rotator(-50, -30, 0))
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity", 3.0)
    light.set_editor_property("light_color", unreal.Color(255, 230, 195))
    sky = spawn(unreal.SkyLight, (0, 0, 1000), "SANDBOX_Sky")
    sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    spawn(unreal.SkyAtmosphere, (0, 0, 0), "SANDBOX_Atmosphere")
    light.set_editor_property("atmosphere_sun_light", True)
    world = unreal.EditorLevelLibrary.get_editor_world()
    unreal.EditorAssetLibrary.set_metadata_tag(world, GENERATOR_TAG, GENERATOR_VERSION)
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save sandbox map")
    write_completion()
    unreal.log(f"Created {MAP_PATH}: {len(batches)} instanced geometry batches")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        unreal.log_error(traceback.format_exc())
        raise
