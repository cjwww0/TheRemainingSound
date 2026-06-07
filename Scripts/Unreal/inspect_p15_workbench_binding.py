import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
WORKBENCH_LABEL = "BP_WorkbenchPanel_PuzzleActor"
CONTROLLER_LABEL = "BP_WorkshopLightFaultController"


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
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


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


def _actor_row(category, actor, message=""):
    return {
        "category": category,
        "label": _label(actor) if actor else "",
        "class_path": _class_path(actor) if actor else "",
        "location": _vec_text(actor.get_actor_location()) if actor else "",
        "valid": "true" if actor else "false",
        "message": message,
    }


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p15_workbench_binding.csv")
    fieldnames = ["category", "label", "class_path", "location", "valid", "message"]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P15BindingInspect] Report written: {output_path}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    workbench = _find_actor(WORKBENCH_LABEL)
    controller = _find_actor(CONTROLLER_LABEL)
    assigned_controller = _get_prop(workbench, "light_fault_controller", "LightFaultController")

    rows = [
        _actor_row("workbench", workbench),
        _actor_row("expected_controller", controller),
        _actor_row("workbench_light_fault_controller", assigned_controller),
    ]

    if workbench and controller and assigned_controller == controller:
        rows.append(_actor_row("binding_result", assigned_controller, "OK: Workbench LightFaultController points to expected controller"))
    elif workbench and assigned_controller:
        rows.append(_actor_row("binding_result", assigned_controller, "WARN: Workbench points to a different controller actor"))
    else:
        rows.append(_actor_row("binding_result", None, "FAIL: Workbench LightFaultController is empty or unreadable"))

    _write_report(rows)


if __name__ == "__main__":
    main()
