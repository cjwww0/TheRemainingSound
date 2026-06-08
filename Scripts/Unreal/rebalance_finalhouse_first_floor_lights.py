import csv
import os

import unreal


TARGET_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"
OUTPUT_NAME = "finalhouse_first_floor_light_rebalance_report.csv"

FIRST_FLOOR_MAX_Z = 1450.0
INTENSITY_THRESHOLD = 2500.0
INTENSITY_SCALE = 0.35
INTENSITY_CAP = 3500.0

EXCLUDED_COMPONENT_CLASSES = {
    "DirectionalLightComponent",
    "SkyLightComponent",
}


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def safe_get(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def safe_set(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def is_light_component(component):
    class_name = component.get_class().get_name()
    if class_name in EXCLUDED_COMPONENT_CLASSES:
        return False
    return "LightComponent" in class_name


def new_intensity_for(old_intensity):
    return min(old_intensity * INTENSITY_SCALE, INTENSITY_CAP)


def main():
    unreal.log(f"[FinalHouseLightRebalance] Loading map: {TARGET_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP):
        raise RuntimeError(f"Failed to load map: {TARGET_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rows = []
    changed = 0

    for actor in actor_subsystem.get_all_level_actors():
        actor_location = actor.get_actor_location()
        if actor_location.z > FIRST_FLOOR_MAX_Z:
            continue

        for component in actor.get_components_by_class(unreal.ActorComponent):
            if not is_light_component(component):
                continue

            before = safe_get(component, "intensity")
            if before is None:
                continue

            try:
                before_float = float(before)
            except Exception:
                continue

            if before_float <= INTENSITY_THRESHOLD:
                continue

            after = new_intensity_for(before_float)
            if safe_set(component, "intensity", after):
                changed += 1
                rows.append({
                    "actor_label": label(actor),
                    "actor_name": actor.get_name(),
                    "actor_class": actor.get_class().get_name(),
                    "actor_location": vec_text(actor_location),
                    "component_name": component.get_name(),
                    "component_class": component.get_class().get_name(),
                    "before_intensity": before_float,
                    "after_intensity": after,
                    "scale": INTENSITY_SCALE,
                    "cap": INTENSITY_CAP,
                    "threshold": INTENSITY_THRESHOLD,
                    "action": "scaled first-floor local light",
                })
                unreal.log(
                    f"[FinalHouseLightRebalance] {label(actor)}.{component.get_name()} "
                    f"{before_float:.1f} -> {after:.1f}"
                )

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    fieldnames = [
        "actor_label",
        "actor_name",
        "actor_class",
        "actor_location",
        "component_name",
        "component_class",
        "before_intensity",
        "after_intensity",
        "scale",
        "cap",
        "threshold",
        "action",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    if changed == 0:
        unreal.log_warning("[FinalHouseLightRebalance] No first-floor lights required adjustment")
    else:
        saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        unreal.log(f"[FinalHouseLightRebalance] Changed={changed}; Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
