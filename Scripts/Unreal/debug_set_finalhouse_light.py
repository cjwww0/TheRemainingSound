import json
import os

import unreal


FINALHOUSE_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUTPUT_PATH = os.path.join(PROJECT_DIR, "Saved", "DebugSetFinalHouseLight.json")


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


def component_state(component):
    return {
        "name": component.get_name(),
        "class": component.get_class().get_name(),
        "visible": safe_get(component, "visible"),
        "hidden_in_game": safe_get(component, "hidden_in_game"),
        "intensity": safe_get(component, "intensity"),
        "affects_world": safe_get(component, "affects_world"),
    }


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(FINALHOUSE_MAP):
        raise RuntimeError(f"Failed to load map: {FINALHOUSE_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    result = {}

    target = None
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name() == "DirectionalLight":
            target = actor
            break

    if target is None:
        result["error"] = "No DirectionalLight found"
    else:
        result["actor"] = target.get_path_name()
        light_components = [
            component
            for component in target.get_components_by_class(unreal.ActorComponent)
            if component.get_class().get_name() == "DirectionalLightComponent"
        ]
        result["before"] = [component_state(component) for component in light_components]
        result["set_results"] = []
        for component in light_components:
            result["set_results"].append({
                "visible": safe_set(component, "visible", False),
                "hidden_in_game": safe_set(component, "hidden_in_game", True),
                "intensity": safe_set(component, "intensity", 0.0),
                "affects_world": safe_set(component, "affects_world", False),
            })
            if hasattr(component, "set_visibility"):
                component.set_visibility(False, True)
            if hasattr(component, "set_hidden_in_game"):
                component.set_hidden_in_game(True, True)
            if hasattr(component, "set_intensity"):
                component.set_intensity(0.0)
            if hasattr(component, "post_edit_change"):
                component.post_edit_change()
        if hasattr(target, "post_edit_change"):
            target.post_edit_change()
        result["after_same_process"] = [component_state(component) for component in light_components]
        save_results = []
        for label, obj in [
            ("target_actor", target),
            ("target_outermost", target.get_outermost()),
            ("light_component", light_components[0] if light_components else None),
            ("light_component_outermost", light_components[0].get_outermost() if light_components else None),
        ]:
            if obj is None:
                continue
            try:
                saved = unreal.EditorAssetLibrary.save_loaded_asset(obj, False)
            except Exception as exc:
                saved = f"<save failed: {exc}>"
            save_results.append({"label": label, "object": str(obj), "result": saved})
        result["save_loaded_asset_results"] = save_results

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log_warning(f"[SceneRepair] Wrote debug light set report to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
