import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
TARGET_LABEL = "FinalHouse"


def log(message):
    unreal.log(f"[LevelInstanceInspect] {message}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() != TARGET_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue

        log(f"Actor: {actor.get_actor_label()} {actor.get_path_name()}")
        log(f"HiddenEd={actor.is_hidden_ed()}")
        log(f"Collision={actor.get_actor_enable_collision()} Location={actor.get_actor_location()}")

        for prop in [
            "world_asset",
            "desired_runtime_behavior",
            "level_instance_spawn_guid",
            "level_instance_actor_guid",
            "hidden",
            "hidden_in_game",
            "is_spatially_loaded",
            "runtime_grid",
        ]:
            try:
                value = actor.get_editor_property(prop)
            except Exception as exc:
                value = f"<read failed: {exc}>"
            log(f"  {prop} = {value}")

        names = [name for name in dir(actor) if any(token in name.lower() for token in ["level", "world", "runtime", "stream", "load"])]
        log("  methods/properties containing keywords:")
        for name in sorted(names):
            log(f"    {name}")


if __name__ == "__main__":
    main()
