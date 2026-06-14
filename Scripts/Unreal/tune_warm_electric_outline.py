import unreal


TARGET_ASSET = "/Game/SoftOutline/Styles/MI_TRS_InteractOutline_WarmElectric"
HUE_SHIFT = 0.14
INTENSITY = 2.25


def main():
    asset_library = unreal.EditorAssetLibrary
    material_library = unreal.MaterialEditingLibrary

    target = asset_library.load_asset(TARGET_ASSET)
    if not target:
        raise RuntimeError(f"[TRS WarmElectric] Target asset not found: {TARGET_ASSET}")

    material_library.set_material_instance_scalar_parameter_value(target, "HueShift", HUE_SHIFT)
    material_library.set_material_instance_scalar_parameter_value(target, "Intensity", INTENSITY)

    asset_library.save_loaded_asset(target, only_if_is_dirty=False)
    unreal.log_warning(
        f"[TRS WarmElectric] Tuned: {TARGET_ASSET} | HueShift={HUE_SHIFT} | Intensity={INTENSITY}"
    )


main()
