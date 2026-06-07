import json
import os

import unreal


DEMO_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
MOOD_PP_LABEL = "TRS_GlobalMood_PostProcess"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
REPORT_PATH = os.path.join(PROJECT_DIR, "Saved", "DemoSceneHorrorMoodPostProcess.json")


def safe_get(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as exc:
        return f"<read failed: {exc}>"


def safe_set(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        return f"<set failed: {exc}>"


def set_first(obj, names, value):
    results = {}
    for name in names:
        result = safe_set(obj, name, value)
        results[name] = result
        if result is True:
            return name, results
    return None, results


def enum_value(enum_type, names):
    for name in names:
        try:
            return getattr(enum_type, name)
        except Exception:
            continue
    return None


def json_safe(value):
    if isinstance(value, (str, int, float, bool)) or value is None:
        return value
    if isinstance(value, (list, tuple)):
        return [json_safe(item) for item in value]
    if isinstance(value, dict):
        return {str(key): json_safe(item) for key, item in value.items()}
    return str(value)


def find_or_create_postprocess(actor_subsystem):
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() == MOOD_PP_LABEL and actor.get_class().get_name() == "PostProcessVolume":
            return actor, False

    actor = actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(0.0, 0.0, 200.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(MOOD_PP_LABEL)
    return actor, True


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(DEMO_MAP):
        raise RuntimeError(f"Failed to load map: {DEMO_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor, created = find_or_create_postprocess(actor_subsystem)

    report = {
        "map": DEMO_MAP,
        "actor_label": MOOD_PP_LABEL,
        "created": created,
        "actor_path": actor.get_path_name(),
        "sets": {},
    }

    report["sets"]["enabled"] = set_first(actor, ["enabled", "b_enabled"], True)
    report["sets"]["unbound"] = set_first(actor, ["unbound", "b_unbound"], True)
    report["sets"]["blend_weight"] = set_first(actor, ["blend_weight"], 1.0)
    report["sets"]["blend_radius"] = set_first(actor, ["blend_radius"], 0.0)
    report["sets"]["priority"] = set_first(actor, ["priority"], 100.0)

    settings = safe_get(actor, "settings")
    report["settings_before"] = {}
    report["settings_sets"] = {}

    if not isinstance(settings, str):
        for name in [
            "auto_exposure_method",
            "auto_exposure_bias",
            "camera_iso",
            "camera_shutter_speed",
            "camera_aperture",
        ]:
            report["settings_before"][name] = json_safe(safe_get(settings, name))

        manual = enum_value(unreal.AutoExposureMethod, ["AEM_MANUAL", "MANUAL"])
        if manual is not None:
            report["settings_sets"]["override_auto_exposure_method"] = safe_set(settings, "override_auto_exposure_method", True)
            report["settings_sets"]["auto_exposure_method"] = safe_set(settings, "auto_exposure_method", manual)

        # First horror pass: visibly darker but still playable. Lower this later if needed.
        report["settings_sets"]["override_auto_exposure_bias"] = safe_set(settings, "override_auto_exposure_bias", True)
        report["settings_sets"]["auto_exposure_bias"] = safe_set(settings, "auto_exposure_bias", -2.5)

        # Make manual exposure deterministic in case the engine ignores bias under manual mode.
        report["settings_sets"]["override_camera_iso"] = safe_set(settings, "override_camera_iso", True)
        report["settings_sets"]["camera_iso"] = safe_set(settings, "camera_iso", 100.0)
        report["settings_sets"]["override_camera_aperture"] = safe_set(settings, "override_camera_aperture", True)
        report["settings_sets"]["camera_aperture"] = safe_set(settings, "camera_aperture", 8.0)
        report["settings_sets"]["override_camera_shutter_speed"] = safe_set(settings, "override_camera_shutter_speed", True)
        report["settings_sets"]["camera_shutter_speed"] = safe_set(settings, "camera_shutter_speed", 250.0)

        # Subtle desaturation for horror tone. Unsupported property names are harmlessly reported.
        report["settings_sets"]["override_color_saturation"] = safe_set(settings, "override_color_saturation", True)
        report["settings_sets"]["color_saturation"] = safe_set(settings, "color_saturation", unreal.Vector4(0.86, 0.86, 0.86, 1.0))

        actor.set_editor_property("settings", settings)

    if hasattr(actor, "post_edit_change"):
        actor.post_edit_change()

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        json.dump(json_safe(report), handle, ensure_ascii=False, indent=2)

    unreal.log_warning(f"[SceneRepair] Applied horror mood PP. Report={REPORT_PATH}")


if __name__ == "__main__":
    main()
