import csv
import os

import unreal


MAPS_TO_SCAN = [
    "/Game/HorrorMechanics/Demo/Maps/DemoScene_01",
    "/Game/HorrorMechanics/Demo/Maps/FinalHouse",
    "/Game/Anemoia/MAIN/Maps/Demonstration",
]

INCLUDE_PATTERNS = [
    "BP_Document",
    "BP_Door",
    "BP_AnimatedDoor",
    "BP_BidirectionalDoor",
    "BP_Furniture",
    "BP_Inventory",
    "BP_InventoryKey",
    "BP_InventoryItem",
    "BP_SimpleCollectible",
    "BP_VirtualKey",
    "BP_PuzzleActor",
    "BP_Workbench",
    "BP_WorkshopLightFaultController",
    "BP_Keypad",
    "BP_CombinationLock",
    "BP_CirclePanel",
    "BP_SubObjective",
    "BP_Examinable",
    "BP_AbstractExaminable",
    "BP_Checkpoint",
    "BP_SaveMachine",
    "BP_Switch",
    "BP_EmissiveMeshLight",
    "BP_EventLight",
    "BP_PanelPickupShockController",
    "BP_NurseEncounter",
    "BP_Flashlight",
    "BP_Wieldable",
    "BP_Weapon",
    "BP_Axe",
    "BP_Pistol",
]


def _actor_folder(actor):
    try:
        return str(actor.get_folder_path())
    except Exception:
        return ""


def _actor_label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _actor_class_path(actor):
    try:
        return actor.get_class().get_path_name()
    except Exception:
        return ""


def _actor_path(actor):
    try:
        return actor.get_path_name()
    except Exception:
        return ""


def _vec_to_text(vec):
    return f"{vec.x:.3f},{vec.y:.3f},{vec.z:.3f}"


def _rot_to_text(rot):
    return f"{rot.roll:.3f},{rot.pitch:.3f},{rot.yaw:.3f}"


def _scale_to_text(vec):
    return f"{vec.x:.3f},{vec.y:.3f},{vec.z:.3f}"


def _is_likely_interaction_actor(row):
    haystack = " ".join([
        row["actor_label"],
        row["class_path"],
        row["actor_path"],
        row["folder"],
    ])
    return any(pattern in haystack for pattern in INCLUDE_PATTERNS)


def _get_all_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


def scan_loaded_map(map_path):
    unreal.log(f"[InteractionInventory] Loading map: {map_path}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        unreal.log_warning(f"[InteractionInventory] Failed to load map: {map_path}")
        return []

    rows = []
    for actor in _get_all_actors():
        loc = actor.get_actor_location()
        rot = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        row = {
            "map": map_path,
            "actor_label": _actor_label(actor),
            "actor_name": actor.get_name(),
            "class_path": _actor_class_path(actor),
            "actor_path": _actor_path(actor),
            "folder": _actor_folder(actor),
            "location_xyz": _vec_to_text(loc),
            "rotation_rpy": _rot_to_text(rot),
            "scale_xyz": _scale_to_text(scale),
            "tags": ";".join([str(tag) for tag in actor.tags]),
        }
        row["likely_interaction"] = "true" if _is_likely_interaction_actor(row) else "false"
        rows.append(row)
    return rows


def write_csv(path, rows):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    fieldnames = [
        "map",
        "likely_interaction",
        "actor_label",
        "actor_name",
        "class_path",
        "actor_path",
        "folder",
        "location_xyz",
        "rotation_rpy",
        "scale_xyz",
        "tags",
    ]
    with open(path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def main():
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    all_rows = []
    for map_path in MAPS_TO_SCAN:
        all_rows.extend(scan_loaded_map(map_path))

    filtered_rows = [row for row in all_rows if row["likely_interaction"] == "true"]
    all_path = os.path.join(output_dir, "interaction_actor_inventory_all.csv")
    filtered_path = os.path.join(output_dir, "interaction_actor_inventory_filtered.csv")
    write_csv(all_path, all_rows)
    write_csv(filtered_path, filtered_rows)
    unreal.log(f"[InteractionInventory] Wrote {len(all_rows)} actors to {all_path}")
    unreal.log(f"[InteractionInventory] Wrote {len(filtered_rows)} likely interaction actors to {filtered_path}")


if __name__ == "__main__":
    main()
