import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
CONTROLLER_LABEL = "BP_WorkshopLightFaultController"
PROXY_LIGHT_LABELS = [
    "P15_ProxyLivingRoomFaultLight",
    "P15_ProxyWorkshopFaultLight",
]

PULSE_DURATION = 0.25
MINIMUM_PULSE_INTENSITY = 25000.0
FAULT_INTENSITY = 0.0
NORMAL_INTENSITY = 0.0


def _get_all_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


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
        unreal.log_warning(f"[P15Tune] Multiple actors share label '{label}', using {matches[0].get_path_name()}")
    return matches[0]


def _set_property(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            pass
    return False


def _get_point_light(actor):
    if not actor:
        return None
    component = actor.get_component_by_class(unreal.PointLightComponent)
    if component:
        return component
    return actor.find_component_by_class(unreal.PointLightComponent)


def _make_light_config(light_actor):
    config = unreal.RemainFaultLightConfig()
    if not _set_property(config, ["light_actor", "LightActor"], light_actor):
        raise RuntimeError("Failed to set LightActor on RemainFaultLightConfig")
    if not _set_property(config, ["fault_intensity", "FaultIntensity"], FAULT_INTENSITY):
        raise RuntimeError("Failed to set FaultIntensity on RemainFaultLightConfig")
    _set_property(config, ["b_toggle_visibility", "ToggleVisibility", "bToggleVisibility"], True)
    return config


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "p15_proxy_fault_lights_tuning_report.csv")
    fieldnames = [
        "label",
        "found",
        "normal_intensity",
        "fault_intensity",
        "pulse_duration",
        "minimum_pulse_intensity",
        "message",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[P15Tune] Report written: {output_path}")


def main():
    unreal.log(f"[P15Tune] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    controller = _find_actor(CONTROLLER_LABEL)
    if not controller:
        raise RuntimeError(f"Cannot find controller: {CONTROLLER_LABEL}")

    rows = []
    light_actors = []

    for label in PROXY_LIGHT_LABELS:
        actor = _find_actor(label)
        light = _get_point_light(actor)
        if not actor or not light:
            rows.append({
                "label": label,
                "found": "false",
                "normal_intensity": "",
                "fault_intensity": "",
                "pulse_duration": PULSE_DURATION,
                "minimum_pulse_intensity": MINIMUM_PULSE_INTENSITY,
                "message": "Missing proxy light actor or PointLightComponent",
            })
            continue

        light.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity", NORMAL_INTENSITY)
        light.set_visibility(True)
        actor.set_actor_hidden_in_game(False)
        actor.set_actor_enable_collision(False)
        light_actors.append(actor)

        rows.append({
            "label": label,
            "found": "true",
            "normal_intensity": NORMAL_INTENSITY,
            "fault_intensity": FAULT_INTENSITY,
            "pulse_duration": PULSE_DURATION,
            "minimum_pulse_intensity": MINIMUM_PULSE_INTENSITY,
            "message": "Configured as pulse-only proxy light",
        })

    controller.set_editor_property("controlled_lights", [_make_light_config(actor) for actor in light_actors])
    _set_property(controller, ["fault_pulse_duration", "FaultPulseDuration"], PULSE_DURATION)
    _set_property(controller, ["minimum_fault_pulse_intensity", "MinimumFaultPulseIntensity"], MINIMUM_PULSE_INTENSITY)

    _write_report(rows)
    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[P15Tune] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
