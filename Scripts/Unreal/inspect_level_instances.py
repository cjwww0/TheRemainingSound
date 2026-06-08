import csv
import os

import unreal


TARGET_MAPS = [
    "/Game/HorrorMechanics/Demo/Maps/DemoScene_01",
    "/Game/VintageRoom/Maps/Showcase",
    "/Game/HorrorMechanics/Demo/Maps/Sub_ViolinWorkshop",
]
OUTPUT_NAME = "level_instances_report.csv"


KEYWORDS = (
    "level",
    "instance",
    "violin",
    "workshop",
    "workplace",
    "final",
    "house",
    "showcase",
    "vintage",
    "sub_",
)


def label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return actor.get_name()


def vec_text(vector):
    return f"{vector.x:.3f},{vector.y:.3f},{vector.z:.3f}" if vector else ""


def class_name(actor):
    try:
        return actor.get_class().get_name()
    except Exception:
        return ""


def level_name(actor):
    try:
        return actor.get_level().get_outer().get_path_name()
    except Exception:
        return ""


def get_if_exists(obj, names):
    for name in names:
        try:
            value = obj.get_editor_property(name)
            if value is not None:
                return value
        except Exception:
            pass
    return None


def call_if_exists(obj, names):
    for name in names:
        fn = getattr(obj, name, None)
        if not fn:
            continue
        try:
            return fn()
        except Exception:
            pass
    return ""


def value_path(value):
    if value is None:
        return ""
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def main():
    rows = []

    for map_path in TARGET_MAPS:
        unreal.log(f"[LevelInstances] Loading map: {map_path}")
        if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
            unreal.log_warning(f"[LevelInstances] Failed to load map: {map_path}")
            continue

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        for actor in actor_subsystem.get_all_level_actors():
            cls = class_name(actor)
            name = label(actor)
            actor_path = actor.get_path_name()
            combined = f"{cls} {name} {actor_path}".lower()
            asset = get_if_exists(actor, [
                "world_asset",
                "level_instance_asset",
                "level_asset",
                "template_world",
                "world",
            ])
            asset_path = value_path(asset)
            should_include = "levelinstance" in cls.lower() or any(keyword in combined for keyword in KEYWORDS) or any(keyword in asset_path.lower() for keyword in KEYWORDS)
            if not should_include:
                continue

            center, extent = actor.get_actor_bounds(False)
            rows.append({
                "map": map_path,
                "label": name,
                "class": cls,
                "level": level_name(actor),
                "actor_path": actor_path,
                "asset_path": asset_path,
                "location": vec_text(actor.get_actor_location()),
                "rotation": str(actor.get_actor_rotation()),
                "scale": vec_text(actor.get_actor_scale3d()),
                "bounds_center": vec_text(center),
                "bounds_extent": vec_text(extent),
                "hidden": call_if_exists(actor, ["is_hidden_ed"]),
                "hidden_game": call_if_exists(actor, ["is_hidden"]),
            })

    output_dir = os.path.join(unreal.Paths.project_saved_dir(), "Migration")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_NAME)
    fieldnames = list(rows[0].keys()) if rows else [
        "map",
        "label",
        "class",
        "level",
        "actor_path",
        "asset_path",
        "location",
        "rotation",
        "scale",
        "bounds_center",
        "bounds_extent",
        "hidden",
        "hidden_game",
    ]
    with open(output_path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    unreal.log(f"[LevelInstances] Wrote {len(rows)} rows to {output_path}")


if __name__ == "__main__":
    main()
