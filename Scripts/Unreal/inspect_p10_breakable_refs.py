import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
CONTROLLER_LABEL = "BP_PanelPickupShockController"


def _get_all_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name() if actor else ""


def _class_path(obj):
    try:
        return obj.get_class().get_path_name()
    except Exception:
        return ""


def _vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def _find_actor(label):
    for actor in _get_all_actors():
        if _label(actor) == label:
            return actor
    return None


def _get_prop(obj, *names):
    if not obj:
        return None
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def _find_component_by_class_name(actor, class_name_part):
    try:
        for component in actor.get_components_by_class(unreal.ActorComponent):
            if class_name_part in _class_path(component):
                return component
    except Exception:
        pass
    return None


def _actor_row(category, index, actor, note=""):
    if not actor:
        return {
            "category": category,
            "index": index,
            "label": "",
            "class_path": "",
            "location": "",
            "hidden": "",
            "collision": "",
            "folder": "",
            "note": note or "NULL",
        }
    try:
        hidden = actor.is_hidden_ed() or actor.is_temporarily_hidden_in_editor()
    except Exception:
        hidden = ""
    try:
        collision = actor.get_actor_enable_collision()
    except Exception:
        collision = ""
    try:
        folder = str(actor.get_folder_path())
    except Exception:
        folder = ""
    return {
        "category": category,
        "index": index,
        "label": _label(actor),
        "class_path": _class_path(actor),
        "location": _vec_text(actor.get_actor_location()),
        "hidden": str(hidden),
        "collision": str(collision),
        "folder": folder,
        "note": note,
    }


def _value_row(category, index, value, note=""):
    return {
        "category": category,
        "index": index,
        "label": str(value),
        "class_path": _class_path(value) if value else "",
        "location": "",
        "hidden": "",
        "collision": "",
        "folder": "",
        "note": note,
    }


def _component_row(category, index, component, note=""):
    if not component:
        return _actor_row(category, index, None, note or "NULL")
    try:
        mesh = component.get_editor_property("static_mesh")
    except Exception:
        mesh = None
    try:
        visible = component.is_visible()
    except Exception:
        visible = ""
    try:
        hidden_game = component.bHiddenInGame
    except Exception:
        hidden_game = ""
    try:
        simulate = component.is_simulating_physics()
    except Exception:
        simulate = ""
    try:
        location = component.get_world_location()
    except Exception:
        try:
            location = component.get_owner().get_actor_location()
        except Exception:
            location = unreal.Vector()
    try:
        scale = component.get_world_scale()
    except Exception:
        try:
            scale = component.get_editor_property("relative_scale3d")
        except Exception:
            scale = ""
    return {
        "category": category,
        "index": index,
        "label": component.get_name(),
        "class_path": _class_path(component),
        "location": _vec_text(location),
        "hidden": f"visible={visible};hidden_game={hidden_game};simulate={simulate}",
        "collision": str(component.get_collision_enabled()),
        "folder": "",
        "note": f"mesh={mesh}; scale={scale}; {note}",
    }


def _append_actor_array(rows, category, actors):
    if actors is None:
        rows.append(_actor_row(category, "", None, "property unreadable"))
        return
    if len(actors) == 0:
        rows.append(_actor_row(category, "", None, "empty"))
        return
    for index, actor in enumerate(actors):
        rows.append(_actor_row(category, index, actor))


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p10_breakable_refs.csv")
    fieldnames = ["category", "index", "label", "class_path", "location", "hidden", "collision", "folder", "note"]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P10Inspect] Report written: {output_path}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    controller = _find_actor(CONTROLLER_LABEL)
    if not controller:
        raise RuntimeError(f"Cannot find {CONTROLLER_LABEL}")

    rows = [_actor_row("controller", 0, controller)]
    rows.append(_actor_row("trigger_pickup_actor", 0, _get_prop(controller, "trigger_pickup_actor", "TriggerPickupActor")))
    rows.append(_actor_row("nurse_encounter", 0, _get_prop(controller, "nurse_encounter", "NurseEncounter")))
    _append_actor_array(rows, "lights_to_extinguish", _get_prop(controller, "lights_to_extinguish", "LightsToExtinguish"))
    _append_actor_array(rows, "ambient_audio_actors", _get_prop(controller, "ambient_audio_actors", "AmbientAudioActors"))

    breakable = _find_component_by_class_name(controller, "RemainBreakableSwapComponent")
    if not breakable:
        rows.append(_actor_row("breakable_component", 0, None, "RemainBreakableSwapComponent missing"))
    else:
        rows.append({
            "category": "breakable_component",
            "index": 0,
            "label": breakable.get_name(),
            "class_path": _class_path(breakable),
            "location": "",
            "hidden": "",
            "collision": "",
            "folder": "",
            "note": "found",
        })
        rows.append(_value_row("runtime_intact_enabled", 0, _get_prop(breakable, "spawn_runtime_intact_on_begin_play", "b_spawn_runtime_intact_on_begin_play", "bSpawnRuntimeIntactOnBeginPlay")))
        rows.append(_value_row("runtime_intact_mesh", 0, _get_prop(breakable, "runtime_intact_mesh", "RuntimeIntactMesh")))
        rows.append(_value_row("runtime_intact_relative_location", 0, _get_prop(breakable, "runtime_intact_relative_location", "RuntimeIntactRelativeLocation")))
        rows.append(_value_row("runtime_intact_relative_rotation", 0, _get_prop(breakable, "runtime_intact_relative_rotation", "RuntimeIntactRelativeRotation")))
        rows.append(_value_row("runtime_intact_scale", 0, _get_prop(breakable, "runtime_intact_scale", "RuntimeIntactScale")))
        rows.append(_value_row("runtime_spawn_enabled", 0, _get_prop(breakable, "spawn_runtime_fragments_on_break", "b_spawn_runtime_fragments_on_break", "bSpawnRuntimeFragmentsOnBreak")))
        rows.append(_value_row("runtime_fragment_mesh", 0, _get_prop(breakable, "runtime_fragment_mesh", "RuntimeFragmentMesh")))
        rows.append(_value_row("runtime_fragment_count", 0, _get_prop(breakable, "runtime_fragment_count", "RuntimeFragmentCount")))
        rows.append(_value_row("runtime_fragment_spread_radius", 0, _get_prop(breakable, "runtime_fragment_spread_radius", "RuntimeFragmentSpreadRadius")))
        intact_actors = _get_prop(breakable, "intact_actors", "IntactActors")
        _append_actor_array(rows, "intact_actors", intact_actors)
        if intact_actors:
            for index, intact_actor in enumerate(intact_actors):
                if intact_actor:
                    components = intact_actor.get_components_by_class(unreal.StaticMeshComponent)
                    if components:
                        rows.append(_component_row("intact_static_mesh_component", index, components[0]))
        _append_actor_array(rows, "broken_actors", _get_prop(breakable, "broken_actors", "BrokenActors"))

    _write_report(rows)


if __name__ == "__main__":
    main()
