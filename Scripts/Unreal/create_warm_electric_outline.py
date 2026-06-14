import unreal


SOURCE_ASSET = "/Game/SoftOutline/Styles/SoftOutline_Electric"
TARGET_DIR = "/Game/SoftOutline/Styles"
TARGET_NAME = "MI_TRS_InteractOutline_WarmElectric"
TARGET_ASSET = f"{TARGET_DIR}/{TARGET_NAME}"


def _get_scalar_parameter(material_instance, parameter_name, fallback):
    try:
        value = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
            material_instance,
            parameter_name,
        )
        if isinstance(value, tuple):
            value = value[0]
        return float(value)
    except Exception as exc:
        unreal.log_warning(
            f"[TRS WarmElectric] Could not read scalar parameter {parameter_name}: {exc}. "
            f"Using fallback {fallback}."
        )
        return fallback


def main():
    asset_library = unreal.EditorAssetLibrary
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_library = unreal.MaterialEditingLibrary

    source = asset_library.load_asset(SOURCE_ASSET)
    if not source:
        raise RuntimeError(f"[TRS WarmElectric] Source asset not found: {SOURCE_ASSET}")

    if asset_library.does_asset_exist(TARGET_ASSET):
        target = asset_library.load_asset(TARGET_ASSET)
        unreal.log_warning(f"[TRS WarmElectric] Updating existing asset: {TARGET_ASSET}")
    else:
        target = asset_tools.duplicate_asset(TARGET_NAME, TARGET_DIR, source)
        if not target:
            raise RuntimeError(f"[TRS WarmElectric] Failed to duplicate {SOURCE_ASSET} to {TARGET_ASSET}")
        unreal.log_warning(f"[TRS WarmElectric] Created asset: {TARGET_ASSET}")

    electric_intensity = _get_scalar_parameter(source, "Intensity", 1.0)
    target_intensity = max(electric_intensity * 1.5, 1.5)

    material_library.set_material_instance_scalar_parameter_value(target, "HueShift", 0.0)
    material_library.set_material_instance_scalar_parameter_value(target, "Intensity", target_intensity)

    asset_library.save_loaded_asset(target, only_if_is_dirty=False)
    unreal.log_warning(
        f"[TRS WarmElectric] Ready: {TARGET_ASSET} | HueShift=0.0 | "
        f"ElectricIntensity={electric_intensity} | TargetIntensity={target_intensity}"
    )


main()
