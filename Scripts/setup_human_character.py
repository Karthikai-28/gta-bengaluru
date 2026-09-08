"""Run in Unreal after building. Create our animation BP from Epic's bundled rig."""
import unreal

SOURCE = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
TARGET = "/Game/NammaCity/Characters/ABP_NammaHuman"
parent = unreal.load_class(None, "/Script/NammaCity.NammaHumanAnimInstance")
if not parent:
    raise RuntimeError("Build NammaCityEditor before preparing the human rig")
blueprint = unreal.load_asset(TARGET) if unreal.EditorAssetLibrary.does_asset_exist(TARGET) else None
if not blueprint:
    blueprint = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET)
if not blueprint:
    raise RuntimeError("Import the bundled Characters template resources first")
unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
    raise RuntimeError("Could not save the human animation blueprint")
mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
physics = unreal.load_asset("/Game/Characters/Mannequins/Rigs/PA_Mannequin")
assert mesh and physics
unreal.log("NAMMA_HUMAN_SETUP_OK: skeletal mesh, animation blueprint and physics asset loaded")

# Add a small repeatable physics exercise without rebuilding the existing street.
map_path = "/Game/NammaCity/Maps/L_PlayerSandbox"
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    if not unreal.EditorLevelLibrary.load_level(map_path):
        raise RuntimeError("Could not open sandbox for physics props")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    labels = {a.get_actor_label() for a in actors.get_all_level_actors()}
    for label, y, mass in [("HUMAN_Liftable_12kg", -3100, 12.0), ("HUMAN_Heavy_60kg", -2950, 60.0)]:
        if label in labels:
            continue
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-4400, y, 55))
        actor.set_actor_label(label)
        part = actor.static_mesh_component
        part.set_mobility(unreal.ComponentMobility.MOVABLE)
        part.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
        part.set_world_scale3d(unreal.Vector(0.6, 0.6, 0.6))
        part.set_collision_profile_name("PhysicsActor")
        part.set_mass_override_in_kg("None", mass, True)
        part.set_linear_damping(0.2)
        part.set_angular_damping(0.5)
        part.set_simulate_physics(True)
        part.set_material(0, unreal.load_asset("/Game/NammaCity/Materials/Sandbox/M_Sandbox_ochre"))
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save physics exercise")
