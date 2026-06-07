import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_LABEL = "FinalHouse"
CUP_LABELS = ["SM_GlassBottle", "GC_SM_GlassBottle_Test"]
TARGET_FOLDER = "Gameplay_Logic/02_P10_P11"
PLACEMENT_TAG = "P10CupPlaced_20260607"

# Based on the imported FinalHouse cup/table anchor near ch_cup_tea.
TARGET_LOCAL_LOCATION = (1306.684, 551.052, 1012.0)


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


def _find_actor(label):
    matches = [actor for actor in _get_all_actors() if _label(actor) == label]
    if not matches:
        return None
    if len(matches) > 1:
        unreal.log_warning(f"[P10CupPlacement] Multiple actors share label '{label}', using {matches[0].get_path_name()}")
    return matches[0]


def _vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def _world_from_finalhouse(finalhouse_origin, local_xyz):
    return unreal.Vector(
        finalhouse_origin.x + local_xyz[0],
        finalhouse_origin.y + local_xyz[1],
        finalhouse_origin.z + local_xyz[2],
    )


def _set_folder(actor, folder):
    try:
        actor.set_folder_path(folder)
    except Exception:
        actor.set_folder_path(unreal.Name(folder))


def _add_tag(actor, tag):
    tags = [str(existing) for existing in actor.tags]
    if tag not in tags:
        actor.tags = list(actor.tags) + [unreal.Name(tag)]


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p10_cup_placement_report.csv")
    fieldnames = ["label", "status", "old_location", "new_location", "folder", "note"]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P10CupPlacement] Report written: {output_path}")


def main():
    unreal.log(f"[P10CupPlacement] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    finalhouse = _find_actor(FINALHOUSE_LABEL)
    if not finalhouse:
        raise RuntimeError(f"Cannot find LevelInstance actor label '{FINALHOUSE_LABEL}'")

    target_location = _world_from_finalhouse(finalhouse.get_actor_location(), TARGET_LOCAL_LOCATION)
    rows = []

    for label in CUP_LABELS:
        actor = _find_actor(label)
        if not actor:
            rows.append({
                "label": label,
                "status": "missing",
                "old_location": "",
                "new_location": "",
                "folder": TARGET_FOLDER,
                "note": "Actor not found",
            })
            continue

        old_location = actor.get_actor_location()
        actor.set_actor_location(target_location, False, True)
        actor.set_actor_hidden_in_game(False)
        actor.set_actor_enable_collision(True)
        _set_folder(actor, TARGET_FOLDER)
        _add_tag(actor, PLACEMENT_TAG)

        rows.append({
            "label": label,
            "status": "moved",
            "old_location": _vec_text(old_location),
            "new_location": _vec_text(actor.get_actor_location()),
            "folder": TARGET_FOLDER,
            "note": "Kept existing BreakableCup references; runtime component decides intact/broken visibility",
        })
        unreal.log(f"[P10CupPlacement] {label}: {_vec_text(old_location)} -> {_vec_text(actor.get_actor_location())}")

    _write_report(rows)
    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[P10CupPlacement] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
