import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_LABEL = "FinalHouse"
PLACEMENT_TAG = "MainRoutePlaced_20260607"


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


def _vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def _rot_text(rotator):
    return f"{rotator.roll:.3f},{rotator.pitch:.3f},{rotator.yaw:.3f}"


def _find_actor_by_label(label):
    matches = [actor for actor in _get_all_actors() if _label(actor) == label]
    if not matches:
        return None
    if len(matches) > 1:
        unreal.log_warning(f"[MainRoutePlacement] Multiple actors share label '{label}', using {matches[0].get_path_name()}")
    return matches[0]


def _set_folder(actor, folder):
    try:
        actor.set_folder_path(folder)
    except Exception:
        try:
            actor.set_folder_path(unreal.Name(folder))
        except Exception as exc:
            unreal.log_warning(f"[MainRoutePlacement] Failed to set folder for {_label(actor)}: {exc}")


def _add_tag(actor, tag):
    tags = [str(existing) for existing in actor.tags]
    if tag not in tags:
        actor.tags = list(actor.tags) + [unreal.Name(tag)]


def _world_from_finalhouse(finalhouse_origin, local_xyz):
    return unreal.Vector(
        finalhouse_origin.x + local_xyz[0],
        finalhouse_origin.y + local_xyz[1],
        finalhouse_origin.z + local_xyz[2],
    )


def _move_actor(label, local_xyz, folder, finalhouse_origin, rows, notes="", rotation_yaw=None):
    actor = _find_actor_by_label(label)
    row = {
        "label": label,
        "status": "missing",
        "folder": folder,
        "old_location": "",
        "new_location": "",
        "old_rotation": "",
        "new_rotation": "",
        "notes": notes,
    }
    if not actor:
        unreal.log_warning(f"[MainRoutePlacement] Missing actor: {label}")
        rows.append(row)
        return

    old_loc = actor.get_actor_location()
    old_rot = actor.get_actor_rotation()
    new_loc = _world_from_finalhouse(finalhouse_origin, local_xyz)

    actor.set_actor_location(new_loc, False, True)
    if rotation_yaw is not None:
        actor.set_actor_rotation(unreal.Rotator(0.0, rotation_yaw, 0.0), False)
    _set_folder(actor, folder)
    _add_tag(actor, PLACEMENT_TAG)

    row.update({
        "status": "moved",
        "old_location": _vec_text(old_loc),
        "new_location": _vec_text(actor.get_actor_location()),
        "old_rotation": _rot_text(old_rot),
        "new_rotation": _rot_text(actor.get_actor_rotation()),
    })
    rows.append(row)
    unreal.log(f"[MainRoutePlacement] {label}: {_vec_text(old_loc)} -> {_vec_text(actor.get_actor_location())}")


