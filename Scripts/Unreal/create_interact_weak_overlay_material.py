import unreal


ASSET_DIR = "/Game/HorrorMechanics/Materials"
ASSET_NAME = "M_InteractWarmOverlay"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log(f"{ASSET_PATH} already exists; skipping material rebuild")
        return
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            ASSET_NAME,
            ASSET_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    if not material:
        raise RuntimeError(f"Failed to create or load {ASSET_PATH}")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)

    # Keep this readable as focus feedback, but below "gamey outline/glow" intensity.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant3Vector,
        -400,
        -120,
    )
    color.set_editor_property("constant", unreal.LinearColor(0.95, 0.76, 0.34, 1.0))

    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -400,
        80,
    )
    opacity.set_editor_property("r", 0.32)

    unreal.MaterialEditingLibrary.connect_material_property(
        color,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity,
        "",
        unreal.MaterialProperty.MP_OPACITY,
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log(f"Created/updated {ASSET_PATH}")


if __name__ == "__main__":
    main()
