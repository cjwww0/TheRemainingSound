import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
SHOWCASE_LABEL = "Showcase"

EXPECTED_WORKBENCH_PART_IDS = ["Part01", "Part02", "Part03", "Part04", "Part05"]
EXPECTED_WORKBENCH_SLOT_IDS = ["Panel01", "Panel02", "Panel03", "Panel04", "Panel05"]

EXPECTED_ACTORS = [
    "HM_BidirectionalDoor6",
    "BP_WorkbenchPanel_PuzzleActor",
    "BP_WorkshopLightFaultController",
    "P15_ProxyWorkshopFaultLight",
    "BP_WorkbenchPickup_01",
    "BP_WorkbenchPickup_02",
    "BP_WorkbenchPickup_03",
    "BP_WorkbenchPickup_04",
    "BP_WorkbenchPickup_05",
    "BP_ViolinPart_TopPlate0",
    "BP_ViolinPart_BackPlate0",
    "BP_ViolinPart_SoundPost27",
    "BP_ViolinPart_Bridge31",
    "BP_ViolinPart_Bow34",
]


def _actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _class_path(obj):
    try:
        return obj.get_class().get_path_name()
    except Exception:
        return ""


def _find(label):
    matches = [actor for actor in _actors() if _label(actor) == label]
    return matches[0] if matches else None


def _vec_text(value):
    return f"{value.x:.3f},{value.y:.3f},{value.z:.3f}"


def _folder(actor):
    try:
        return str(actor.get_folder_path())
    except Exception:
        return ""


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
    return "" if text == "None" else text


def _find_component_by_class_name(actor, class_name_part):
    try:
        for component in actor.get_components_by_class(unreal.ActorComponent):
            if class_name_part in _class_path(component):
                return component
    except Exception:
        pass
    return None


def _append(rows, label, status, message, actor=None):
    rows.append({
        "label": label,
        "status": status,
        "message": message,
        "location": _vec_text(actor.get_actor_location()) if actor else "",
        "folder": _folder(actor) if actor else "",
        "class_path": _class_path(actor) if actor else "",
    })


def _near_workshop(location, origin):
    return (
        origin.x - 750.0 <= location.x <= origin.x + 750.0
        and origin.y - 1000.0 <= location.y <= origin.y + 700.0
        and origin.z - 20.0 <= location.z <= origin.z + 320.0
    )


def _validate_actor_locations(rows, origin):
    for label in EXPECTED_ACTORS:
        actor = _find(label)
        if not actor:
            _append(rows, label, "FAIL", "Actor missing")
            continue

        if not _near_workshop(actor.get_actor_location(), origin):
            _append(rows, label, "WARN", "Outside expected Showcase workshop placement window", actor)
        else:
            _append(rows, label, "PASS", "Workshop placement window OK", actor)


def _validate_workbench(rows):
    label = "BP_WorkbenchPanel_PuzzleActor"
    actor = _find(label)
    if not actor:
        return

    component = _find_component_by_class_name(actor, "RemainWorkbenchPuzzleComponent")
    if not component:
        _append(rows, label, "FAIL", "RemainWorkbenchPuzzleComponent missing", actor)
        return

    slot_configs = _get_prop(component, "slot_configs", "SlotConfigs")
    if slot_configs is None:
        _append(rows, label, "WARN", "Cannot read SlotConfigs via Python; verify manually", actor)
        return

    if len(slot_configs) != 5:
        _append(rows, label, "FAIL", f"Expected 5 SlotConfigs, got {len(slot_configs)}", actor)
    else:
        _append(rows, label, "PASS", "SlotConfigs count OK", actor)

    for index, slot in enumerate(slot_configs):
        expected_slot = EXPECTED_WORKBENCH_SLOT_IDS[index]
        expected_part = EXPECTED_WORKBENCH_PART_IDS[index]
        slot_id = _name_text(_get_prop(slot, "slot_id", "SlotId"))
        part_id = _name_text(_get_prop(slot, "required_part_id", "RequiredPartId"))
        _append(rows, label, "PASS" if slot_id == expected_slot else "FAIL", f"Slot {index} SlotId={slot_id}, expected={expected_slot}", actor)
        _append(rows, label, "PASS" if part_id == expected_part else "FAIL", f"Slot {index} RequiredPartId={part_id}, expected={expected_part}", actor)


def _validate_light_fault(rows):
    label = "BP_WorkshopLightFaultController"
    actor = _find(label)
    if not actor:
        return

    controlled_lights = _get_prop(actor, "controlled_lights", "ControlledLights")
    if controlled_lights is None:
        _append(rows, label, "WARN", "Cannot read ControlledLights via Python; verify manually", actor)
        return

    if len(controlled_lights) == 0:
        _append(rows, label, "FAIL", "ControlledLights is empty", actor)
        return

    missing = 0
    for config in controlled_lights:
        if not _get_prop(config, "light_actor", "LightActor"):
            missing += 1

    if missing:
        _append(rows, label, "FAIL", f"ControlledLights has {missing} empty LightActor reference(s)", actor)
    else:
        _append(rows, label, "PASS", f"ControlledLights configured: {len(controlled_lights)}", actor)


def _write(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "workshop_mainline_final_validation.csv")
    fields = ["label", "status", "message", "location", "folder", "class_path"]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[WorkshopMainlineValidation] Report written: {output_path}")


def main():
    unreal.log(f"[WorkshopMainlineValidation] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    showcase = _find(SHOWCASE_LABEL)
    if not showcase:
        raise RuntimeError(f"Cannot find '{SHOWCASE_LABEL}'")

    rows = []
    _validate_actor_locations(rows, showcase.get_actor_location())
    _validate_workbench(rows)
    _validate_light_fault(rows)
    _write(rows)

    counts = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    unreal.log(f"[WorkshopMainlineValidation] Status counts: {counts}")


if __name__ == "__main__":
    main()
