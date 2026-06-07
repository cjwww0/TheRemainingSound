import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_LABEL = "FinalHouse"
PLACEMENT_TAG = "MainRoutePlaced_20260607"

EXPECTED_ACTORS = {
    "Player Start2": {
        "folder": "Gameplay_Logic/00_Player",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_Document16": {
        "folder": "Gameplay_Logic/01_Opening",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_Document14": {
        "folder": "Gameplay_Logic/01_Opening",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_Document11": {
        "folder": "Gameplay_Logic/01_Opening",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_InventoryItem": {
        "folder": "Gameplay_Logic/01_Opening",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_PanelPickupShockController": {
        "folder": "Gameplay_Logic/02_P10_P11",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_NurseEncounter_01": {
        "folder": "Gameplay_Logic/02_P10_P11",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPanel_PuzzleActor": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_WorkshopLightFaultController": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPickup_01": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPickup_02": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPickup_03": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPickup_04": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_WorkbenchPickup_05": {
        "folder": "Gameplay_Logic/03_Workbench",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_CheckpointTrigger": {
        "folder": "Gameplay_Logic/04_Gates_Checkpoints",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_SaveMachine": {
        "folder": "Gameplay_Logic/04_Gates_Checkpoints",
        "bounds": "second_floor",
        "required_tag": True,
    },
    "BP_Keypad_Puzzle": {
        "folder": "Gameplay_Logic/04_Gates_Checkpoints",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "BP_CombinationLock_PuzzleActor": {
        "folder": "Gameplay_Logic/04_Gates_Checkpoints",
        "bounds": "first_floor",
        "required_tag": True,
    },
    "HM_BidirectionalDoor6": {
        "folder": "Gameplay_Logic/04_Gates_Checkpoints",
        "bounds": "first_floor",
        "required_tag": True,
    },
}

EXPECTED_WORKBENCH_PART_IDS = ["Part01", "Part02", "Part03", "Part04", "Part05"]
EXPECTED_WORKBENCH_SLOT_IDS = ["Panel01", "Panel02", "Panel03", "Panel04", "Panel05"]


def _get_all_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _folder(actor):
    try:
        return str(actor.get_folder_path())
    except Exception:
        return ""


def _class_path(obj):
    try:
        return obj.get_class().get_path_name()
    except Exception:
        return ""


def _vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def _has_tag(actor, tag):
    return tag in [str(existing) for existing in actor.tags]


def _find_actor(label):
    matches = [actor for actor in _get_all_actors() if _label(actor) == label]
    return matches[0] if matches else None


def _get_prop(obj, *names):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def _name_text(value):
    if value is None:
        return ""
    text = str(value)
    if text == "None":
        return ""
    return text


def _bounds_for(finalhouse_origin, bounds_name):
    # Intentionally padded rough world bounds around the currently placed FinalHouse instance.
    if bounds_name == "first_floor":
        return {
            "x_min": finalhouse_origin.x + 950.0,
            "x_max": finalhouse_origin.x + 1850.0,
            "y_min": finalhouse_origin.y - 420.0,
            "y_max": finalhouse_origin.y + 650.0,
            "z_min": finalhouse_origin.z + 900.0,
            "z_max": finalhouse_origin.z + 1160.0,
        }
    if bounds_name == "second_floor":
        return {
            "x_min": finalhouse_origin.x - 420.0,
            "x_max": finalhouse_origin.x + 850.0,
            "y_min": finalhouse_origin.y - 120.0,
            "y_max": finalhouse_origin.y + 1050.0,
            "z_min": finalhouse_origin.z + 1550.0,
            "z_max": finalhouse_origin.z + 1740.0,
        }
    return None


def _is_in_bounds(location, bounds):
    if not bounds:
        return True
    return (
        bounds["x_min"] <= location.x <= bounds["x_max"]
        and bounds["y_min"] <= location.y <= bounds["y_max"]
        and bounds["z_min"] <= location.z <= bounds["z_max"]
    )


def _actor_has_primitive_component(actor):
    try:
        for component in actor.get_components_by_class(unreal.PrimitiveComponent):
            if component:
                return True
    except Exception:
        pass
    return False


def _component_names(actor):
    names = []
    try:
        for component in actor.get_components_by_class(unreal.ActorComponent):
            names.append(component.get_name())
    except Exception:
        pass
    return names


def _find_component_by_class_name(actor, class_name_part):
    try:
        for component in actor.get_components_by_class(unreal.ActorComponent):
            if class_name_part in _class_path(component):
                return component
    except Exception:
        pass
    return None


def _append_actor_result(rows, label, status, severity, message, actor=None):
    rows.append({
        "category": "actor",
        "label": label,
        "status": status,
        "severity": severity,
        "message": message,
        "folder": _folder(actor) if actor else "",
        "location": _vec_text(actor.get_actor_location()) if actor else "",
        "class_path": _class_path(actor) if actor else "",
    })


def _validate_expected_actors(rows, finalhouse_origin):
    for label, expected in EXPECTED_ACTORS.items():
        actor = _find_actor(label)
        if not actor:
            _append_actor_result(rows, label, "FAIL", "error", "Actor missing")
            continue

        expected_folder = expected["folder"]
        actual_folder = _folder(actor)
        if actual_folder != expected_folder:
            _append_actor_result(rows, label, "FAIL", "error", f"Folder mismatch: expected {expected_folder}, got {actual_folder}", actor)
        else:
            _append_actor_result(rows, label, "PASS", "info", "Folder OK", actor)

        if expected.get("required_tag") and not _has_tag(actor, PLACEMENT_TAG):
            _append_actor_result(rows, label, "WARN", "warning", f"Missing tag {PLACEMENT_TAG}", actor)

        bounds = _bounds_for(finalhouse_origin, expected.get("bounds"))
        if bounds and not _is_in_bounds(actor.get_actor_location(), bounds):
            _append_actor_result(rows, label, "WARN", "warning", f"Outside rough {expected.get('bounds')} bounds", actor)
        else:
            _append_actor_result(rows, label, "PASS", "info", f"Rough {expected.get('bounds')} bounds OK", actor)

        if not _actor_has_primitive_component(actor) and label != "Player Start2":
            _append_actor_result(rows, label, "WARN", "warning", "No PrimitiveComponent found; raycast/collision may need manual check", actor)


def _validate_workbench(rows):
    label = "BP_WorkbenchPanel_PuzzleActor"
    actor = _find_actor(label)
    if not actor:
        return

    component = _find_component_by_class_name(actor, "RemainWorkbenchPuzzleComponent")
    if not component:
        _append_actor_result(rows, label, "FAIL", "error", "RemainWorkbenchPuzzleComponent missing", actor)
        return

    _append_actor_result(rows, label, "PASS", "info", "RemainWorkbenchPuzzleComponent found", actor)

    slot_configs = _get_prop(component, "slot_configs", "SlotConfigs")
    if slot_configs is None:
        _append_actor_result(rows, label, "WARN", "warning", "Cannot read SlotConfigs via Python; verify in Details panel", actor)
        return

    if len(slot_configs) != 5:
        _append_actor_result(rows, label, "FAIL", "error", f"Expected 5 SlotConfigs, got {len(slot_configs)}", actor)
    else:
        _append_actor_result(rows, label, "PASS", "info", "SlotConfigs count OK", actor)

    names = set(_component_names(actor))
    for index, slot in enumerate(slot_configs):
        expected_slot_id = EXPECTED_WORKBENCH_SLOT_IDS[index] if index < len(EXPECTED_WORKBENCH_SLOT_IDS) else ""
        expected_part_id = EXPECTED_WORKBENCH_PART_IDS[index] if index < len(EXPECTED_WORKBENCH_PART_IDS) else ""
        slot_id = _name_text(_get_prop(slot, "slot_id", "SlotId"))
        required_part_id = _name_text(_get_prop(slot, "required_part_id", "RequiredPartId"))

        if slot_id != expected_slot_id:
            _append_actor_result(rows, label, "WARN", "warning", f"Slot {index} SlotId expected {expected_slot_id}, got {slot_id}", actor)
        else:
            _append_actor_result(rows, label, "PASS", "info", f"Slot {index} SlotId OK", actor)

        if required_part_id != expected_part_id:
            _append_actor_result(rows, label, "FAIL", "error", f"Slot {index} RequiredPartId expected {expected_part_id}, got {required_part_id}", actor)
        else:
            _append_actor_result(rows, label, "PASS", "info", f"Slot {index} RequiredPartId OK", actor)

        slot_mesh_name = f"SlotMesh_0{index + 1}"
        warm_light_name = f"WarmLight_0{index + 1}"
        if slot_mesh_name not in names:
            _append_actor_result(rows, label, "WARN", "warning", f"Component {slot_mesh_name} not found", actor)
        if warm_light_name not in names:
            _append_actor_result(rows, label, "WARN", "warning", f"Component {warm_light_name} not found", actor)


def _validate_light_fault_controller(rows):
    label = "BP_WorkshopLightFaultController"
    actor = _find_actor(label)
    if not actor:
        return

    controlled_lights = _get_prop(actor, "controlled_lights", "ControlledLights")
    if controlled_lights is None:
        _append_actor_result(rows, label, "WARN", "warning", "Cannot read ControlledLights; verify in Details panel", actor)
        return

    if len(controlled_lights) == 0:
        _append_actor_result(rows, label, "WARN", "warning", "ControlledLights is empty; P15 light flicker needs manual assignment or proxy lights", actor)
    else:
        null_count = 0
        for config in controlled_lights:
            light_actor = _get_prop(config, "light_actor", "LightActor")
            if not light_actor:
                null_count += 1
        if null_count:
            _append_actor_result(rows, label, "WARN", "warning", f"ControlledLights has {null_count} empty LightActor reference(s)", actor)
        else:
            _append_actor_result(rows, label, "PASS", "info", f"ControlledLights configured: {len(controlled_lights)}", actor)


def _validate_manual_items(rows):
    manual = [
        "PIE spawn, camera, HUD, and player input",
        "Interact trace is not blocked by imported FinalHouse collision",
        "Opening documents can be opened and closed",
        "Document dizziness triggers while document UI is still open",
        "P10 screen flash/audio/light feedback after panel pickup",
        "P11 nurse appears in the intended sightline and proximity feedback works",
        "Workbench Choose UI opens only for the current required part",
        "Save/load restores workbench and event state",
    ]
    for item in manual:
        rows.append({
            "category": "manual",
            "label": "",
            "status": "MANUAL",
            "severity": "manual",
            "message": item,
            "folder": "",
            "location": "",
            "class_path": "",
        })


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "main_route_static_validation.csv")
    fieldnames = [
        "category",
        "label",
        "status",
        "severity",
        "message",
        "folder",
        "location",
        "class_path",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[MainRouteValidation] Report written: {output_path}")


def main():
    unreal.log(f"[MainRouteValidation] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    finalhouse = _find_actor(FINALHOUSE_LABEL)
    if not finalhouse:
        raise RuntimeError(f"Cannot find LevelInstance actor label '{FINALHOUSE_LABEL}' in {MAP_PATH}")

    rows = []
    _validate_expected_actors(rows, finalhouse.get_actor_location())
    _validate_workbench(rows)
    _validate_light_fault_controller(rows)
    _validate_manual_items(rows)
    _write_report(rows)

    counts = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    unreal.log(f"[MainRouteValidation] Status counts: {counts}")


if __name__ == "__main__":
    main()
