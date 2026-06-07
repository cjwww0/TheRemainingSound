import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
TARGET_LABEL = "FinalHouse"


def log(message):
    unreal.log(f"[SceneRepair] {message}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    changed = 0

    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() != TARGET_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue

        old_asset = actor.get_editor_property("world_asset")
        log(f"Detaching {TARGET_LABEL} LevelInstance. Previous world_asset={old_asset}")
        actor.set_world_asset(None)
        actor.set_editor_property("hidden", True)
        actor.set_actor_enable_collision(False)
        changed += 1

    if changed == 0:
        raise RuntimeError(f"No LevelInstance actor labeled {TARGET_LABEL} found in {MAP_PATH}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Saved {changed} detached LevelInstance change(s) in DemoScene_01")


if __name__ == "__main__":
    main()
