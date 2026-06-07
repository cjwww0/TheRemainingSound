import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
TARGET_LABEL = "FinalHouse"


def log(message):
    unreal.log(f"[SceneRepair] {message}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    changed = 0
    for actor in actors:
        if actor.get_actor_label() != TARGET_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue

        actor.set_actor_hidden_in_game(True)
        actor.set_is_temporarily_hidden_in_editor(True)
        actor.set_actor_enable_collision(False)
        log(f"Disabled LevelInstance for isolation: {actor.get_actor_label()} at {actor.get_actor_location()}")
        changed += 1

    if changed == 0:
        raise RuntimeError(f"No LevelInstance actor labeled {TARGET_LABEL} found in {MAP_PATH}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Saved {changed} LevelInstance isolation change(s) in DemoScene_01")


if __name__ == "__main__":
    main()
