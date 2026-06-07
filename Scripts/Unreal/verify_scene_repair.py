import json
import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUTPUT_PATH = os.path.join(PROJECT_DIR, "Saved", "SceneRepairVerification.json")
DEMO_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"
LEVEL_INSTANCE_LABEL = "FinalHouse"

GLOBAL_RENDER_CLASSES = {
    "DirectionalLight",
    "SkyLight",
    "SkyAtmosphere",
    "ExponentialHeightFog",
    "VolumetricCloud",
    "SunSky",
    "PostProcessVolume",
    "LevelSequenceActor",
}


def safe_get(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def object_path(value):
    if value is None:
        return None
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def actor_summary(actor):
    data = {
        "label": actor.get_actor_label(),
        "name": actor.get_name(),
        "class": actor.get_class().get_name(),
        "hidden_ed": bool(actor.is_hidden_ed()),
        "hidden": bool(safe_get(actor, "hidden", False)),
        "hidden_in_game": bool(safe_get(actor, "hidden_in_game", False)),
        "collision": bool(actor.get_actor_enable_collision()),
    }
    components = []
    for component in actor.get_components_by_class(unreal.ActorComponent):
        component_data = {
            "name": component.get_name(),
            "class": component.get_class().get_name(),
            "visible": safe_get(component, "visible"),
            "hidden_in_game": safe_get(component, "hidden_in_game"),
            "intensity": safe_get(component, "intensity"),
            "affects_world": safe_get(component, "affects_world"),
        }
        if any(value is not None for key, value in component_data.items() if key not in {"name", "class"}):
            components.append(component_data)
    data["components"] = components
    return data


def inspect_demo_levelinstance():
    if not unreal.EditorLoadingAndSavingUtils.load_map(DEMO_MAP):
        raise RuntimeError(f"Failed to load map: {DEMO_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    matches = []
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() != LEVEL_INSTANCE_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue
        data = actor_summary(actor)
        data["world_asset"] = object_path(safe_get(actor, "world_asset"))
        data["desired_runtime_behavior"] = str(safe_get(actor, "desired_runtime_behavior"))
        matches.append(data)
    return matches


def inspect_finalhouse_globals():
    if not unreal.EditorLoadingAndSavingUtils.load_map(FINALHOUSE_MAP):
        raise RuntimeError(f"Failed to load map: {FINALHOUSE_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    globals_found = []
    for actor in actor_subsystem.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if class_name not in GLOBAL_RENDER_CLASSES:
            continue
        data = actor_summary(actor)
        if class_name == "PostProcessVolume":
            data["enabled"] = bool(safe_get(actor, "enabled", False))
            data["blend_weight"] = float(safe_get(actor, "blend_weight", 0.0) or 0.0)
        if class_name == "LevelSequenceActor":
            data["auto_play"] = bool(safe_get(actor, "auto_play", False))
            data["disable_camera_cuts"] = bool(safe_get(actor, "disable_camera_cuts", False))
        globals_found.append(data)
    return globals_found


def main():
    result = {
        "demo_levelinstances": inspect_demo_levelinstance(),
        "finalhouse_global_render_actors": inspect_finalhouse_globals(),
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)

    unreal.log_warning(f"[SceneRepair] Wrote verification to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
