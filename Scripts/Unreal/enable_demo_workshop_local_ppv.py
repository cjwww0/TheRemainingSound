import csv
import os

import unreal


PERSISTENT_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
OUTPUT_NAME = "demo_workshop_local_ppv_report.csv"

TARGET_LABEL = "TRS_WorkshopLocal_PostProcess"
SHOWCASE_LABEL = "Showcase"

PRIORITY = 180.0
BLEND_RADIUS = 300.0
BLEND_WEIGHT = 1.0

EXPOSURE_BIAS = 6.25
MIN_EXPOSURE = 0.029999999329447746
MAX_EXPOSURE = 8.0
BLOOM_INTENSITY = 0.675000011920929
VIGNETTE_INTENSITY = 0.4000000059604645

# Default PPV brush is roughly 200x200x200 at scale 1, so this covers only the
# workshop instance area without bleeding back into FinalHouse.
LOCAL_SCALE = unreal.Vector(18.0, 15.0, 12.0)
LOCAL_Z_OFFSET = 180.0


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def get_if_exists(obj, names):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def set_if_exists(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            pass
    return ""


def set_setting(settings, names, value, override_names=None):
    override_written = ""
    if override_names:
        override_written = set_if_exists(settings, override_names, True)
    property_written = set_if_exists(settings, names, value)
    return property_written, override_written


def safe_str(value):
    if value is None:
        return ""
    return str(value)


def find_actor_by_label(actor_subsystem, target_label):
    for actor in actor_subsystem.get_all_level_actors():
        if label(actor) == target_label:
            return actor
    return None


def main():
    unreal.log(f"[DemoWorkshopPPV] Loading map: {PERSISTENT_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(PERSISTENT_MAP):
        raise RuntimeError(f"Failed to load map: {PERSISTENT_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    showcase = find_actor_by_label(actor_subsystem, SHOWCASE_LABEL)
    if showcase is None:
        raise RuntimeError(f"LevelInstance not found: {SHOWCASE_LABEL}")

    target = find_actor_by_label(actor_subsystem, TARGET_LABEL)
    if target is None:
        target = actor_subsystem.spawn_actor_from_class(
            unreal.PostProcessVolume,
            showcase.get_actor_location() + unreal.Vector(0.0, 0.0, LOCAL_Z_OFFSET),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        target.set_actor_label(TARGET_LABEL)

    target.set_actor_location(showcase.get_actor_location() + unreal.Vector(0.0, 0.0, LOCAL_Z_OFFSET), False, False)
    target.set_actor_scale3d(LOCAL_SCALE)

    settings = get_if_exists(target, ["settings"])
    if settings is None:
        raise RuntimeError(f"PostProcessVolume has no settings: {TARGET_LABEL}")

    before = {
        "location": target.get_actor_location(),
        "scale": target.get_actor_scale3d(),
        "enabled": get_if_exists(target, ["enabled", "b_enabled"]),
        "unbound": get_if_exists(target, ["unbound", "b_unbound"]),
        "priority": get_if_exists(target, ["priority"]),
        "blend_radius": get_if_exists(target, ["blend_radius"]),
        "blend_weight": get_if_exists(target, ["blend_weight"]),
        "auto_exposure_bias": get_if_exists(settings, ["auto_exposure_bias"]),
    }

    set_if_exists(target, ["enabled", "b_enabled"], True)
    set_if_exists(target, ["unbound", "b_unbound"], False)
    set_if_exists(target, ["priority"], PRIORITY)
    set_if_exists(target, ["blend_radius"], BLEND_RADIUS)
    set_if_exists(target, ["blend_weight"], BLEND_WEIGHT)

    set_setting(
        settings,
        ["auto_exposure_method"],
        unreal.AutoExposureMethod.AEM_MANUAL,
        ["override_auto_exposure_method", "b_override_auto_exposure_method"],
    )
    set_setting(
        settings,
        ["auto_exposure_bias"],
        EXPOSURE_BIAS,
        ["override_auto_exposure_bias", "b_override_auto_exposure_bias"],
    )
    set_setting(
        settings,
        ["auto_exposure_min_brightness", "auto_exposure_min_ev100"],
        MIN_EXPOSURE,
        ["override_auto_exposure_min_brightness", "override_auto_exposure_min_ev100", "b_override_auto_exposure_min_brightness", "b_override_auto_exposure_min_ev100"],
    )
    set_setting(
        settings,
        ["auto_exposure_max_brightness", "auto_exposure_max_ev100"],
        MAX_EXPOSURE,
        ["override_auto_exposure_max_brightness", "override_auto_exposure_max_ev100", "b_override_auto_exposure_max_brightness", "b_override_auto_exposure_max_ev100"],
    )
    set_setting(
        settings,
        ["bloom_intensity"],
        BLOOM_INTENSITY,
        ["override_bloom_intensity", "b_override_bloom_intensity"],
    )
    set_setting(
        settings,
        ["vignette_intensity"],
        VIGNETTE_INTENSITY,
        ["override_vignette_intensity", "b_override_vignette_intensity"],
    )

    after = {
        "location": target.get_actor_location(),
        "scale": target.get_actor_scale3d(),
        "enabled": get_if_exists(target, ["enabled", "b_enabled"]),
        "unbound": get_if_exists(target, ["unbound", "b_unbound"]),
        "priority": get_if_exists(target, ["priority"]),
        "blend_radius": get_if_exists(target, ["blend_radius"]),
        "blend_weight": get_if_exists(target, ["blend_weight"]),
        "auto_exposure_bias": get_if_exists(settings, ["auto_exposure_bias"]),
    }

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    row = {
        "target_label": label(target),
        "showcase_location": vec_text(showcase.get_actor_location()),
    }
    for key, value in before.items():
        row[f"before_{key}"] = vec_text(value) if isinstance(value, unreal.Vector) else safe_str(value)
    for key, value in after.items():
        row[f"after_{key}"] = vec_text(value) if isinstance(value, unreal.Vector) else safe_str(value)

    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(row.keys()))
        writer.writeheader()
        writer.writerow(row)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[DemoWorkshopPPV] Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
