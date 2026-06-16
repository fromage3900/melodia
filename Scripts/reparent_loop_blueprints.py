import unreal

def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Could not load asset: {path}")
    return asset

def reparent_blueprint(asset_path, parent_class_path):
    blueprint = load_asset(asset_path)
    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(f"Could not load parent class: {parent_class_path}")

    if hasattr(unreal, "BlueprintEditorLibrary") and hasattr(unreal.BlueprintEditorLibrary, "reparent_blueprint"):
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent_class)
    else:
        blueprint.set_editor_property("parent_class", parent_class)

    if hasattr(unreal, "BlueprintEditorLibrary") and hasattr(unreal.BlueprintEditorLibrary, "compile_blueprint"):
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    elif hasattr(unreal, "Kismet2BlueprintEditorUtils") and hasattr(unreal.Kismet2BlueprintEditorUtils, "compile_blueprint"):
        unreal.Kismet2BlueprintEditorUtils.compile_blueprint(blueprint)
    else:
        unreal.log_warning("No Blueprint compile API exposed; saving reparented Blueprint without explicit compile.")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
        raise RuntimeError(f"Could not save asset: {asset_path}")
    unreal.log(f"MELODIA_REPARENT_OK {asset_path} -> {parent_class_path}")

reparent_blueprint(
    "/Game/Melodia/Core/BP_QuestManager",
    "/Script/MelodiaMelusina_PROD.MelodiaQuestManagerBase",
)

reparent_blueprint(
    "/Game/Blueprints/WBP_RhythmHUD",
    "/Script/MelodiaMelusina_PROD.MelodiaRhythmHUDWidget",
)

unreal.log("MELODIA_REPARENT_DONE")
