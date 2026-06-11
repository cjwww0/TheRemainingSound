import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
CONTROLLER_LABEL = "BP_PanelPickupShockController"
INTACT_LABEL = "SM_GlassBottle"
OLD_CHAOS_LABEL = "GC_SM_GlassBottle_Test"
FRAGMENT_LABEL_PREFIX = "P10_BottleFragment_"
FRAGMENT_COUNT = 6
TARGET_FOLDER = "Gameplay_Logic/02_P10_P11"
FRAGMENT_MESH_PATH = "/Game/NotKnowWhereToDeposit/TriggerBottle/SM_BottleFragment.SM_BottleFragment"
INTACT_MESH_PATH = "/Game/Fab/Glass_dirt/scene/Sm/SM_GlassBottle.SM_GlassBottle"


def _actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name() if actor else ""


def _find_actor(label):
    for actor in _actors():
        if _label(actor) == label:
            return actor
    return None


def _set_label(actor, label):
    try:
        actor.set_actor_label(label, mark_dirty=True)
    except TypeError:
        actor.set_actor_label(label)


def _set_folder(actor, folder):
    try:
        actor.set_folder_path(folder)
    except Exception:
        actor.set_folder_path(unreal.Name(folder))


def _set_if_present(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def _find_component_by_class_name(actor, class_name_part):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        try:
            if class_name_part in component.get_class().get_path_name():
                return component
        except Exception:
            pass
    return None


def _static_mesh_component(actor):
    components = actor.get_components_by_class(unreal.StaticMeshComponent)
    return components[0] if components else None


def _set_breakable_enabled(actor, enabled):
    if not actor:
        return
    actor.set_actor_hidden_in_game(not enabled)
    actor.set_actor_enable_collision(enabled)
    actor.set_actor_tick_enabled(enabled)
    _set_if_present(actor, "hidden", not enabled)
    for component in actor.get_components_by_class(unreal.PrimitiveComponent):
        _set_if_present(component, "visible", enabled)
        _set_if_present(component, "hidden_in_game", not enabled)
        component.set_visibility(enabled, True)
        component.set_hidden_in_game(not enabled, True)
        component.set_collision_enabled(
            unreal.CollisionEnabled.QUERY_AND_PHYSICS if enabled else unreal.CollisionEnabled.NO_COLLISION
        )
        try:
            component.set_simulate_physics(False)
        except Exception:
            pass


def _fragment_transform(base_location, index):
    offsets = [
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Vector(10.0, -8.0, 2.0),
        unreal.Vector(-12.0, 8.0, 1.0),
        unreal.Vector(8.0, 14.0, 3.0),
        unreal.Vector(-10.0, -14.0, 2.0),
        unreal.Vector(16.0, 2.0, 4.0),
    ]
    rotations = [
        unreal.Rotator(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 32.0, 18.0),
        unreal.Rotator(0.0, -41.0, -12.0),
        unreal.Rotator(14.0, 80.0, 5.0),
        unreal.Rotator(-10.0, -96.0, 22.0),
        unreal.Rotator(18.0, 145.0, -16.0),
    ]
    return base_location + offsets[index % len(offsets)], rotations[index % len(rotations)]


def _ensure_fragment_actor(label, location, rotation, mesh):
    actor = _find_actor(label)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor:
        actor = subsystem.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
        _set_label(actor, label)
    else:
        actor.set_actor_location(location, False, True)
        actor.set_actor_rotation(rotation, False)

    _set_folder(actor, TARGET_FOLDER)
    component = _static_mesh_component(actor)
    if component and mesh:
        component.set_static_mesh(mesh)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_simulate_physics(False)
    _set_breakable_enabled(actor, False)
    return actor


def _disable_legacy_fragment_actor(label, hidden_location):
    actor = _find_actor(label)
    if not actor:
        return None
    actor.set_actor_location(hidden_location, False, True)
    _set_folder(actor, TARGET_FOLDER)
    _set_breakable_enabled(actor, False)
    return actor


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p10_stable_breakable_setup.csv")
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["label", "status", "note"])
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P10StableBreakable] Report written: {output_path}")


