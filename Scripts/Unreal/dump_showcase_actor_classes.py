import csv
import os

import unreal


TARGET_MAPS = [
    "/Game/VintageRoom/Maps/Showcase",
    "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop",
    "/Game/HorrorMechanics/Demo/Maps/DemoScene_01",
]


def safe_get(obj, name, default=""):
    try:
        value = obj.get_editor_property(name)
        return "" if value is None else str(value)
    except Exception:
        return default


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def dump_map(map_path):
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        unreal.log_warning(f"[ShowcaseDump] Failed to load map: {map_path}")
        return []

    rows = []
    actors = actor_subsystem().get_all_level_actors()
    unreal.log(f"[ShowcaseDump] {map_path}: {len(actors)} actor(s)")
    for actor in actors:
        component_classes = []
        light_intensities = []
        for component in actor.get_components_by_class(unreal.ActorComponent):
            component_class = component.get_class().get_name()
            component_classes.append(component_class)
            intensity = safe_get(component, "intensity")
            if intensity != "":
                light_intensities.append(f"{component.get_name()}={intensity}")

        rows.append({
            "map": map_path,
            "label": label(actor),
            "actor_name": actor.get_name(),
            "actor_class": actor.get_class().get_name(),
            "class_path": actor.get_class().get_path_name(),
            "location": vec(actor.get_actor_location()),
            "hidden_ed": str(actor.is_hidden_ed()),
            "hidden_in_game": safe_get(actor, "hidden_in_game"),
            "hidden": safe_get(actor, "hidden"),
            "component_classes": ";".join(sorted(set(component_classes))),
            "light_intensities": ";".join(light_intensities),
        })
    return rows


def main():
    rows = []
    for map_path in TARGET_MAPS:
        rows.extend(dump_map(map_path))

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "showcase_actor_class_dump.csv")
    fieldnames = [
        "map",
        "label",
        "actor_name",
        "actor_class",
        "class_path",
        "location",
        "hidden_ed",
        "hidden_in_game",
        "hidden",
        "component_classes",
        "light_intensities",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    unreal.log(f"[ShowcaseDump] Wrote {len(rows)} row(s): {output_path}")


if __name__ == "__main__":
    main()
