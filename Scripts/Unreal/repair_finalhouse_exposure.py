import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/FinalHouse"


def log(msg):
    unreal.log(f"[SceneRepair] {msg}")


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    repaired = False
    for actor in actors:
        if actor.get_class().get_name() != "PostProcessVolume":
            continue

        settings = actor.get_editor_property("settings")
        try:
            actor.set_editor_property("unbound", False)
        except Exception:
            pass

        settings.set_editor_property(
            "auto_exposure_method",
            unreal.AutoExposureMethod.AEM_HISTOGRAM,
        )
        settings.set_editor_property("auto_exposure_bias", 0.0)
        settings.set_editor_property("camera_iso", 100.0)
        settings.set_editor_property("camera_shutter_speed", 60.0)
        actor.set_editor_property("settings", settings)
        repaired = True
        log(
            "Adjusted PostProcessVolume to gameplay-safe exposure "
            "(Histogram, bias 0.0, unbound false if available)"
        )

    if not repaired:
        raise RuntimeError("No PostProcessVolume found in FinalHouse")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved updated FinalHouse map")


if __name__ == "__main__":
    main()