def _folder_only(labels, folder, rows):
    for label in labels:
        actor = _find_actor_by_label(label)
        row = {
            "label": label,
            "status": "missing",
            "folder": folder,
            "old_location": "",
            "new_location": "",
            "old_rotation": "",
            "new_rotation": "",
            "notes": "folder_only",
        }
        if actor:
            old_loc = actor.get_actor_location()
            old_rot = actor.get_actor_rotation()
            _set_folder(actor, folder)
            row.update({
                "status": "foldered",
                "old_location": _vec_text(old_loc),
                "new_location": _vec_text(old_loc),
                "old_rotation": _rot_text(old_rot),
                "new_rotation": _rot_text(old_rot),
            })
        rows.append(row)


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "main_route_placement_report.csv")
    fieldnames = [
        "label",
        "status",
        "folder",
        "old_location",
        "new_location",
        "old_rotation",
        "new_rotation",
        "notes",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[MainRoutePlacement] Report written: {output_path}")


def main():
    unreal.log(f"[MainRoutePlacement] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    finalhouse = _find_actor_by_label(FINALHOUSE_LABEL)
    if not finalhouse:
        raise RuntimeError(f"Cannot find LevelInstance actor label '{FINALHOUSE_LABEL}' in {MAP_PATH}")

    finalhouse_origin = finalhouse.get_actor_location()
    unreal.log(f"[MainRoutePlacement] FinalHouse origin: {_vec_text(finalhouse_origin)}")

    rows = []

    # Local coordinates are authored in the FinalHouse sub-level space, then offset by
    # the placed FinalHouse LevelInstance origin in DemoScene_01.
    placements = [
        ("Player Start2", (1685.0, -245.0, 1057.0), "Gameplay_Logic/00_Player", "new scene start", 180.0),

        ("BP_Document16", (1268.0, 531.0, 1036.0), "Gameplay_Logic/01_Opening", "opening letter / first readable", None),
        ("BP_Document14", (1308.0, 552.0, 1036.0), "Gameplay_Logic/01_Opening", "opening diary / document", None),
        ("BP_Document11", (1180.0, 355.0, 1034.0), "Gameplay_Logic/01_Opening", "opening photo / document", None),
        ("BP_InventoryItem", (1295.0, 515.0, 1036.0), "Gameplay_Logic/01_Opening", "opening inventory pickup candidate", None),

        ("BP_PanelPickupShockController", (1300.0, 500.0, 1045.0), "Gameplay_Logic/02_P10_P11", "P10 controller near table/cup", None),
        ("BP_NurseEncounter_01", (1730.0, -120.0, 1057.0), "Gameplay_Logic/02_P10_P11", "P11 sightline near entrance corridor", None),

        ("BP_WorkbenchPanel_PuzzleActor", (-315.0, 900.0, 1690.0), "Gameplay_Logic/03_Workbench", "workbench puzzle placeholder", None),
        ("BP_WorkshopLightFaultController", (-260.0, 875.0, 1650.0), "Gameplay_Logic/03_Workbench", "P15 controller; light refs need viewport verification", None),
        ("BP_WorkbenchPickup_01", (1265.0, 545.0, 1038.0), "Gameplay_Logic/03_Workbench", "Part01 on first-floor table", None),
        ("BP_WorkbenchPickup_02", (-300.0, 890.0, 1665.0), "Gameplay_Logic/03_Workbench", "Part02 near workbench", None),
        ("BP_WorkbenchPickup_03", (742.0, 108.0, 1665.0), "Gameplay_Logic/03_Workbench", "Part03 upstairs side table", None),
        ("BP_WorkbenchPickup_04", (1610.0, 540.0, 1038.0), "Gameplay_Logic/03_Workbench", "Part04 first-floor round table", None),
        ("BP_WorkbenchPickup_05", (-307.0, -25.0, 1665.0), "Gameplay_Logic/03_Workbench", "Part05 upstairs side table", None),

        ("BP_CheckpointTrigger", (1500.0, 100.0, 1057.0), "Gameplay_Logic/04_Gates_Checkpoints", "post-opening checkpoint", None),
        ("BP_SaveMachine", (-300.0, 930.0, 1665.0), "Gameplay_Logic/04_Gates_Checkpoints", "upstairs save/debug point", None),
        ("BP_Keypad_Puzzle", (1770.0, -150.0, 1120.0), "Gameplay_Logic/04_Gates_Checkpoints", "main-route keypad candidate", 90.0),
        ("BP_CombinationLock_PuzzleActor", (1600.0, -330.0, 1120.0), "Gameplay_Logic/04_Gates_Checkpoints", "optional lock candidate", 180.0),
        ("HM_BidirectionalDoor6", (1770.0, -100.0, 1057.0), "Gameplay_Logic/04_Gates_Checkpoints", "door proxy candidate; verify against visual door", None),
    ]

    for label, local_xyz, folder, notes, rotation_yaw in placements:
        _move_actor(label, local_xyz, folder, finalhouse_origin, rows, notes, rotation_yaw)

    _folder_only([
        "BP_Document",
        "BP_Document2",
        "BP_Document3",
        "BP_Document4",
        "BP_Document5",
        "BP_Document6",
        "BP_Document7",
        "BP_Document8",
        "BP_Document9",
        "BP_Document10",
        "BP_Document12",
        "BP_Document13",
        "BP_Document15",
        "BP_InventoryItem2",
        "BP_InventoryItem3",
        "BP_InventoryItem4",
        "BP_InventoryItem5",
        "BP_InventoryItem6",
        "BP_Keypad_Puzzle2",
        "BP_CombinationLock_PuzzleActor2",
        "BP_CombinationLock_PuzzleActor3",
        "BP_CombinationLock_PuzzleActor4",
    ], "Template_Reference", rows)

    _write_report(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[MainRoutePlacement] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