def main():
    rows = []
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    controller = _find_actor(CONTROLLER_LABEL)
    intact_actor = _find_actor(INTACT_LABEL)
    if not controller:
        raise RuntimeError(f"Cannot find {CONTROLLER_LABEL}")
    if not intact_actor:
        raise RuntimeError(f"Cannot find {INTACT_LABEL}")

    fragment_mesh = unreal.EditorAssetLibrary.load_asset(FRAGMENT_MESH_PATH)
    if not fragment_mesh:
        raise RuntimeError(f"Cannot load fragment mesh: {FRAGMENT_MESH_PATH}")
    intact_mesh = unreal.EditorAssetLibrary.load_asset(INTACT_MESH_PATH)
    if not intact_mesh:
        raise RuntimeError(f"Cannot load intact mesh: {INTACT_MESH_PATH}")

    base_location = intact_actor.get_actor_location()
    hidden_location = base_location + unreal.Vector(0.0, 0.0, -10000.0)
    for index in range(FRAGMENT_COUNT):
        label = f"{FRAGMENT_LABEL_PREFIX}{index + 1:02d}"
        fragment = _disable_legacy_fragment_actor(label, hidden_location + unreal.Vector(index * 20.0, 0.0, 0.0))
        if fragment:
            rows.append({"label": label, "status": "disabled", "note": "legacy pre-placed fragment hidden; runtime fragments spawn on break"})

    old_chaos = _find_actor(OLD_CHAOS_LABEL)
    if old_chaos:
        _set_breakable_enabled(old_chaos, False)
        _set_folder(old_chaos, TARGET_FOLDER)
        rows.append({"label": OLD_CHAOS_LABEL, "status": "disabled", "note": "left in map as disabled Chaos reference"})

    breakable = _find_component_by_class_name(controller, "RemainBreakableSwapComponent")
    if not breakable:
        raise RuntimeError("BreakableCup component missing")

    _set_if_present(breakable, "intact_actors", [intact_actor])
    _set_if_present(breakable, "broken_actors", [])
    _set_if_present(breakable, "b_prepare_broken_actors_on_begin_play", True)
    _set_if_present(breakable, "b_disable_intact_actors_on_break", True)
    _set_if_present(breakable, "spawn_runtime_intact_on_begin_play", True)
    _set_if_present(breakable, "b_spawn_runtime_intact_on_begin_play", True)
    _set_if_present(breakable, "runtime_intact_mesh", intact_mesh)
    _set_if_present(breakable, "runtime_intact_relative_location", unreal.Vector(8.0, 52.0, 35.0))
    _set_if_present(breakable, "runtime_intact_relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
    _set_if_present(breakable, "runtime_intact_scale", unreal.Vector(0.5, 0.5, 0.5))
    _set_if_present(breakable, "hide_configured_intact_actors_when_runtime_intact", True)
    _set_if_present(breakable, "b_hide_configured_intact_actors_when_runtime_intact", True)
    _set_if_present(breakable, "spawn_runtime_fragments_on_break", True)
    _set_if_present(breakable, "b_spawn_runtime_fragments_on_break", True)
    _set_if_present(breakable, "runtime_fragment_mesh", fragment_mesh)
    _set_if_present(breakable, "runtime_fragment_count", 6)
    _set_if_present(breakable, "runtime_fragment_spread_radius", 18.0)
    _set_if_present(breakable, "runtime_fragment_scale", unreal.Vector(0.35, 0.35, 0.35))
    _set_if_present(breakable, "b_use_chaos_geometry_collections", False)
    _set_if_present(breakable, "b_use_single_visible_chaos_actor", False)
    _set_if_present(breakable, "b_enable_physics_on_broken_actors", True)
    _set_if_present(breakable, "b_force_broken_actors_movable", True)
    _set_if_present(breakable, "b_force_broken_collision_block_all", False)
    _set_if_present(breakable, "b_apply_radial_impulse", True)
    _set_if_present(breakable, "impulse_radius", 260.0)
    _set_if_present(breakable, "impulse_strength", 850.0)
    _set_if_present(breakable, "b_impulse_vel_change", True)
    _set_if_present(breakable, "b_apply_directional_launch_impulse", True)
    _set_if_present(breakable, "directional_launch_impulse", unreal.Vector(1.0, -0.35, 0.45))
    _set_if_present(breakable, "directional_launch_impulse_strength", 700.0)
    _set_if_present(breakable, "b_debug_breakable", False)

    _set_breakable_enabled(intact_actor, False)
    _set_folder(intact_actor, TARGET_FOLDER)
    rows.append({"label": INTACT_LABEL, "status": "disabled", "note": "legacy map cup hidden; BreakableCup spawns runtime intact cup"})
    rows.append({"label": CONTROLLER_LABEL, "status": "configured", "note": "BreakableCup set to runtime intact cup + runtime non-Chaos fragments"})

    _write_report(rows)
    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[P10StableBreakable] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
