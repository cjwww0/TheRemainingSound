import unreal


BP_PATH = "/Game/NotKnowWhereToDeposit/BP_DiaryReadable_01"
BP_CLASS_PATH = "/Game/NotKnowWhereToDeposit/BP_DiaryReadable_01.BP_DiaryReadable_01_C"
BOOKS_TABLE_PATH = "/Game/HorrorMechanics/Database/Documents/Books_DataTable.Books_DataTable"


def set_first_existing(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            unreal.log(f"[ConfigureDiaryReadable] set {name} = {value}")
            return True
        except Exception:
            pass
    unreal.log_warning(f"[ConfigureDiaryReadable] none of these properties could be set: {names}")
    return False


bp_asset = unreal.EditorAssetLibrary.load_asset(BP_PATH)
bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_CLASS_PATH)
books_table = unreal.EditorAssetLibrary.load_asset(BOOKS_TABLE_PATH)

if not bp_asset:
    raise RuntimeError(f"Could not load blueprint asset: {BP_PATH}")
if not bp_class:
    raise RuntimeError(f"Could not load blueprint class: {BP_CLASS_PATH}")
if not books_table:
    raise RuntimeError(f"Could not load data table: {BOOKS_TABLE_PATH}")

cdo = unreal.get_default_object(bp_class)

set_first_existing(cdo, ["DocumentDataTable", "document_data_table"], books_table)
set_first_existing(cdo, ["DocumentRowName", "document_row_name"], unreal.Name("Dairy_01"))
set_first_existing(cdo, ["DocumentTypeName", "document_type_name"], unreal.Name("Book"))
set_first_existing(cdo, ["bSpawnTemplateDocumentProxy", "spawn_template_document_proxy"], True)
set_first_existing(cdo, ["bEnableDirectInteractInputFallback", "enable_direct_interact_input_fallback"], True)
set_first_existing(cdo, ["DirectInteractActionName", "direct_interact_action_name"], unreal.Name("Interact"))
set_first_existing(cdo, ["DirectInteractTraceDistance", "direct_interact_trace_distance"], 450.0)
set_first_existing(cdo, ["bDebugDocumentOpen", "debug_document_open"], True)

unreal.BlueprintEditorLibrary.compile_blueprint(bp_asset)
unreal.EditorAssetLibrary.save_loaded_asset(bp_asset)
unreal.log("[ConfigureDiaryReadable] BP_DiaryReadable_01 configured, compiled, and saved.")
