"""Create the deterministic Phase 0 smoke-test level inside Unreal Editor."""

import unreal


MAP_PATH = "/Game/NammaCity/Maps/L_SmokeTest"


def spawn(actor_class, location, rotation, label):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        unreal.Vector(*location),
        unreal.Rotator(*rotation),
    )
    if actor is None:
        raise RuntimeError(f"Could not spawn {label}")
    actor.set_actor_label(label)
    return actor


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log(f"Smoke-test map already exists: {MAP_PATH}")
        return

    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create map: {MAP_PATH}")

    floor = spawn(unreal.StaticMeshActor, (0, 0, -50), (0, 0, 0), "SMOKE_Floor")
    floor_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor.static_mesh_component.set_static_mesh(floor_mesh)
    floor.set_actor_scale3d(unreal.Vector(20, 20, 1))

    spawn(unreal.PlayerStart, (0, 0, 150), (0, 0, 0), "SMOKE_PlayerStart")
    sun = spawn(unreal.DirectionalLight, (0, 0, 500), (-45, -30, 0), "SMOKE_Sun")
    sun.light_component.set_editor_property("intensity", 5.0)
    spawn(unreal.SkyLight, (0, 0, 300), (0, 0, 0), "SMOKE_SkyLight")

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save the smoke-test map")
    unreal.log(f"Created smoke-test map: {MAP_PATH}")


main()
