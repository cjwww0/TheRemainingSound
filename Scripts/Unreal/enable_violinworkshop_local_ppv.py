import csv
import os

import unreal


TARGET_MAP = "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop"
OUTPUT_NAME = "violinworkshop_local_ppv_report.csv"

TARGET_LABEL = "PostProcessVolume2"
LOCAL_PRIORITY = 150.0
LOCAL_BLEND_RADIUS = 350.0
LOCAL_BLEND_WEIGHT = 1.0

# Keep the same exposure baseline as the validated global PPV, then lift workshop only.
GLOBAL_EXPOSURE_BIAS_BASELINE = 5.0
WORKSHOP_EXPOSURE_LIFT = 0.75
WORKSHOP_EXPOSURE_BIAS = GLOBAL_EXPOSURE_BIAS_BASELINE + WORKSHOP_EXPOSURE_LIFT

GLOBAL_MIN_EXPOSURE = 0.029999999329447746
GLOBAL_MAX_EXPOSURE = 8.0
GLOBAL_BLOOM_INTENSITY = 0.675000011920929
GLOBAL_VIGNETTE_INTENSITY = 0.4000000059604645


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


def set_override(settings, names, value=True):
    return set_if_exists(settings, names, value)


def set_setting(settings, names, value, override_names=None):
    override_written = ""
    if override_names:
        override_written = set_override(settings, override_names, True)
    property_written = set_if_exists(settings, names, value)
    return property_written, override_written


def safe_str(value):
    if value is None:
        return ""
    return str(value)


def main():
    unreal.log(f"[ViolinWorkshopPPV] Loading map: {TARGET_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP):
        raise RuntimeError(f"Failed to load map: {TARGET_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    target = None
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name() != "PostProcessVolume":
            continue
        if label(actor) == TARGET_LABEL:
            target = actor
            break

    if target is None:
        raise RuntimeError(f"PostProcessVolume not found: {TARGET_LABEL}")

    settings = get_if_exists(target, ["settings"])
    if settings is None:
        raise RuntimeError(f"PostProcessVolume has no settings: {TARGET_LABEL}")

    before = {
        "enabled": get_if_exists(target, ["enabled", "b_enabled"]),
        "unbound": get_if_exists(target, ["unbound", "b_unbound"]),
        "priority": get_if_exists(target, ["priority"]),
        "blend_radius": get_if_exists(target, ["blend_radius"]),
        "blend_weight": get_if_exists(target, ["blend_weight"]),
        "auto_exposure_method": get_if_exists(settings, ["auto_exposure_method"]),
        "auto_exposure_bias": get_if_exists(settings, ["auto_exposure_bias"]),
        "auto_exposure_min": get_if_exists(settings, ["auto_exposure_min_brightness", "auto_exposure_min_ev100"]),
        "auto_exposure_max": get_if_exists(settings, ["auto_exposure_max_brightness", "auto_exposure_max_ev100"]),
        "bloom_intensity": get_if_exists(settings, ["bloom_intensity"]),
        "vignette_intensity": get_if_exists(settings, ["vignette_intensity"]),
    }

    set_if_exists(target, ["enabled", "b_enabled"], True)
    set_if_exists(target, ["unbound", "b_unbound"], False)
    set_if_exists(target, ["priority"], LOCAL_PRIORITY)
    set_if_exists(target, ["blend_radius"], LOCAL_BLEND_RADIUS)
    set_if_exists(target, ["blend_weight"], LOCAL_BLEND_WEIGHT)

    set_setting(
        settings,
        ["auto_exposure_method"],
        unreal.AutoExposureMethod.AEM_MANUAL,
        ["override_auto_exposure_method", "b_override_auto_exposure_method"],
    )
    set_setting(
        settings,
        ["auto_exposure_bias"],
        WORKSHOP_EXPOSURE_BIAS,
        ["override_auto_exposure_bias", "b_override_auto_exposure_bias"],
    )
    set_setting(
        settings,
        ["auto_exposure_min_brightness", "auto_exposure_min_ev100"],
        GLOBAL_MIN_EXPOSURE,
        ["override_auto_exposure_min_brightness", "override_auto_exposure_min_ev100", "b_override_auto_exposure_min_brightness", "b_override_auto_exposure_min_ev100"],
    )
    set_setting(
        settings,
        ["auto_exposure_max_brightness", "auto_exposure_max_ev100"],
        GLOBAL_MAX_EXPOSURE,
        ["override_auto_exposure_max_brightness", "override_auto_exposure_max_ev100", "b_override_auto_exposure_max_brightness", "b_override_auto_exposure_max_ev100"],
    )
    set_setting(
        settings,
        ["bloom_intensity"],
        GLOBAL_BLOOM_INTENSITY,
        ["override_bloom_intensity", "b_override_bloom_intensity"],
    )
    set_setting(
        settings,
        ["vignette_intensity"],
        GLOBAL_VIGNETTE_INTENSITY,
        ["override_vignette_intensity", "b_override_vignette_intensity"],
    )

    after = {
        "enabled": get_if_exists(target, ["enabled", "b_enabled"]),
        "unbound": get_if_exists(target, ["unbound", "b_unbound"]),
        "priority": get_if_exists(target, ["priority"]),
        "blend_radius": get_if_exists(target, ["blend_radius"]),
        "blend_weight": get_if_exists(target, ["blend_weight"]),
        "auto_exposure_method": get_if_exists(settings, ["auto_exposure_method"]),
        "auto_exposure_bias": get_if_exists(settings, ["auto_exposure_bias"]),
        "auto_exposure_min": get_if_exists(settings, ["auto_exposure_min_brightness", "auto_exposure_min_ev100"]),
        "auto_exposure_max": get_if_exists(settings, ["auto_exposure_max_brightness", "auto_exposure_max_ev100"]),
        "bloom_intensity": get_if_exists(settings, ["bloom_intensity"]),
        "vignette_intensity": get_if_exists(settings, ["vignette_intensity"]),
    }

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    row = {
        "label": label(target),
        "location": vec_text(target.get_actor_location()),
        "scale": vec_text(target.get_actor_scale3d()),
        "exposure_lift": WORKSHOP_EXPOSURE_LIFT,
    }
    for key, value in before.items():
        row[f"before_{key}"] = safe_str(value)
    for key, value in after.items():
        row[f"after_{key}"] = safe_str(value)

    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(row.keys()))
        writer.writeheader()
        writer.writerow(row)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ViolinWorkshopPPV] Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
