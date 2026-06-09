import csv
import os

import unreal


MAP_PATH = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
FLASHLIGHT_LABEL = "TRS_PlayerFlashlight"
FOG_LABEL = "TRS_FlashlightVolumetricFog"


FLASHLIGHT_VALUES = {
    "b_start_enabled": True,
    "b_bind_toggle_input": True,
    "toggle_action_name": unreal.Name("FlashlightToggle"),
    "intensity": 50000.0,
    "attenuation_radius": 3600.0,
    "inner_cone_angle": 8.0,
    "outer_cone_angle": 30.0,
    "volumetric_scattering_intensity": 8.0,
    "source_radius": 1.5,
    "soft_source_radius": 24.0,
    "b_use_temperature": True,
    "temperature": 4300.0,
    "camera_relative_location": unreal.Vector(10.0, 0.0, -4.0),
    "camera_relative_rotation": unreal.Rotator(0.0, 0.0, 0.0),
}


SPOTLIGHT_VALUES = {
    "intensity": 50000.0,
    "attenuation_radius": 3600.0,
    "inner_cone_angle": 8.0,
    "outer_cone_angle": 30.0,
    "volumetric_scattering_intensity": 8.0,
    "source_radius": 1.5,
    "soft_source_radius": 24.0,
    "use_temperature": True,
    "temperature": 4300.0,
}


FOG_VALUES = {
    "volumetric_fog": True,
    "enable_volumetric_fog": True,
    "b_enable_volumetric_fog": True,
    "fog_density": 0.018,
    "volumetric_fog_distance": 4500.0,
    "volumetric_fog_scattering_distribution": 0.25,
    "volumetric_fog_extinction_scale": 0.7,
}


def _actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.get_all_level_actors()


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def _find(label):
    for actor in _actors():
        if _label(actor) == label:
            return actor
    return None


def _set_if_present(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def _class_name(obj):
    try:
        return obj.get_class().get_name()
    except Exception:
        return obj.__class__.__name__


def _apply_properties(target, values):
    applied = []
    skipped = []
    for name, value in values.items():
        if _set_if_present(target, name, value):
            applied.append(name)
        else:
            skipped.append(name)
    return applied, skipped


def _tune_flashlight(rows):
    actor = _find(FLASHLIGHT_LABEL)
    if not actor:
        rows.append({
            "target": FLASHLIGHT_LABEL,
            "status": "FAIL",
            "applied": "",
            "skipped": "",
            "message": "Actor not found. Run setup_player_flashlight_and_door_proxy.py first.",
        })
        return

    actor.modify()
    applied, skipped = _apply_properties(actor, FLASHLIGHT_VALUES)
    rows.append({
        "target": FLASHLIGHT_LABEL,
        "status": "PASS",
        "applied": ";".join(applied),
        "skipped": ";".join(skipped),
        "message": "Actor flashlight values updated",
    })

    components = actor.get_components_by_class(unreal.ActorComponent)
    for component in components:
        if "SpotLight" not in _class_name(component):
            continue
        component.modify()
        comp_applied, comp_skipped = _apply_properties(component, SPOTLIGHT_VALUES)
        try:
            component.set_visibility(True, True)
            component.set_active(True)
        except Exception:
            pass
        rows.append({
            "target": f"{FLASHLIGHT_LABEL}.{component.get_name()}",
            "status": "PASS",
            "applied": ";".join(comp_applied),
            "skipped": ";".join(comp_skipped),
            "message": "SpotLight component values updated",
        })


def _tune_height_fog(rows):
    fog_count = 0
    for actor in _actors():
        if "ExponentialHeightFog" not in _class_name(actor):
            continue
        fog_count += 1
        actor.modify()
        applied, skipped = _apply_properties(actor, FOG_VALUES)
        rows.append({
            "target": _label(actor),
            "status": "PASS",
            "applied": ";".join(applied),
            "skipped": ";".join(skipped),
            "message": "Height fog actor checked",
        })

        for component in actor.get_components_by_class(unreal.ActorComponent):
            if "ExponentialHeightFog" not in _class_name(component):
                continue
            component.modify()
            comp_applied, comp_skipped = _apply_properties(component, FOG_VALUES)
            rows.append({
                "target": f"{_label(actor)}.{component.get_name()}",
                "status": "PASS",
                "applied": ";".join(comp_applied),
                "skipped": ";".join(comp_skipped),
                "message": "Height fog component checked",
            })

    if fog_count == 0:
        fog_class = unreal.load_class(None, "/Script/Engine.ExponentialHeightFog")
        if fog_class:
            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
                fog_class,
                unreal.Vector(0.0, 0.0, 0.0),
                unreal.Rotator(0.0, 0.0, 0.0),
            )
            actor.set_actor_label(FOG_LABEL, mark_dirty=True)
            try:
                actor.set_folder_path("Gameplay_Logic/00_Player")
            except Exception:
                pass
            actor.modify()
            applied, skipped = _apply_properties(actor, FOG_VALUES)
            rows.append({
                "target": FOG_LABEL,
                "status": "PASS",
                "applied": ";".join(applied),
                "skipped": ";".join(skipped),
                "message": "Spawned low-density height fog for flashlight beam visibility",
            })
            for component in actor.get_components_by_class(unreal.ActorComponent):
                if "ExponentialHeightFog" not in _class_name(component):
                    continue
                component.modify()
                comp_applied, comp_skipped = _apply_properties(component, FOG_VALUES)
                rows.append({
                    "target": f"{FOG_LABEL}.{component.get_name()}",
                    "status": "PASS",
                    "applied": ";".join(comp_applied),
                    "skipped": ";".join(comp_skipped),
                    "message": "Spawned height fog component checked",
                })
            return

        rows.append({
            "target": "ExponentialHeightFog",
            "status": "WARN",
            "applied": "",
            "skipped": "",
            "message": "No ExponentialHeightFog actor found and /Script/Engine.ExponentialHeightFog could not be loaded.",
        })


def _write_report(rows):
    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "player_flashlight_visibility_tuning.csv")
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["target", "status", "applied", "skipped", "message"])
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[FlashlightVisibility] Report written: {output_path}")


def main():
    unreal.log(f"[FlashlightVisibility] Loading map: {MAP_PATH}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    rows = []
    _tune_flashlight(rows)
    _tune_height_fog(rows)
    _write_report(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[FlashlightVisibility] save_dirty_packages returned: {saved}")


if __name__ == "__main__":
    main()
