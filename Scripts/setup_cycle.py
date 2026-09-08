"""Run in Unreal after building. Adds the rideable cycle and its road-surface test
strip to the finished street without regenerating it, in the same additive, label
idempotent way Scripts/setup_human_character.py adds the physics crates."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sandbox_layout import cycle_features, load_layout
import unreal

MAP_PATH = "/Game/NammaCity/Maps/L_PlayerSandbox"
MATERIAL_ROOT = "/Game/NammaCity/Materials/Sandbox"
CYCLE_LABEL = "CYCLE_VEH_01_002"
STRIP_PREFIX = "CYCLE_Surface_"


def material(name, rgb):
    """Reuses the generator's flat sandbox material, creating it only if missing."""
    path = f"{MATERIAL_ROOT}/M_Sandbox_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        f"M_Sandbox_{name}", MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if asset is None:
        raise RuntimeError(f"Failed to create {path}")
    color = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(*rgb))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    fill = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant3Vector)
    fill.set_editor_property("constant", unreal.LinearColor(*(v * 0.12 for v in rgb)))
    unreal.MaterialEditingLibrary.connect_material_property(fill, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant)
    # Wet paint and metal covers are the low-grip case, so they read as polished.
    roughness.set_editor_property("r", 0.15 if name == "wet" else 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save {path}")
    return asset


def main():
    bicycle_cls = unreal.load_class(None, "/Script/NammaCity.NammaBicycle")
    prop_cls = unreal.load_class(None, "/Script/NammaCity.NammaInstancedProp")
    if not bicycle_cls or not prop_cls:
        raise RuntimeError("Build NammaCityEditor before adding the cycle")
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        raise RuntimeError("Generate the sandbox with Scripts/create_player_sandbox.sh first")
    if not unreal.EditorLevelLibrary.load_level(MAP_PATH):
        raise RuntimeError("Could not open the sandbox map")

    data = load_layout()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    labels = {a.get_actor_label() for a in actors.get_all_level_actors()}

    if CYCLE_LABEL not in labels:
        cycle = actors.spawn_actor_from_class(
            bicycle_cls, unreal.Vector(*(v * 100 for v in data["cycle"])),
            unreal.Rotator(0, data["cycle_yaw"], 0))
        if cycle is None:
            raise RuntimeError("Failed to spawn the cycle")
        cycle.set_actor_label(CYCLE_LABEL)
        unreal.log(f"Placed {CYCLE_LABEL} outside CYCLE REPAIRS at {data['cycle']}")

    # One instanced actor per (shape, colour, collision), so a surface is a single
    # tagged actor the wheel traces can classify.
    batches = {}
    for shape, color, collision, center, size, pitch in cycle_features(data):
        key = (shape, color, collision)
        label = f"{STRIP_PREFIX}{shape}_{color}_{collision}"
        if label in labels:
            continue
        if key not in batches:
            actor = actors.spawn_actor_from_class(prop_cls, unreal.Vector(0, 0, 0))
            if actor is None:
                raise RuntimeError(f"Failed to spawn {label}")
            actor.set_actor_label(label)
            actor.tags = [unreal.Name(f"NammaSurface.{color}")]
            component = actor.get_editor_property("instances")
            component.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{shape}.{shape}"))
            component.set_material(0, material(color, data["palette"][color]))
            component.set_collision_enabled(
                unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision
                else unreal.CollisionEnabled.NO_COLLISION)
            batches[key] = component
        batches[key].add_instance(unreal.Transform(
            location=unreal.Vector(*(v * 100 for v in center)),
            rotation=unreal.Rotator(pitch, 0, 0), scale=unreal.Vector(*size)))

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save the sandbox map")
    unreal.log(f"NAMMA_CYCLE_SETUP_OK: cycle placed, {len(batches)} surface batches added")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        unreal.log_error(traceback.format_exc())
        raise
