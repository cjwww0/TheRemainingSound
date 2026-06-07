import unreal


FINALHOUSE_MAP = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"
FINALHOUSE_MAP_ASSET = "/Game/HorrorMechanics/Demo/Maps/FinalHouse.FinalHouse"
DEMO_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
LEVEL_INSTANCE_LABEL = "FinalHouse"

GLOBAL_RENDER_CLASSES = {
    "DirectionalLight",
    "SkyLight",
    "SkyAtmosphere",
    "ExponentialHeightFog",
    "VolumetricCloud",
    "SunSky",
}


def log(message):
    unreal.log(f"[SceneRepair] {message}")


def safe_get(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def safe_set(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def component_names(actor):
    try:
        return [component.get_name() for component in actor.get_components_by_class(unreal.ActorComponent)]
    except Exception:
        return []


def set_actor_inactive(actor):
    try:
        actor.modify()
    except Exception:
        pass
    safe_set(actor, "hidden", True)
    safe_set(actor, "hidden_in_game", True)
    try:
        actor.set_actor_hidden_in_game(True)
    except Exception:
        pass
    try:
        actor.set_actor_enable_collision(False)
    except Exception:
        pass

    for component in actor.get_components_by_class(unreal.ActorComponent):
        try:
            component.modify()
        except Exception:
            pass
        if hasattr(component, "set_component_tick_enabled"):
            try:
                component.set_component_tick_enabled(False)
            except Exception:
                pass
        if hasattr(component, "set_visibility"):
            try:
                component.set_visibility(False, True)
            except Exception:
                pass
        if hasattr(component, "set_hidden_in_game"):
            try:
                component.set_hidden_in_game(True, True)
            except Exception:
                pass
        safe_set(component, "visible", False)
        safe_set(component, "b_visible", False)
        safe_set(component, "hidden_in_game", True)
        safe_set(component, "b_hidden_in_game", True)
        safe_set(component, "intensity", 0.0)
        safe_set(component, "affects_world", False)
        safe_set(component, "b_affects_world", False)

        if hasattr(component, "set_intensity"):
            try:
                component.set_intensity(0.0)
            except Exception:
                pass
        if hasattr(component, "set_brightness"):
            try:
                component.set_brightness(0.0)
            except Exception:
                pass
        if hasattr(component, "post_edit_change"):
            try:
                component.post_edit_change()
            except Exception:
                pass

    if hasattr(actor, "post_edit_change"):
        try:
            actor.post_edit_change()
        except Exception:
            pass
    try:
        unreal.EditorAssetLibrary.save_loaded_asset(actor, False)
    except Exception as exc:
        log(f"Failed to save external actor {actor.get_actor_label()}: {exc}")


def disable_post_process(actor):
    safe_set(actor, "enabled", False)
    safe_set(actor, "b_enabled", False)
    safe_set(actor, "blend_weight", 0.0)
    safe_set(actor, "unbound", False)
    safe_set(actor, "b_unbound", False)
    set_actor_inactive(actor)


def disable_level_sequence(actor):
    # Prevent imported showcase sequences from taking camera or exposure control in PIE.
    safe_set(actor, "auto_play", False)
    safe_set(actor, "disable_camera_cuts", True)


def sanitize_finalhouse():
    if not unreal.EditorLoadingAndSavingUtils.load_map(FINALHOUSE_MAP):
        raise RuntimeError(f"Failed to load map: {FINALHOUSE_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()
    disabled = []
    sequence_count = 0

    for actor in actors:
        class_name = actor.get_class().get_name()
        label = actor.get_actor_label()

        if class_name in GLOBAL_RENDER_CLASSES:
            set_actor_inactive(actor)
            disabled.append(f"{label}<{class_name}>")
            continue

        if class_name == "PostProcessVolume":
            disable_post_process(actor)
            disabled.append(f"{label}<PostProcessVolume>")
            continue

        if class_name == "LevelSequenceActor":
            disable_level_sequence(actor)
            sequence_count += 1

    log(f"Sanitized FinalHouse global render actors: {len(disabled)}")
    for item in disabled:
        log(f"  disabled {item}")
    if sequence_count:
        log(f"Disabled autoplay/camera cuts on {sequence_count} LevelSequenceActor(s)")


def restore_level_instance():
    if not unreal.EditorLoadingAndSavingUtils.load_map(DEMO_MAP):
        raise RuntimeError(f"Failed to load map: {DEMO_MAP}")

    finalhouse_world = unreal.EditorAssetLibrary.load_asset(FINALHOUSE_MAP_ASSET)
    if finalhouse_world is None:
        raise RuntimeError(f"Failed to load map asset: {FINALHOUSE_MAP_ASSET}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    restored = 0

    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() != LEVEL_INSTANCE_LABEL:
            continue
        if actor.get_class().get_name() != "LevelInstance":
            continue

        actor.set_world_asset(finalhouse_world)
        safe_set(actor, "hidden", False)
        try:
            actor.set_actor_enable_collision(True)
        except Exception:
            pass
        restored += 1
        log(f"Restored LevelInstance {LEVEL_INSTANCE_LABEL} -> {FINALHOUSE_MAP_ASSET}")

    if restored == 0:
        raise RuntimeError(f"No LevelInstance labeled {LEVEL_INSTANCE_LABEL} found in {DEMO_MAP}")


def main():
    sanitize_finalhouse()
    restore_level_instance()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved sanitized FinalHouse and restored DemoScene_01 LevelInstance")


if __name__ == "__main__":
    main()
