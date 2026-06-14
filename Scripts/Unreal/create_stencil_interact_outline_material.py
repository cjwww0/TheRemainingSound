import unreal


ASSET_PATH = "/Game/HorrorMechanics/Materials/M_PP_InteractOutline_StencilOnly"
PACKAGE_PATH = "/Game/HorrorMechanics/Materials"
ASSET_NAME = "M_PP_InteractOutline_StencilOnly"


def set_prop_if_available(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def main():
    editor_assets = unreal.EditorAssetLibrary
    if editor_assets.does_asset_exist(ASSET_PATH):
        unreal.log(f"{ASSET_PATH} already exists; leaving it unchanged")
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(
        ASSET_NAME,
        PACKAGE_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )

    if material is None:
        raise RuntimeError(f"Failed to create material {ASSET_PATH}")

    set_prop_if_available(material, "material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    set_prop_if_available(material, "blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

    # This script intentionally does not delete old expressions. It is normally run once
    # for a fresh asset; if rerun, the last connected custom node wins.
    custom = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionCustom,
        -360,
        0,
    )
    custom.set_editor_property("description", "StencilOnlyInteractionOutline")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property(
        "code",
        """
float2 UV = GetDefaultSceneTextureUV(Parameters, PPI_PostProcessInput0);
float2 Texel = View.ViewSizeAndInvSize.zw;
float3 SceneColor = SceneTextureLookup(UV, PPI_PostProcessInput0, false).rgb;

float CenterStencil = SceneTextureLookup(UV, PPI_CustomStencil, false).r;
float CenterMask = (CenterStencil > 251.5 && CenterStencil < 252.5) ? 1.0 : 0.0;

float NeighborMask = 0.0;
float2 Offsets[8] = {
    float2(1.0, 0.0),
    float2(-1.0, 0.0),
    float2(0.0, 1.0),
    float2(0.0, -1.0),
    float2(1.0, 1.0),
    float2(1.0, -1.0),
    float2(-1.0, 1.0),
    float2(-1.0, -1.0)
};

for (int i = 0; i < 8; ++i)
{
    float NeighborStencil = SceneTextureLookup(UV + Offsets[i] * Texel * 1.35, PPI_CustomStencil, false).r;
    NeighborMask = max(NeighborMask, (NeighborStencil > 251.5 && NeighborStencil < 252.5) ? 1.0 : 0.0);
}

float Outline = saturate(NeighborMask - CenterMask);
float3 OutlineColor = float3(0.76, 0.65, 0.46);
return lerp(SceneColor, OutlineColor, Outline * 0.24);
""",
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        custom,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    editor_assets.save_asset(ASSET_PATH, only_if_is_dirty=False)
    unreal.log(f"Created/updated {ASSET_PATH}")


if __name__ == "__main__":
    main()
