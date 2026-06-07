import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse.FinalHouse"
TARGET_LABEL = "FinalHouse"


def log(message):
    unreal.log(f"[SceneRepair] {message}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    finalhouse_world = unreal.EditorAssetLibrary.load_asset(FINALHOUSE_MAP)
    if finalhouse_world is None:
        raise RuntimeError(f"Failed to load FinalHouse map asset: {FINALHOUSE_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    changed = 0

    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() != TARGET_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue

        actor.set_world_asset(finalhouse_world)
        actor.set_editor_property("hidden", False)
        actor.set_actor_enable_collision(True)
        log(f"Restored {TARGET_LABEL} LevelInstance world_asset={FINALHOUSE_MAP}")
        changed += 1

    if changed == 0:
        raise RuntimeError(f"No LevelInstance actor labeled {TARGET_LABEL} found in {MAP_PATH}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Saved {changed} restored LevelInstance change(s) in DemoScene_01")


if __name__ == "__main__":
    main()
