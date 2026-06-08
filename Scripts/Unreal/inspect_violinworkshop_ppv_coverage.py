import csv
import os

import unreal


PERSISTENT_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
OUTPUT_NAME = "violinworkshop_ppv_coverage.csv"


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def level_name(actor):
    try:
        level = actor.get_level()
        outer = level.get_outer()
        return outer.get_path_name()
    except Exception:
        return ""


def class_name(actor):
    try:
        return actor.get_class().get_name()
    except Exception:
        return ""


def get_if_exists(obj, names):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def add_bounds(min_v, max_v, center, extent):
    local_min = unreal.Vector(center.x - extent.x, center.y - extent.y, center.z - extent.z)
    local_max = unreal.Vector(center.x + extent.x, center.y + extent.y, center.z + extent.z)
    if min_v is None:
        return local_min, local_max
    min_v.x = min(min_v.x, local_min.x)
    min_v.y = min(min_v.y, local_min.y)
    min_v.z = min(min_v.z, local_min.z)
    max_v.x = max(max_v.x, local_max.x)
    max_v.y = max(max_v.y, local_max.y)
    max_v.z = max(max_v.z, local_max.z)
    return min_v, max_v


def main():
    unreal.log(f"[ViolinWorkshopCoverage] Loading map: {PERSISTENT_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(PERSISTENT_MAP):
        raise RuntimeError(f"Failed to load map: {PERSISTENT_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    workshop_actors = []
    ppv_actors = []
    min_v = None
    max_v = None

    for actor in actors:
        lv = level_name(actor)
        if "Sub_ViolinWorkshop" not in lv:
            continue

        cls = class_name(actor)
        name = label(actor)
        if cls == "PostProcessVolume":
            ppv_actors.append(actor)
            continue

        # Use visible geometry and gameplay actors for the practical room bounds.
        if cls in ("SkyLight", "DirectionalLight", "PostProcessVolume"):
            continue

        center, extent = actor.get_actor_bounds(False)
        if extent.x <= 1.0 and extent.y <= 1.0 and extent.z <= 1.0:
            continue

        workshop_actors.append(actor)
        min_v, max_v = add_bounds(min_v, max_v, center, extent)

    if min_v is None or max_v is None:
        raise RuntimeError("No workshop bounds found. Is Sub_ViolinWorkshop loaded in DemoScene_01?")

    bounds_center = unreal.Vector(
        (min_v.x + max_v.x) * 0.5,
        (min_v.y + max_v.y) * 0.5,
        (min_v.z + max_v.z) * 0.5,
    )
    bounds_extent = unreal.Vector(
        (max_v.x - min_v.x) * 0.5,
        (max_v.y - min_v.y) * 0.5,
        (max_v.z - min_v.z) * 0.5,
    )

    rows = []
    for ppv in ppv_actors:
        location = ppv.get_actor_location()
        delta = unreal.Vector(
            location.x - bounds_center.x,
            location.y - bounds_center.y,
            location.z - bounds_center.z,
        )
        settings = get_if_exists(ppv, ["settings"])
        rows.append({
            "kind": "ppv",
            "label": label(ppv),
            "class": class_name(ppv),
            "level": level_name(ppv),
            "actor_count_for_bounds": len(workshop_actors),
            "workshop_center": vec_text(bounds_center),
            "workshop_extent": vec_text(bounds_extent),
            "ppv_location": vec_text(location),
            "ppv_scale": vec_text(ppv.get_actor_scale3d()),
            "delta_from_workshop_center": vec_text(delta),
            "enabled": get_if_exists(ppv, ["enabled", "b_enabled"]),
            "unbound": get_if_exists(ppv, ["unbound", "b_unbound"]),
            "priority": get_if_exists(ppv, ["priority"]),
            "blend_radius": get_if_exists(ppv, ["blend_radius"]),
            "blend_weight": get_if_exists(ppv, ["blend_weight"]),
            "auto_exposure_bias": get_if_exists(settings, ["auto_exposure_bias"]) if settings else "",
        })

    rows.append({
        "kind": "bounds",
        "label": "Sub_ViolinWorkshop geometry bounds",
        "class": "",
        "level": "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop",
        "actor_count_for_bounds": len(workshop_actors),
        "workshop_center": vec_text(bounds_center),
        "workshop_extent": vec_text(bounds_extent),
        "ppv_location": "",
        "ppv_scale": "",
        "delta_from_workshop_center": "",
        "enabled": "",
        "unbound": "",
        "priority": "",
        "blend_radius": "",
        "blend_weight": "",
        "auto_exposure_bias": "",
    })

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        fieldnames = list(rows[0].keys()) if rows else []
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    unreal.log(f"[ViolinWorkshopCoverage] Wrote {output_path}")


if __name__ == "__main__":
    main()
