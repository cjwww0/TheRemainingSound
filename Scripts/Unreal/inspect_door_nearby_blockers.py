import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
DOOR_LABEL = "HM_BidirectionalDoor6"
RADIUS = 450.0


def _actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _find(label):
    for actor in _actors():
        if _label(actor) == label:
            return actor
    return None


def _vec_text(value):
    return f"{value.x:.3f},{value.y:.3f},{value.z:.3f}"


def _class_name(obj):
    try:
        return obj.get_class().get_name()
    except Exception:
        return obj.__class__.__name__


def _safe_prop(obj, name, default=""):
    try:
        value = obj.get_editor_property(name)
        return str(value)
    except Exception:
        return default


def _component_mesh_name(component):
    for prop in ("static_mesh", "skeletal_mesh_asset", "skeletal_mesh"):
        try:
            mesh = component.get_editor_property(prop)
            if mesh:
                return mesh.get_name()
        except Exception:
            pass
    return ""


def _component_collision(component):
    collision_enabled = _safe_prop(component, "collision_enabled")
    visibility_response = ""
    camera_response = ""
    try:
        visibility_response = str(component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY))
    except Exception:
        pass
    try:
        camera_response = str(component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA))
    except Exception:
        pass
    return collision_enabled, visibility_response, camera_response


def _component_location(component, fallback):
    for method_name in ("get_component_location", "get_world_location"):
        try:
            method = getattr(component, method_name)
        except Exception:
            continue
        try:
            return method()
        except Exception:
            continue
    try:
        return fallback + component.get_editor_property("relative_location")
    except Exception:
        pass
    return fallback


def _component_visible(component):
    try:
        return str(component.is_visible())
    except Exception:
        return ""


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "door6_nearby_blockers.csv")
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=[
            "actor_label",
            "actor_name",
            "actor_class",
            "actor_folder",
            "actor_location",
            "distance",
            "component_name",
            "component_class",
            "component_location",
            "mesh",
            "collision_enabled",
            "visibility_response",
            "camera_response",
            "hidden_actor",
            "hidden_component",
        ])
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[DoorBlockerInspect] Report written: {output_path}")


def main():
    unreal.log(f"[DoorBlockerInspect] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    door = _find(DOOR_LABEL)
    if not door:
        raise RuntimeError(f"Door actor not found: {DOOR_LABEL}")

    door_location = door.get_actor_location()
    rows = []

    for actor in _actors():
        actor_location = actor.get_actor_location()
        distance = (actor_location - door_location).length()
        if distance > RADIUS:
            continue

        components = actor.get_components_by_class(unreal.PrimitiveComponent)
        if not components:
            rows.append({
                "actor_label": _label(actor),
                "actor_name": actor.get_name(),
                "actor_class": _class_name(actor),
                "actor_folder": str(actor.get_folder_path()),
                "actor_location": _vec_text(actor_location),
                "distance": f"{distance:.3f}",
                "component_name": "",
                "component_class": "",
                "component_location": "",
                "mesh": "",
                "collision_enabled": "",
                "visibility_response": "",
                "camera_response": "",
                "hidden_actor": str(actor.is_hidden_ed()),
                "hidden_component": "",
            })
            continue

        for component in components:
            collision_enabled, visibility_response, camera_response = _component_collision(component)
            rows.append({
                "actor_label": _label(actor),
                "actor_name": actor.get_name(),
                "actor_class": _class_name(actor),
                "actor_folder": str(actor.get_folder_path()),
                "actor_location": _vec_text(actor_location),
                "distance": f"{distance:.3f}",
                "component_name": component.get_name(),
                "component_class": _class_name(component),
                "component_location": _vec_text(_component_location(component, actor_location)),
                "mesh": _component_mesh_name(component),
                "collision_enabled": collision_enabled,
                "visibility_response": visibility_response,
                "camera_response": camera_response,
                "hidden_actor": str(actor.is_hidden_ed()),
                "hidden_component": _component_visible(component),
            })

    rows.sort(key=lambda row: float(row["distance"]))
    _write_report(rows)
    unreal.log(f"[DoorBlockerInspect] Wrote {len(rows)} nearby primitive rows around {DOOR_LABEL}")


if __name__ == "__main__":
    main()
