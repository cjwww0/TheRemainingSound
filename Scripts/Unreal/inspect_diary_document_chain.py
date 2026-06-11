import unreal


ASSETS = [
    "/Game/HorrorMechanics/Blueprint/Documents/Data/BP_NoteData",
    "/Game/HorrorMechanics/Blueprint/Documents/Data/BP_BookData",
    "/Game/HorrorMechanics/Blueprint/Documents/Data/BP_GenericDocumentData",
    "/Game/NotKnowWhereToDeposit/BP_DiaryReadable_01",
    "/Game/HorrorMechanics/Blueprint/Interfaces/BP_Interactive",
]


def _load(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        unreal.log_warning(f"[DiaryInspect] Failed to load {asset_path}")
        return None
    return asset


def _generated_class(asset):
    try:
        return asset.generated_class()
    except Exception:
        try:
            return asset.get_editor_property("generated_class")
        except Exception:
            return None


def main():
    for path in ASSETS:
        asset = _load(path)
        if not asset:
            continue

        cls = _generated_class(asset)
        parent = None
        if cls:
            try:
                parent = cls.get_super_class()
            except Exception:
                parent = None
        unreal.log(f"[DiaryInspect] {path} | asset={asset.get_class().get_name()} | class={cls.get_name() if cls else '<none>'} | parent={parent.get_name() if parent else '<none>'}")

    book_asset = _load("/Game/HorrorMechanics/Blueprint/Documents/Data/BP_BookData")
    note_asset = _load("/Game/HorrorMechanics/Blueprint/Documents/Data/BP_NoteData")
    book_cls = _generated_class(book_asset) if book_asset else None
    note_cls = _generated_class(note_asset) if note_asset else None
    if book_cls and note_cls:
        try:
            result = book_cls.is_child_of(note_cls)
        except Exception as exc:
            result = f"<unavailable: {exc}>"
        unreal.log(f"[DiaryInspect] BP_BookData is child of BP_NoteData: {result}")


if __name__ == "__main__":
    main()
