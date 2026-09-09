"""Run in Unreal after building. Regrows the sandbox's street trees in place.

The generator refuses to rebuild a finished map, so the trees are replaced inside
it the same additive, label idempotent way Scripts/setup_cycle.py adds the cycle.
Re-running this after editing sandbox_layout.py regrows them from the new shapes.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sandbox_layout import load_layout, tree_geometry
# The generator's flat sandbox material, rather than a third copy of the recipe.
from setup_cycle import material
import unreal

MAP_PATH = "/Game/NammaCity/Maps/L_PlayerSandbox"
MATERIAL_ROOT = "/Game/NammaCity/Materials/Sandbox"
TREE_PREFIX = "TREE_"
# The lollipop trees the first generator pass left behind: one cylinder trunk and
# one leaf sphere each, and nothing else in the map used those two batches.
LEGACY_BATCHES = ["SANDBOX_Cylinder_trunk_True", "SANDBOX_Sphere_leaf_False"]


def clear_old_trees(actors, expected):
    """Remove the previous trees, refusing to touch a batch that is not one.

    A stale batch holding an unexpected number of instances means the map has
    diverged from this layout, and deleting it would quietly take scenery with it.
    """
    removed = 0
    for actor in list(actors.get_all_level_actors()):
        label = actor.get_actor_label()
        if label in LEGACY_BATCHES:
            component = actor.get_editor_property("instances")
            count = component.get_instance_count()
            if count != expected:
                raise RuntimeError(
                    f"{label} holds {count} instances, not the {expected} street trees. "
                    "Inspect the map before regrowing the trees.")
        elif not label.startswith(TREE_PREFIX):
            continue
        actors.destroy_actor(actor)
        removed += 1
    return removed


def main():
    prop_cls = unreal.load_class(None, "/Script/NammaCity.NammaInstancedProp")
    if not prop_cls:
        raise RuntimeError("Build NammaCityEditor before growing the trees")
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        raise RuntimeError("Generate the sandbox with Scripts/create_player_sandbox.sh first")
    if not unreal.EditorLevelLibrary.load_level(MAP_PATH):
        raise RuntimeError("Could not open the sandbox map")

    data = load_layout()
    parts = list(tree_geometry(data))
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    removed = clear_old_trees(actors, len(data["trees"]))

    batches = {}
    for shape, color, collision, center, size, pitch, yaw in parts:
        key = (shape, color, collision)
        if key not in batches:
            actor = actors.spawn_actor_from_class(prop_cls, unreal.Vector(0, 0, 0))
            if actor is None:
                raise RuntimeError(f"Failed to spawn {TREE_PREFIX}{shape}_{color}_{collision}")
            actor.set_actor_label(f"{TREE_PREFIX}{shape}_{color}_{collision}")
            component = actor.get_editor_property("instances")
            component.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{shape}.{shape}"))
            mat = material(color, data["palette"][color])
            # Bark and the three canopy greens are new, so cook the instanced
            # permutation the way the cycle's shared materials are cooked.
            if not mat.get_editor_property("used_with_instanced_static_meshes"):
                mat.set_editor_property("used_with_instanced_static_meshes", True)
                unreal.MaterialEditingLibrary.recompile_material(mat)
                if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
                    raise RuntimeError(f"Could not save M_Sandbox_{color}")
            component.set_material(0, mat)
            component.set_collision_enabled(
                unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision
                else unreal.CollisionEnabled.NO_COLLISION)
            batches[key] = component
        # Unreal's Python Rotator is (roll, pitch, yaw), so name them; passing
        # pitch and yaw in order silently rolls every limb instead of aiming it.
        batches[key].add_instance(unreal.Transform(
            location=unreal.Vector(*(v * 100 for v in center)),
            rotation=unreal.Rotator(pitch=pitch, yaw=yaw, roll=0),
            scale=unreal.Vector(*size)))

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save the sandbox map")
    unreal.log(f"NAMMA_TREES_SETUP_OK: {len(data['trees'])} trees, {len(parts)} parts in "
               f"{len(batches)} batches, {removed} old batches removed")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        unreal.log_error(traceback.format_exc())
        raise
