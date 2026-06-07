import csv
import os

import unreal


TARGET_MAP = "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop"
OUTPUT_NAME = "showcase_render_state.csv"

RENDER_CLASSES = {
    "PostProcessVolume",
    "DirectionalLight",
    "SkyLight",
    "PointLight",
    "SpotLight",
    "RectLight",
    "SkyAtmosphere",
    "ExponentialHeightFog",
    "VolumetricCloud",
    "LevelSequenceActor",
    "SunSky",
}


def safe_get(obj, *names):
    if obj is None:
        return None
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    return None


def safe_str(value):
    if value is None:
        return ""
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def vec_text(vector):
    if vector is None:
        return ""
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def actor_label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def get_actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def component_summary(actor):
    rows = []
    for component in actor.get_components_by_class(unreal.ActorComponent):
        class_name = component.get_class().get_name()
        if "Light" not in class_name and class_name not in {"ExponentialHeightFogComponent", "SkyAtmosphereComponent"}:
            continue

        rows.append({
            "component": component.get_name(),
            "component_class": class_name,
            "intensity": safe_str(safe_get(component, "intensity")),
            "indirect_lighting_intensity": safe_str(safe_get(component, "indirect_lighting_intensity")),
            "attenuation_radius": safe_str(safe_get(component, "attenuation_radius")),
            "source_radius": safe_str(safe_get(component, "source_radius")),
            "affects_world": safe_str(safe_get(component, "affects_world")),
            "visible": safe_str(safe_get(component, "visible")),
            "hidden_in_game": safe_str(safe_get(component, "hidden_in_game")),
            "mobility": safe_str(safe_get(component, "mobility")),
            "fog_density": safe_str(safe_get(component, "fog_density")),
            "fog_height_falloff": safe_str(safe_get(component, "fog_height_falloff")),
        })
    return rows


def postprocess_rows(actor):
    settings = safe_get(actor, "settings")
    return [{
        "component": "",
        "component_class": "",
        "enabled": safe_str(safe_get(actor, "enabled", "b_enabled")),
        "unbound": safe_str(safe_get(actor, "unbound", "b_unbound")),
        "blend_weight": safe_str(safe_get(actor, "blend_weight")),
        "auto_exposure_method": safe_str(safe_get(settings, "auto_exposure_method")),
        "auto_exposure_bias": safe_str(safe_get(settings, "auto_exposure_bias")),
        "auto_exposure_min_brightness": safe_str(safe_get(settings, "auto_exposure_min_brightness", "auto_exposure_min_ev100")),
        "auto_exposure_max_brightness": safe_str(safe_get(settings, "auto_exposure_max_brightness", "auto_exposure_max_ev100")),
        "camera_iso": safe_str(safe_get(settings, "camera_iso")),
        "camera_shutter_speed": safe_str(safe_get(settings, "camera_shutter_speed")),
        "camera_aperture": safe_str(safe_get(settings, "camera_aperture")),
        "color_gain": safe_str(safe_get(settings, "color_gain")),
        "film_slope": safe_str(safe_get(settings, "film_slope")),
        "film_toe": safe_str(safe_get(settings, "film_toe")),
        "bloom_intensity": safe_str(safe_get(settings, "bloom_intensity")),
        "lens_flare_intensity": safe_str(safe_get(settings, "lens_flare_intensity")),
    }]


def sequence_rows(actor):
    return [{
        "component": "",
        "component_class": "",
        "level_sequence": safe_str(safe_get(actor, "level_sequence")),
        "auto_play": safe_str(safe_get(actor, "auto_play")),
        "disable_camera_cuts": safe_str(safe_get(actor, "disable_camera_cuts")),
    }]


def main():
    unreal.log(f"[ShowcaseInspect] Loading map: {TARGET_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP):
        raise RuntimeError(f"Failed to load map: {TARGET_MAP}")

    rows = []
    actors = get_actor_subsystem().get_all_level_actors()
    for actor in actors:
        class_name = actor.get_class().get_name()
        if class_name not in RENDER_CLASSES:
            continue

        if class_name == "PostProcessVolume":
            detail_rows = postprocess_rows(actor)
        elif class_name == "LevelSequenceActor":
            detail_rows = sequence_rows(actor)
        else:
            detail_rows = component_summary(actor) or [{}]

        for details in detail_rows:
            row = {
                "map": TARGET_MAP,
                "label": actor_label(actor),
                "actor_name": actor.get_name(),
                "class": class_name,
                "location": vec_text(actor.get_actor_location()),
                "hidden_ed": safe_str(actor.is_hidden_ed()),
                "actor_hidden_in_game": safe_str(safe_get(actor, "hidden_in_game")),
                "actor_hidden": safe_str(safe_get(actor, "hidden")),
                "actor_collision": safe_str(actor.get_actor_enable_collision()),
            }
            row.update(details)
            rows.append(row)

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    fieldnames = sorted({key for row in rows for key in row.keys()})
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    unreal.log(f"[ShowcaseInspect] Wrote {len(rows)} row(s): {output_path}")


if __name__ == "__main__":
    main()
