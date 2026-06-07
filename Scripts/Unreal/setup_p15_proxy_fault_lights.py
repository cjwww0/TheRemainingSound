import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FINALHOUSE_LABEL = "FinalHouse"
CONTROLLER_LABEL = "BP_WorkshopLightFaultController"
TARGET_FOLDER = "Gameplay_Logic/03_Workbench"


PROXY_LIGHTS = [
    {
        "label": "P15_ProxyLivingRoomFaultLight",
        # Near the first-floor table/living room area.
        "local_location": (1305.0, 535.0, 1215.0),
        "intensity": 4200.0,
        "fault_intensity": 900.0,
        "attenuation_radius": 900.0,
        "color": unreal.Color(255, 199, 122, 255),
    },
    {
        "label": "P15_ProxyWorkshopFaultLight",
        # Near the current upstairs workbench placement area.
        "local_location": (-300.0, 900.0, 1845.0),
        "intensity": 3600.0,
        "fault_intensity": 650.0,
        "attenuation_radius": 850.0,
        "color": unreal.Color(255, 179, 107, 255),
    },
]


def _get_all_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _find_actor(label):
    matches = [actor for actor in _get_all_actors() if _label(actor) == label]
    if not matches:
        return None
    if len(matches) > 1:
        unreal.log_warning(f"[P15ProxyLights] Multiple actors share label '{label}', using {matches[0].get_path_name()}")
    return matches[0]


def _world_from_finalhouse(finalhouse_origin, local_xyz):
    return unreal.Vector(
        finalhouse_origin.x + local_xyz[0],
        finalhouse_origin.y + local_xyz[1],
        finalhouse_origin.z + local_xyz[2],
    )


def _vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}"


def _set_folder(actor, folder):
    try:
        actor.set_folder_path(folder)
    except Exception:
        actor.set_folder_path(unreal.Name(folder))


def _configure_point_light(actor, config):
    component = actor.get_component_by_class(unreal.PointLightComponent)
    if not component:
        component = actor.find_component_by_class(unreal.PointLightComponent)
    if not component:
        raise RuntimeError(f"PointLightComponent missing on {config['label']}")

    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", config["intensity"])
    component.set_editor_property("attenuation_radius", config["attenuation_radius"])
    component.set_editor_property("light_color", config["color"])
    component.set_visibility(True)
    actor.set_actor_hidden_in_game(False)
    actor.set_actor_enable_collision(False)


def _spawn_or_update_light(config, finalhouse_origin):
    target_location = _world_from_finalhouse(finalhouse_origin, config["local_location"])
    actor = _find_actor(config["label"])
    status = "updated"

    if not actor:
        subsystem = _actor_subsystem()
        if subsystem:
            actor = subsystem.spawn_actor_from_class(unreal.PointLight, target_location, unreal.Rotator(0.0, 0.0, 0.0))
        else:
            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PointLight, target_location, unreal.Rotator(0.0, 0.0, 0.0))
        actor.set_actor_label(config["label"], mark_dirty=True)
        status = "created"

    actor.set_actor_location(target_location, False, True)
    _set_folder(actor, TARGET_FOLDER)
    _configure_point_light(actor, config)
    return actor, status


def _set_struct_property(struct_obj, names, value):
    for name in names:
        try:
            struct_obj.set_editor_property(name, value)
            return True
        except Exception:
            pass
    return False


def _make_light_config(light_actor, fault_intensity):
    config = unreal.RemainFaultLightConfig()
    if not _set_struct_property(config, ["light_actor", "LightActor"], light_actor):
        raise RuntimeError("Failed to set LightActor on RemainFaultLightConfig")
    if not _set_struct_property(config, ["fault_intensity", "FaultIntensity"], fault_intensity):
        raise RuntimeError("Failed to set FaultIntensity on RemainFaultLightConfig")
    _set_struct_property(config, ["b_toggle_visibility", "ToggleVisibility", "bToggleVisibility"], True)
    return config


def _set_controller_lights(controller, light_actors):
    configs = []
    for actor, proxy_config in light_actors:
        configs.append(_make_light_config(actor, proxy_config["fault_intensity"]))

    try:
        controller.set_editor_property("controlled_lights", configs)
    except Exception:
        controller.set_editor_property("ControlledLights", configs)

    try:
        controller.set_editor_property("flash_duration", 1.0)
    except Exception:
        pass


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p15_proxy_fault_lights_report.csv")
    fieldnames = ["label", "status", "location", "intensity", "fault_intensity", "attenuation_radius"]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P15ProxyLights] Report written: {output_path}")


def main():
    unreal.log(f"[P15ProxyLights] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    finalhouse = _find_actor(FINALHOUSE_LABEL)
    if not finalhouse:
        raise RuntimeError(f"Cannot find LevelInstance actor label '{FINALHOUSE_LABEL}'")

    controller = _find_actor(CONTROLLER_LABEL)
    if not controller:
        raise RuntimeError(f"Cannot find {CONTROLLER_LABEL}")

    rows = []
    spawned_lights = []
    finalhouse_origin = finalhouse.get_actor_location()

    for proxy_config in PROXY_LIGHTS:
        actor, status = _spawn_or_update_light(proxy_config, finalhouse_origin)
        spawned_lights.append((actor, proxy_config))
        rows.append({
            "label": proxy_config["label"],
            "status": status,
            "location": _vec_text(actor.get_actor_location()),
            "intensity": proxy_config["intensity"],
            "fault_intensity": proxy_config["fault_intensity"],
            "attenuation_radius": proxy_config["attenuation_radius"],
        })

    _set_controller_lights(controller, spawned_lights)
    _write_report(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[P15ProxyLights] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
