import csv
import os

import unreal


TARGET_MAPS = [
    "/Game/HorrorMechanics/Demo/Maps/DemoScene_01",
    "/Game/HorrorMechanics/Demo/Maps/FinalHouse",
    "/Game/Anemoia/MAIN/Maps/Demonstration",
    "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop",
    "/Game/VintageRoom/Maps/Showcase",
]

OUTPUT_NAME = "render_balance_report.csv"

INTERESTING_ACTOR_CLASSES = {
    "PostProcessVolume",
    "DirectionalLight",
    "SkyLight",
    "PointLight",
    "SpotLight",
    "RectLight",
    "ExponentialHeightFog",
    "SkyAtmosphere",
    "VolumetricCloud",
    "LevelInstance",
    "PackedLevelActor",
}


def safe_get(obj, *names):
    if obj is None:
        return ""
    for name in names:
        try:
            value = obj.get_editor_property(name)
            return "" if value is None else value
        except Exception:
            pass
    return ""


def safe_str(value):
    if value is None:
        return ""
    if value == "":
        return ""
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec(value):
    if not value:
        return ""
    return f"{value.x:.3f},{value.y:.3f},{value.z:.3f}"


def level_name(actor):
    try:
        return actor.get_level().get_path_name()
    except Exception:
        return ""


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def first_component(actor, class_name_fragments):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        component_class = component.get_class().get_name()
        if any(fragment in component_class for fragment in class_name_fragments):
            return component
    return None


def ppv_details(actor):
    settings = safe_get(actor, "settings")
    return {
        "enabled": safe_str(safe_get(actor, "enabled", "b_enabled")),
        "unbound": safe_str(safe_get(actor, "unbound", "b_unbound")),
        "priority": safe_str(safe_get(actor, "priority")),
        "blend_radius": safe_str(safe_get(actor, "blend_radius")),
        "blend_weight": safe_str(safe_get(actor, "blend_weight")),
        "auto_exposure_method": safe_str(safe_get(settings, "auto_exposure_method")),
        "auto_exposure_bias": safe_str(safe_get(settings, "auto_exposure_bias")),
        "auto_exposure_min": safe_str(safe_get(settings, "auto_exposure_min_brightness", "auto_exposure_min_ev100")),
        "auto_exposure_max": safe_str(safe_get(settings, "auto_exposure_max_brightness", "auto_exposure_max_ev100")),
        "bloom_intensity": safe_str(safe_get(settings, "bloom_intensity")),
        "lens_flare_intensity": safe_str(safe_get(settings, "lens_flare_intensity")),
        "color_gain": safe_str(safe_get(settings, "color_gain")),
        "vignette_intensity": safe_str(safe_get(settings, "vignette_intensity")),
    }


def light_details(actor):
    component = first_component(actor, ["LightComponent", "SkyLightComponent"])
    if component is None:
        return {}
    return {
        "component": component.get_name(),
        "component_class": component.get_class().get_name(),
        "intensity": safe_str(safe_get(component, "intensity")),
        "indirect_lighting_intensity": safe_str(safe_get(component, "indirect_lighting_intensity")),
        "attenuation_radius": safe_str(safe_get(component, "attenuation_radius")),
        "source_radius": safe_str(safe_get(component, "source_radius")),
        "source_length": safe_str(safe_get(component, "source_length")),
        "outer_cone_angle": safe_str(safe_get(component, "outer_cone_angle")),
        "temperature": safe_str(safe_get(component, "temperature")),
        "use_temperature": safe_str(safe_get(component, "use_temperature")),
        "affects_world": safe_str(safe_get(component, "affects_world")),
        "visible": safe_str(safe_get(component, "visible")),
        "mobility": safe_str(safe_get(component, "mobility")),
        "light_color": safe_str(safe_get(component, "light_color")),
    }


def fog_details(actor):
    component = first_component(actor, ["ExponentialHeightFogComponent"])
    if component is None:
        return {}
    return {
        "component": component.get_name(),
        "component_class": component.get_class().get_name(),
        "fog_density": safe_str(safe_get(component, "fog_density")),
        "fog_height_falloff": safe_str(safe_get(component, "fog_height_falloff")),
        "fog_max_opacity": safe_str(safe_get(component, "fog_max_opacity")),
        "start_distance": safe_str(safe_get(component, "start_distance")),
        "directional_inscattering_exponent": safe_str(safe_get(component, "directional_inscattering_exponent")),
        "directional_inscattering_start_distance": safe_str(safe_get(component, "directional_inscattering_start_distance")),
    }


def level_instance_details(actor):
    return {
        "world_asset": safe_str(safe_get(actor, "world_asset", "world_asset_package")),
        "desired_runtime_behavior": safe_str(safe_get(actor, "desired_runtime_behavior")),
        "level_instance_guid": safe_str(safe_get(actor, "level_instance_guid")),
    }


def row_for_actor(map_path, actor):
    class_name = actor.get_class().get_name()
    row = {
        "map": map_path,
        "loaded_level": level_name(actor),
        "label": label(actor),
        "actor_name": actor.get_name(),
        "class": class_name,
        "class_path": actor.get_class().get_path_name(),
        "location": vec(actor.get_actor_location()),
        "rotation": safe_str(actor.get_actor_rotation()),
        "scale": vec(actor.get_actor_scale3d()),
        "hidden_ed": safe_str(actor.is_hidden_ed()),
        "actor_hidden_in_game": safe_str(safe_get(actor, "hidden_in_game")),
        "actor_collision": safe_str(actor.get_actor_enable_collision()),
    }

    if class_name == "PostProcessVolume":
        row.update(ppv_details(actor))
    elif "Light" in class_name:
        row.update(light_details(actor))
    elif class_name == "ExponentialHeightFog":
        row.update(fog_details(actor))
    elif class_name in {"LevelInstance", "PackedLevelActor"}:
        row.update(level_instance_details(actor))

    return row


def dump_map(map_path):
    unreal.log(f"[RenderBalance] Loading map: {map_path}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        unreal.log_warning(f"[RenderBalance] Failed to load map: {map_path}")
        return []

    rows = []
    actors = actor_subsystem().get_all_level_actors()
    unreal.log(f"[RenderBalance] {map_path}: {len(actors)} actor(s)")
    for actor in actors:
        class_name = actor.get_class().get_name()
        if class_name in INTERESTING_ACTOR_CLASSES or "Light" in class_name:
            rows.append(row_for_actor(map_path, actor))
    return rows


def main():
    rows = []
    for map_path in TARGET_MAPS:
        rows.extend(dump_map(map_path))

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)

    fieldnames = sorted({key for row in rows for key in row.keys()})
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    unreal.log(f"[RenderBalance] Wrote {len(rows)} row(s): {output_path}")


if __name__ == "__main__":
    main()
