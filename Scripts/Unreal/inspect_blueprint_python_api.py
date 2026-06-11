import unreal


def main():
    for name in [
        "BlueprintEditorLibrary",
        "KismetEditorUtilities",
        "EditorAssetLibrary",
        "BlueprintEditorSubsystem",
    ]:
        obj = getattr(unreal, name, None)
        unreal.log(f"[BPApiInspect] {name}: {obj}")
        if obj:
            members = [member for member in dir(obj) if "interface" in member.lower() or "blueprint" in member.lower() or "graph" in member.lower()]
            unreal.log(f"[BPApiInspect] {name} members: {members}")


if __name__ == "__main__":
    main()
