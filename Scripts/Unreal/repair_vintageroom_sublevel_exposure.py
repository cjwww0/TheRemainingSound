import csv
import os

import unreal


TARGET_MAP = "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop"
OUTPUT_NAME = "vintageroom_exposure_repair_report.csv"


def set_if_exists(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            pass
    return False


def get_if_exists(obj, names):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def main():
    unreal.log(f"[VintageRoomRepair] Loading map: {TARGET_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP):
        raise RuntimeError(f"Failed to load map: {TARGET_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rows = []
    changed = 0

    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name() != "PostProcessVolume":
            continue

        before_enabled = get_if_exists(actor, ["enabled", "b_enabled"])
        before_unbound = get_if_exists(actor, ["unbound", "b_unbound"])
        before_blend_weight = get_if_exists(actor, ["blend_weight"])

        set_if_exists(actor, ["enabled", "b_enabled"], False)
        set_if_exists(actor, ["blend_weight"], 0.0)
        set_if_exists(actor, ["unbound", "b_unbound"], False)

        rows.append({
            "label": label(actor),
            "location": vec_text(actor.get_actor_location()),
            "before_enabled": before_enabled,
            "before_unbound": before_unbound,
            "before_blend_weight": before_blend_weight,
            "after_enabled": get_if_exists(actor, ["enabled", "b_enabled"]),
            "after_unbound": get_if_exists(actor, ["unbound", "b_unbound"]),
            "after_blend_weight": get_if_exists(actor, ["blend_weight"]),
            "action": "disabled imported unbound postprocess",
        })
        changed += 1
        unreal.log(f"[VintageRoomRepair] Disabled imported PostProcessVolume: {label(actor)}")

    if changed == 0:
        raise RuntimeError("No PostProcessVolume found in Sub_ViolinWorkshop")

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    fieldnames = [
        "label",
        "location",
        "before_enabled",
        "before_unbound",
        "before_blend_weight",
        "after_enabled",
        "after_unbound",
        "after_blend_weight",
        "action",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[VintageRoomRepair] Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
