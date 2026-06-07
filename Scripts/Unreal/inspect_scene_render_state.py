import unreal


TARGET_MAPS = [
    "/Game/HorrorMechanics/Demo/Maps/DemoScene_01",
    "/Game/HorrorMechanics/Demo/Maps/FinalHouse",
]


def safe_get(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return None


def fmt_vector(vec):
    if vec is None:
        return "None"
    return f"({vec.x:.3f}, {vec.y:.3f}, {vec.z:.3f})"


def fmt_obj(obj):
    if obj is None:
        return "None"
    return obj.get_path_name()


def log(msg):
    unreal.log(f"[SceneInspect] {msg}")


def inspect_post_process(actor):
    settings = safe_get(actor, "settings")
    log(f"PostProcessVolume label={actor.get_actor_label()}")
    log(f"  unbound={safe_get(actor, 'b_unbound')} enabled={safe_get(actor, 'enabled')} blend_weight={safe_get(actor, 'blend_weight')}")
    if settings is not None:
        log(f"  auto_exposure_method={safe_get(settings, 'auto_exposure_method')}")
        log(f"  auto_exposure_bias={safe_get(settings, 'auto_exposure_bias')}")
        log(f"  camera_iso={safe_get(settings, 'camera_iso')}")
        log(f"  camera_shutter_speed={safe_get(settings, 'camera_shutter_speed')}")
        log(f"  camera_aperture={safe_get(settings, 'camera_aperture')}")
        log(f"  motion_blur_amount={safe_get(settings, 'motion_blur_amount')}")
        log(f"  color_gain={safe_get(settings, 'color_gain')}")
        log(f"  film_slope={safe_get(settings, 'film_slope')}")
        log(f"  film_toe={safe_get(settings, 'film_toe')}")


def inspect_level_sequence(actor):
    sequence = safe_get(actor, "level_sequence")
    log(f"LevelSequenceActor label={actor.get_actor_label()}")
    log(f"  sequence={fmt_obj(sequence)} auto_play={safe_get(actor, 'auto_play')} replicate_playback={safe_get(actor, 'replicate_playback')}")
    log(f"  disable_camera_cuts={safe_get(actor, 'disable_camera_cuts')}")


def inspect_level_instance(actor):
    world_asset = safe_get(actor, "world_asset")
    desired_runtime_behavior = safe_get(actor, "desired_runtime_behavior")
    log(f"LevelInstance label={actor.get_actor_label()}")
    log(f"  world_asset={fmt_obj(world_asset)} runtime_behavior={desired_runtime_behavior}")
    log(f"  location={fmt_vector(actor.get_actor_location())} hidden={safe_get(actor, 'hidden')}")


def inspect_light(actor):
    log(f"Light label={actor.get_actor_label()} class={actor.get_class().get_name()} location={fmt_vector(actor.get_actor_location())}")


def inspect_map(map_path):
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        log(f"Failed to load map {map_path}")
        return

    log(f"=== MAP {map_path} ===")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        class_name = actor.get_class().get_name()
        if class_name == "PostProcessVolume":
            inspect_post_process(actor)
        elif class_name == "LevelSequenceActor":
            inspect_level_sequence(actor)
        elif class_name == "LevelInstance":
            inspect_level_instance(actor)
        elif class_name in {
            "DirectionalLight",
            "SkyLight",
            "PointLight",
            "SpotLight",
            "RectLight",
            "SkyAtmosphere",
            "ExponentialHeightFog",
        }:
            inspect_light(actor)


def main():
    for map_path in TARGET_MAPS:
        inspect_map(map_path)


if __name__ == "__main__":
    main()
