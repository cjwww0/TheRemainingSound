import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"


def log(message):
    unreal.log(f"[SceneRepair] {message}")


def set_if_exists(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            continue
    return False


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    changed = 0
    for actor in actors:
        if actor.get_class().get_name() != "PostProcessVolume":
            continue

        set_if_exists(actor, ["enabled", "b_enabled"], False)
        set_if_exists(actor, ["blend_weight"], 0.0)
        set_if_exists(actor, ["unbound", "b_unbound"], False)

        log(f"Disabled imported PostProcessVolume: {actor.get_actor_label()}")
        changed += 1

    if changed == 0:
        raise RuntimeError("No PostProcessVolume found in FinalHouse")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Saved {changed} PostProcessVolume change(s) in FinalHouse")


if __name__ == "__main__":
    main()
