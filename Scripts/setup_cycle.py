"""Run in Unreal after building. Adds the rideable cycle and its road-surface test
strip to the finished street without regenerating it, in the same additive, label
idempotent way Scripts/setup_human_character.py adds the physics crates."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sandbox_layout import cycle_features, load_layout, starter_cycle_pose
import unreal
from sandbox_materials import ensure_instanced, repair_sandbox_materials

MAP_PATH = "/Game/NammaCity/Maps/L_PlayerSandbox"
MATERIAL_ROOT = "/Game/NammaCity/Materials/Sandbox"
CYCLE_LABEL = "CYCLE_VEH_01_002"
STRIP_PREFIX = "CYCLE_Surface_"


def material(name, rgb):
    """Reuses the generator's flat sandbox material, creating it only if missing."""
    path = f"{MATERIAL_ROOT}/M_Sandbox_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return ensure_instanced(unreal.load_asset(path))
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
    # Every sandbox prop is drawn through an instanced mesh component, so without
    # this usage flag the material is silently swapped for the default checker.
    asset.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save {path}")
    return asset


def prepare_cycle_materials():
    path = "/Game/NammaCity/Materials/Cycle/M_CyclePaint"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        paint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_CyclePaint", "/Game/NammaCity/Materials/Cycle", unreal.Material, unreal.MaterialFactoryNew())
        color = unreal.MaterialEditingLibrary.create_material_expression(paint, unreal.MaterialExpressionVectorParameter)
        color.set_editor_property("parameter_name", "FrameColor")
        color.set_editor_property("default_value", unreal.LinearColor(0.05, 0.4, 0.6))
        unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        for value, prop in [(0.25, unreal.MaterialProperty.MP_METALLIC), (0.32, unreal.MaterialProperty.MP_ROUGHNESS)]:
            scalar = unreal.MaterialEditingLibrary.create_material_expression(paint, unreal.MaterialExpressionConstant)
            scalar.set_editor_property("r", value)
            unreal.MaterialEditingLibrary.connect_material_property(scalar, "", prop)
        unreal.MaterialEditingLibrary.recompile_material(paint)
        assert unreal.EditorAssetLibrary.save_loaded_asset(paint)
    # Repair all existing palette assets, including gold, even on repeated setup.
    for name, rgb in load_layout()["palette"].items():
        ensure_instanced(material(name, rgb))


def main():
    repair_sandbox_materials()
    prepare_cycle_materials()
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
    # Older saved native component layouts contain orphaned wheel/paint entries.
    # Recreate each cycle once using the current class, preserving placement and color.
    layout_tag = unreal.Name("NammaCycle.Components.v2")
    for old in list(actors.get_all_level_actors()):
        if old.get_class() != bicycle_cls or layout_tag in old.tags:
            continue
        transform, label = old.get_actor_transform(), old.get_actor_label()
        seed, tags = old.get_editor_property("color_seed"), list(old.tags)
        replacement = actors.spawn_actor_from_class(bicycle_cls, old.get_actor_location(), old.get_actor_rotation())
        if replacement is None:
            raise RuntimeError(f"Failed to refresh {label}")
        replacement.set_actor_transform(transform, False, True)
        replacement.set_editor_property("color_seed", seed)
        replacement.tags = tags + [layout_tag]
        if not actors.destroy_actor(old):
            actors.destroy_actor(replacement)
            raise RuntimeError(f"Could not replace old cycle {label}")
        replacement.set_actor_label(label)
    labels = {a.get_actor_label() for a in actors.get_all_level_actors()}

    if CYCLE_LABEL not in labels:
        cycle = actors.spawn_actor_from_class(
            bicycle_cls, unreal.Vector(*(v * 100 for v in data["cycle"])),
            unreal.Rotator(pitch=0, yaw=data["cycle_yaw"], roll=0))
        if cycle is None:
            raise RuntimeError("Failed to spawn the cycle")
        cycle.set_actor_label(CYCLE_LABEL)
        cycle.tags = [layout_tag]
        unreal.log(f"Placed {CYCLE_LABEL} outside CYCLE REPAIRS at {data['cycle']}")

    # This generated starter follows the current spawn even when the map changes.
    starter = next((a for a in actors.get_all_level_actors()
                    if a.get_actor_label() == "CYCLE_Start_Rideable"), None)
    location, yaw = starter_cycle_pose(data)
    if starter is None:
        starter = actors.spawn_actor_from_class(bicycle_cls, unreal.Vector(*(v * 100 for v in location)),
            unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    if not starter:
        raise RuntimeError("Failed to place starter cycle")
    starter.set_actor_label("CYCLE_Start_Rideable")
    starter.set_actor_location(unreal.Vector(*(v * 100 for v in location)), False, True)
    starter.set_actor_rotation(unreal.Rotator(pitch=0, yaw=yaw, roll=0), True)
    starter.tags = list(set(list(starter.tags) + [layout_tag, unreal.Name("NammaCycle.Starter")]))
    unreal.log(f"Starter cycle updated to {location}; walk forward/left from spawn and press E")

    # One instanced actor per (shape, colour, collision), so a surface is a single
    # tagged actor the wheel traces can classify.
    batches = {}
    for shape, color, collision, center, size, pitch, yaw in cycle_features(data):
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
        # Unreal's Python Rotator is (roll, pitch, yaw), so name them; passing
        # pitch and yaw in order silently rolls the instance instead.
        batches[key].add_instance(unreal.Transform(
            location=unreal.Vector(*(v * 100 for v in center)),
            rotation=unreal.Rotator(pitch=pitch, yaw=yaw, roll=0),
            scale=unreal.Vector(*size)))

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
