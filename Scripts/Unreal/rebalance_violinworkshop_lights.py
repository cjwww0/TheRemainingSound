import csv
import os

import unreal


TARGET_MAP = "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop"
OUTPUT_NAME = "violinworkshop_light_rebalance_report.csv"

SPOT_SCALE = 2.0
POINT_SCALE = 2.0
SPOT_CAP = 30.0
POINT_CAP = 2.5


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


def rebalance_for_component(component, old_intensity):
    class_name = component.get_class().get_name()
    if class_name == "SpotLightComponent":
        return min(old_intensity * SPOT_SCALE, SPOT_CAP), SPOT_SCALE, SPOT_CAP
    if class_name == "PointLightComponent":
        return min(old_intensity * POINT_SCALE, POINT_CAP), POINT_SCALE, POINT_CAP
    return None, None, None


def main():
    unreal.log(f"[ViolinWorkshopLightRebalance] Loading map: {TARGET_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP):
        raise RuntimeError(f"Failed to load map: {TARGET_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rows = []
    changed = 0

    for actor in actor_subsystem.get_all_level_actors():
        for component in actor.get_components_by_class(unreal.ActorComponent):
            before = safe_get(component, "intensity")
            if before is None:
                continue

            try:
                before_float = float(before)
            except Exception:
                continue

            after, scale, cap = rebalance_for_component(component, before_float)
            if after is None:
                continue
            if abs(after - before_float) < 0.0001:
                continue

            if safe_set(component, "intensity", after):
                changed += 1
                rows.append({
                    "actor_label": label(actor),
                    "actor_name": actor.get_name(),
                    "actor_class": actor.get_class().get_name(),
                    "actor_location": vec_text(actor.get_actor_location()),
                    "component_name": component.get_name(),
                    "component_class": component.get_class().get_name(),
                    "before_intensity": before_float,
                    "after_intensity": after,
                    "scale": scale,
                    "cap": cap,
                    "action": "scaled workshop local light",
                })
                unreal.log(
                    f"[ViolinWorkshopLightRebalance] {label(actor)}.{component.get_name()} "
                    f"{before_float:.3f} -> {after:.3f}"
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
        "action",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    if changed == 0:
        unreal.log_warning("[ViolinWorkshopLightRebalance] No workshop lights required adjustment")
    else:
        saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        unreal.log(f"[ViolinWorkshopLightRebalance] Changed={changed}; Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
