import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
SHOWCASE_LABEL = "Showcase"
PLACEMENT_TAG = "WorkshopMainlinePlaced_20260608"


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


def _v(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def _vec_text(value):
    return f"{value.x:.3f},{value.y:.3f},{value.z:.3f}"


def _set_location(label, location, folder="Gameplay_Logic/03_Workbench"):
    actor = _find(label)
    if not actor:
        return {
            "label": label,
            "status": "MISSING",
            "location": "",
            "message": "Actor not found",
        }

    actor.modify()
    actor.set_actor_location(location, False, False)
    try:
        actor.set_folder_path(folder)
    except Exception:
        pass

    existing_tags = {str(tag) for tag in actor.tags}
    if PLACEMENT_TAG not in existing_tags:
        actor.tags = list(actor.tags) + [unreal.Name(PLACEMENT_TAG)]

    return {
        "label": label,
        "status": "MOVED",
        "location": _vec_text(actor.get_actor_location()),
        "message": "",
    }


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "workshop_mainline_final_placement.csv")
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["label", "status", "location", "message"])
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[WorkshopMainline] Report written: {output_path}")


def main():
    unreal.log(f"[WorkshopMainline] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    showcase = _find(SHOWCASE_LABEL)
    if not showcase:
        raise RuntimeError(f"Cannot find level instance '{SHOWCASE_LABEL}'")

    origin = showcase.get_actor_location()
    unreal.log(f"[WorkshopMainline] Showcase origin: {_vec_text(origin)}")

    rows = []

    # Core workbench interaction group. These are persistent-level gameplay proxies
    # placed relative to the imported workshop level instance.
    placements = {
        "BP_WorkbenchPanel_PuzzleActor": origin + _v(-360, -260, 80),
        "BP_WorkshopLightFaultController": origin + _v(-300, -230, 130),
        "P15_ProxyWorkshopFaultLight": origin + _v(-360, -260, 255),
        "HM_BidirectionalDoor6": origin + _v(0, -820, 80),
        "BP_WorkbenchPickup_01": origin + _v(-540, -330, 85),
        "BP_WorkbenchPickup_02": origin + _v(-180, -460, 85),
        "BP_WorkbenchPickup_03": origin + _v(220, -200, 85),
        "BP_WorkbenchPickup_04": origin + _v(400, 180, 85),
        "BP_WorkbenchPickup_05": origin + _v(-120, 260, 85),
        "BP_ViolinPart_TopPlate0": origin + _v(-540, -330, 90),
        "BP_ViolinPart_BackPlate0": origin + _v(-180, -460, 90),
        "BP_ViolinPart_SoundPost27": origin + _v(220, -200, 90),
        "BP_ViolinPart_Bridge31": origin + _v(400, 180, 90),
        "BP_ViolinPart_Bow34": origin + _v(-120, 260, 90),
    }

    for label, location in placements.items():
        folder = "Gameplay_Logic/04_Gates_Checkpoints" if label == "HM_BidirectionalDoor6" else "Gameplay_Logic/03_Workbench"
        rows.append(_set_location(label, location, folder))

    _write_report(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[WorkshopMainline] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
