import json
import os

import unreal


FINALHOUSE_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
REPORT_PATH = os.path.join(PROJECT_DIR, "Saved", "RemovedFinalHouseGlobalActors.json")

REMOVE_CLASSES = {
    "DirectionalLight",
    "SkyLight",
    "SkyAtmosphere",
    "ExponentialHeightFog",
    "PostProcessVolume",
    "VolumetricCloud",
    "SunSky",
}


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(FINALHOUSE_MAP):
        raise RuntimeError(f"Failed to load map: {FINALHOUSE_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    removed = []

    for actor in list(actor_subsystem.get_all_level_actors()):
        class_name = actor.get_class().get_name()
        if class_name not in REMOVE_CLASSES:
            continue

        removed.append({
            "label": actor.get_actor_label(),
            "name": actor.get_name(),
            "class": class_name,
            "path": actor.get_path_name(),
        })
        actor_subsystem.destroy_actor(actor)

    os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        json.dump(removed, handle, ensure_ascii=False, indent=2)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log_warning(f"[SceneRepair] Removed {len(removed)} FinalHouse global render actor(s). Report={REPORT_PATH}")


if __name__ == "__main__":
    main()
