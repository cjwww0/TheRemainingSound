import unreal


def main():
    bp_class = unreal.EditorAssetLibrary.load_blueprint_class(
        "/Game/HorrorMechanics/Blueprint/Inventory/BP_Inventory.BP_Inventory_C"
    )
    if not bp_class:
        unreal.log_error("[InventorySignature] Failed to load BP_Inventory_C")
        return

    cdo = unreal.get_default_object(bp_class)
    function = cdo.find_function("AddDocumentAsDTRef")
    if not function:
        unreal.log_error("[InventorySignature] AddDocumentAsDTRef not found")
        return

    unreal.log(f"[InventorySignature] Function={function.get_name()} ParmsSize={function.get_editor_property('parms_size')}")
    for prop in unreal.TFieldIterator(unreal.Property, function):
        flags = []
        for name in [
            "PARM",
            "OUT_PARM",
            "RETURN_PARM",
            "REFERENCE_PARM",
            "CONST_PARM",
        ]:
            try:
                if prop.has_any_property_flags(getattr(unreal.PropertyFlags, name)):
                    flags.append(name)
            except Exception:
                pass
        unreal.log(f"[InventorySignature] Prop={prop.get_name()} Class={prop.get_class().get_name()} Flags={','.join(flags)}")


if __name__ == "__main__":
    main()
