import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FLASHLIGHT_LABEL = "TRS_PlayerFlashlight"
DOOR_LABEL = "HM_BidirectionalDoor6"


def _actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _find(label):
    matches = [actor for actor in _actors() if _label(actor) == label]
    return matches[0] if matches else None


def _vec_text(value):
    return f"{value.x:.3f},{value.y:.3f},{value.z:.3f}"


def _set_if_present(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def _find_component_by_name(actor, component_name):
    try:
        for component in actor.get_components_by_class(unreal.ActorComponent):
            if component and component.get_name() == component_name:
                return component
    except Exception:
        pass
    return None


def _ensure_player_flashlight(rows):
    actor = _find(FLASHLIGHT_LABEL)
    if not actor:
        flashlight_class = unreal.load_class(None, "/Script/HorrorMechanics.RemainPlayerFlashlightActor")
        if not flashlight_class:
            rows.append({
                "target": FLASHLIGHT_LABEL,
                "status": "FAIL",
                "message": "Cannot load /Script/HorrorMechanics.RemainPlayerFlashlightActor. Compile C++ first.",
                "location": "",
            })
            return
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(flashlight_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
        actor.set_actor_label(FLASHLIGHT_LABEL, mark_dirty=True)

    actor.modify()
    _set_if_present(actor, "b_start_enabled", True)
    _set_if_present(actor, "b_bind_toggle_input", True)
    _set_if_present(actor, "toggle_action_name", unreal.Name("FlashlightToggle"))
    _set_if_present(actor, "intensity", 50000.0)
    _set_if_present(actor, "attenuation_radius", 3600.0)
    _set_if_present(actor, "inner_cone_angle", 8.0)
    _set_if_present(actor, "outer_cone_angle", 30.0)
    _set_if_present(actor, "volumetric_scattering_intensity", 8.0)
    _set_if_present(actor, "source_radius", 1.5)
    _set_if_present(actor, "soft_source_radius", 24.0)
    _set_if_present(actor, "b_use_temperature", True)
    _set_if_present(actor, "temperature", 4300.0)
    _set_if_present(actor, "camera_relative_location", unreal.Vector(10.0, 0.0, -4.0))
    _set_if_present(actor, "camera_relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
    try:
        actor.set_folder_path("Gameplay_Logic/00_Player")
    except Exception:
        pass

    rows.append({
        "target": FLASHLIGHT_LABEL,
        "status": "PASS",
        "message": "Player flashlight actor ensured",
        "location": _vec_text(actor.get_actor_location()),
    })


def _ensure_door_proxy(rows):
    door = _find(DOOR_LABEL)
    if not door:
        rows.append({
            "target": DOOR_LABEL,
            "status": "FAIL",
            "message": "Door actor not found",
            "location": "",
        })
        return

    setup_library = getattr(unreal, "RemainInteractionSetupLibrary", None)
    if not setup_library:
        rows.append({
            "target": DOOR_LABEL,
            "status": "FAIL",
            "message": "RemainInteractionSetupLibrary unavailable. Compile C++ first.",
            "location": _vec_text(door.get_actor_location()),
        })
        return

    component = setup_library.ensure_interaction_trace_proxy(
        door,
        unreal.Vector(0.0, 0.0, 125.0),
        unreal.Vector(155.0, 95.0, 155.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

    rows.append({
        "target": DOOR_LABEL,
        "status": "PASS" if component else "FAIL",
        "message": "Door Visibility trace proxy ensured" if component else "Failed to add proxy component",
        "location": _vec_text(door.get_actor_location()),
    })

    door_visual = _find_component_by_name(door, "Door")
    if door_visual:
        try:
            door_visual.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
            rows.append({
                "target": f"{DOOR_LABEL}.Door",
                "status": "PASS",
                "message": "Door visual collision set to QueryAndPhysics",
                "location": _vec_text(door_visual.get_world_location()),
            })
        except Exception as exc:
            rows.append({
                "target": f"{DOOR_LABEL}.Door",
                "status": "FAIL",
                "message": f"Failed to set collision: {exc}",
                "location": _vec_text(door.get_actor_location()),
            })
    else:
        rows.append({
            "target": f"{DOOR_LABEL}.Door",
            "status": "FAIL",
            "message": "Door visual component not found",
            "location": _vec_text(door.get_actor_location()),
        })

    fallback = setup_library.ensure_door_interaction_fallback(
        door,
        unreal.Name("Interact"),
        450.0,
        True,
        True,
    )
    if fallback:
        _set_if_present(fallback, "b_drive_door_visual", True)
        _set_if_present(fallback, "door_visual_component_name", unreal.Name("Door"))
        _set_if_present(fallback, "open_relative_rotation_offset", unreal.Rotator(0.0, 90.0, 0.0))
        _set_if_present(fallback, "b_disable_door_visual_collision_when_open", True)

    rows.append({
        "target": f"{DOOR_LABEL}.DoorFallback",
        "status": "PASS" if fallback else "FAIL",
        "message": "Door interaction fallback ensured" if fallback else "Failed to add door fallback component",
        "location": _vec_text(door.get_actor_location()),
    })


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "player_flashlight_and_door_proxy_setup.csv")
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["target", "status", "message", "location"])
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[FlashlightDoorSetup] Report written: {output_path}")


def main():
    unreal.log(f"[FlashlightDoorSetup] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    rows = []
    _ensure_player_flashlight(rows)
    _ensure_door_proxy(rows)
    _write_report(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[FlashlightDoorSetup] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
