import csv
import os

import unreal


PERSISTENT_MAP = "/Game/HorrorMechanics/Demo/Maps/DemoScene_01"
OUTPUT_NAME = "workshop_local_lights_report.csv"

SHOWCASE_LABEL = "Showcase"
DISABLE_PPV_LABEL = "TRS_WorkshopLocal_PostProcess"

LIGHTS = [
    {
        "label": "TRS_Workshop_CeilingSoftLight",
        "offset": (0.0, 0.0, 380.0),
        "intensity": 2600.0,
        "attenuation_radius": 850.0,
        "source_radius": 220.0,
        "color": unreal.Color(186, 214, 255, 255),
        "cast_shadows": False,
    },
    {
        "label": "TRS_Workshop_TableWarmLight",
        "offset": (-360.0, -260.0, 120.0),
        "intensity": 1800.0,
        "attenuation_radius": 620.0,
        "source_radius": 90.0,
        "color": unreal.Color(255, 178, 112, 255),
        "cast_shadows": True,
    },
    {
        "label": "TRS_Workshop_BackColdRimLight",
        "offset": (420.0, 360.0, 190.0),
        "intensity": 1100.0,
        "attenuation_radius": 700.0,
        "source_radius": 120.0,
        "color": unreal.Color(145, 190, 255, 255),
        "cast_shadows": False,
    },
]


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def find_actor_by_label(actor_subsystem, target_label):
    for actor in actor_subsystem.get_all_level_actors():
        if label(actor) == target_label:
            return actor
    return None


def first_component(actor, component_name_fragment):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component_name_fragment in component.get_class().get_name():
            return component
    return None


def set_if_exists(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            pass
    return ""


def disable_post_process_volume(actor):
    if actor is None:
        return "missing"
    set_if_exists(actor, ["enabled", "b_enabled"], False)
    set_if_exists(actor, ["blend_weight"], 0.0)
    actor.set_actor_hidden_in_game(True)
    return "disabled"


def spawn_or_update_point_light(actor_subsystem, config, base_location):
    actor = find_actor_by_label(actor_subsystem, config["label"])
    status = "updated"
    location = base_location + unreal.Vector(*config["offset"])

    if actor is None:
        actor = actor_subsystem.spawn_actor_from_class(
            unreal.PointLight,
            location,
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        actor.set_actor_label(config["label"], mark_dirty=True)
        status = "created"

    actor.set_actor_location(location, False, True)
    actor.set_actor_hidden_in_game(False)
    actor.set_actor_enable_collision(False)

    component = first_component(actor, "PointLightComponent")
    if component is None:
        raise RuntimeError(f"PointLightComponent missing on {config['label']}")

    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", config["intensity"])
    component.set_editor_property("attenuation_radius", config["attenuation_radius"])
    component.set_editor_property("source_radius", config["source_radius"])
    component.set_editor_property("light_color", config["color"])
    set_if_exists(component, ["cast_shadows"], config["cast_shadows"])
    set_if_exists(component, ["visible"], True)

    return actor, status


def main():
    unreal.log(f"[WorkshopLights] Loading map: {PERSISTENT_MAP}")
    if not unreal.EditorLoadingAndSavingUtils.load_map(PERSISTENT_MAP):
        raise RuntimeError(f"Failed to load map: {PERSISTENT_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    showcase = find_actor_by_label(actor_subsystem, SHOWCASE_LABEL)
    if showcase is None:
        raise RuntimeError(f"LevelInstance not found: {SHOWCASE_LABEL}")

    ppv_status = disable_post_process_volume(find_actor_by_label(actor_subsystem, DISABLE_PPV_LABEL))

    rows = []
    base_location = showcase.get_actor_location()
    for config in LIGHTS:
        actor, status = spawn_or_update_point_light(actor_subsystem, config, base_location)
        rows.append({
            "label": config["label"],
            "status": status,
            "location": vec_text(actor.get_actor_location()),
            "intensity": config["intensity"],
            "attenuation_radius": config["attenuation_radius"],
            "source_radius": config["source_radius"],
            "cast_shadows": config["cast_shadows"],
            "ppv_status": ppv_status,
        })

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        fieldnames = list(rows[0].keys()) if rows else []
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[WorkshopLights] Saved={saved}; report={output_path}")


if __name__ == "__main__":
    main()
