"""Build UE 5.8 Substrate Toon look-dev materials for Melodia SDF eval.

Run inside the UE 5.8 editor (Melodia on ue5.8-eval):
  Tools -> Execute Python Script -> this file
  or Output Log console:  py "G:/Melodia/Scripts/Materials/melodia_toon58_lookdev_builder.py"

Requires:
  - Stock UE 5.8 editor (not MooaToon 5.7)
  - Config/DefaultEngine.ini: r.Substrate=True (Blendable GBuffer)
  - Editor restart after first Substrate enable
"""
from __future__ import annotations

import unreal

ROOT = "/Game/Melodia/Materials/SDF/Toon58"
PROFILE_DIR = f"{ROOT}/Profiles"
MASTER = f"{ROOT}/M_Toon58_SDFMaster"

# Melusina baroque palette (lavender stucco + gold accent)
LOOKDEV_INSTANCES = [
    {
        "name": "MI_Toon58_Wall",
        "profile": "TP_Toon58_Stucco",
        "tint": (0.72, 0.55, 0.85, 1.0),
        "accent": (0.58, 0.42, 0.68, 1.0),
        "band_scale": 0.035,
        "band_strength": 0.22,
    },
    {
        "name": "MI_Toon58_Floor",
        "profile": "TP_Toon58_Default",
        "tint": (0.45, 0.38, 0.42, 1.0),
        "accent": (0.32, 0.28, 0.30, 1.0),
        "band_scale": 0.05,
        "band_strength": 0.12,
    },
    {
        "name": "MI_Toon58_Accent",
        "profile": "TP_Toon58_Gold",
        "tint": (0.85, 0.65, 0.25, 1.0),
        "accent": (0.95, 0.78, 0.35, 1.0),
        "band_scale": 0.08,
        "band_strength": 0.35,
    },
    {
        "name": "MI_Toon58_Rim",
        "profile": "TP_Toon58_Stucco",
        "tint": (0.25, 0.18, 0.35, 1.0),
        "accent": (0.12, 0.08, 0.20, 1.0),
        "band_scale": 0.12,
        "band_strength": 0.45,
    },
]

TOON_PROFILES = [
    "TP_Toon58_Default",
    "TP_Toon58_Stucco",
    "TP_Toon58_Gold",
]


def _ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def _asset_path(folder: str, name: str) -> str:
    return f"{folder}/{name}.{name}"


def _save_package(asset) -> None:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def create_toon_profiles() -> dict[str, unreal.ToonProfile]:
    _ensure_directory(PROFILE_DIR)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.ToonProfileFactory()
    profiles: dict[str, unreal.ToonProfile] = {}

    for profile_name in TOON_PROFILES:
        path = _asset_path(PROFILE_DIR, profile_name)
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            profiles[profile_name] = unreal.load_asset(path)
            unreal.log(f"Toon58: reusing profile {path}")
            continue

        profile = asset_tools.create_asset(
            profile_name, PROFILE_DIR, unreal.ToonProfile, factory
        )
        if not profile:
            raise RuntimeError(f"Failed to create ToonProfile {profile_name}")
        profiles[profile_name] = profile
        _save_package(profile)
        unreal.log(f"Toon58: created profile {path}")

    return profiles


def _create_expression(material, expression_class, x: int, y: int):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def _connect(from_expr, from_output: str, to_expr, to_input: str) -> None:
    unreal.MaterialEditingLibrary.connect_material_expressions(
        from_expr, from_output, to_expr, to_input
    )


def _connect_front_material(material, from_expr, from_output: str = "") -> None:
    unreal.MaterialEditingLibrary.connect_material_property(
        from_expr,
        from_output,
        unreal.MaterialProperty.MP_FRONT_MATERIAL,
    )


def build_master_material(profiles: dict[str, unreal.ToonProfile]) -> str:
    _ensure_directory(ROOT)

    if unreal.EditorAssetLibrary.does_asset_exist(MASTER):
        unreal.log_warning(f"Toon58: rebuilding existing master {MASTER}")
        unreal.EditorAssetLibrary.delete_asset(MASTER)

    material_factory = unreal.MaterialFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(
        "M_Toon58_SDFMaster", ROOT, unreal.Material, material_factory
    )
    if not material:
        raise RuntimeError("Failed to create M_Toon58_SDFMaster")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    if material.has_editor_property("bUsesSubstrate"):
        material.set_editor_property("bUsesSubstrate", True)

    # --- Parameters (exposed on instances) ---
    tint_param = _create_expression(
        material, unreal.MaterialExpressionVectorParameter, -1400, -120
    )
    tint_param.set_editor_property("parameter_name", "BaseTint")
    tint_param.set_editor_property("group", "Toon58")
    tint_param.set_editor_property(
        "default_value", unreal.LinearColor(0.72, 0.55, 0.85, 1.0)
    )

    accent_param = _create_expression(
        material, unreal.MaterialExpressionVectorParameter, -1400, 40
    )
    accent_param.set_editor_property("parameter_name", "AccentTint")
    accent_param.set_editor_property("group", "Toon58")
    accent_param.set_editor_property(
        "default_value", unreal.LinearColor(0.58, 0.42, 0.68, 1.0)
    )

    band_scale = _create_expression(
        material, unreal.MaterialExpressionScalarParameter, -1400, 200
    )
    band_scale.set_editor_property("parameter_name", "SDF_BandScale")
    band_scale.set_editor_property("group", "SDF")
    band_scale.set_editor_property("default_value", 0.035)

    band_strength = _create_expression(
        material, unreal.MaterialExpressionScalarParameter, -1400, 320
    )
    band_strength.set_editor_property("parameter_name", "SDF_BandStrength")
    band_strength.set_editor_property("group", "SDF")
    band_strength.set_editor_property("default_value", 0.22)

    # --- Faux SDF panel bands (world-aligned sin) ---
    world_pos = _create_expression(
        material, unreal.MaterialExpressionWorldPosition, -1200, 260
    )
    mask_xy = _create_expression(
        material, unreal.MaterialExpressionComponentMask, -1000, 260
    )
    mask_xy.set_editor_property("r", True)
    mask_xy.set_editor_property("g", True)
    mask_xy.set_editor_property("b", False)
    mask_xy.set_editor_property("a", False)
    _connect(world_pos, "", mask_xy, "")

    scale_mul = _create_expression(material, unreal.MaterialExpressionMultiply, -820, 260)
    _connect(mask_xy, "", scale_mul, "A")
    _connect(band_scale, "", scale_mul, "B")

    sin_node = _create_expression(material, unreal.MaterialExpressionSine, -640, 260)
    sin_node.set_editor_property("period", 1.0)
    _connect(scale_mul, "", sin_node, "Input")

    abs_node = _create_expression(material, unreal.MaterialExpressionAbs, -460, 260)
    _connect(sin_node, "", abs_node, "Input")

    band_lerp = _create_expression(material, unreal.MaterialExpressionLinearInterpolate, -240, 40)
    _connect(tint_param, "", band_lerp, "A")
    _connect(accent_param, "", band_lerp, "B")
    _connect(abs_node, "", band_lerp, "Alpha")

    strength_mul = _create_expression(material, unreal.MaterialExpressionMultiply, -40, 40)
    _connect(band_lerp, "", strength_mul, "A")
    _connect(band_strength, "", strength_mul, "B")

    final_tint = _create_expression(material, unreal.MaterialExpressionAdd, 160, 40)
    _connect(tint_param, "", final_tint, "A")
    _connect(strength_mul, "", final_tint, "B")

    # --- Substrate Toon BSDF ---
    toon_bsdf = _create_expression(
        material, unreal.MaterialExpressionSubstrateToonBSDF, 420, 0
    )
    toon_bsdf.set_editor_property("toon_profile", profiles["TP_Toon58_Default"])
    _connect(final_tint, "", toon_bsdf, "DiffuseColor")

    _connect_front_material(material, toon_bsdf)

    unreal.MaterialEditingLibrary.recompile_material(material)
    _save_package(material)
    unreal.log(f"Toon58: built master {MASTER}")
    return MASTER


def _set_instance_vector(instance, name: str, rgba: tuple[float, float, float, float]) -> None:
    color = unreal.LinearColor(*rgba)
    if hasattr(unreal.MaterialEditingLibrary, "set_material_instance_vector_parameter_value"):
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, name, color
        )
    else:
        instance.set_vector_parameter_value_editor_only(name, color)


def _set_instance_scalar(instance, name: str, value: float) -> None:
    if hasattr(unreal.MaterialEditingLibrary, "set_material_instance_scalar_parameter_value"):
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value
        )
    else:
        instance.set_scalar_parameter_value_editor_only(name, value)


def _set_instance_toon_profile(instance, profile: unreal.ToonProfile) -> None:
    if instance.has_editor_property("toon_profile"):
        instance.set_editor_property("toon_profile", profile)
    if instance.has_editor_property("override_toon_profile"):
        instance.set_editor_property("override_toon_profile", True)


def build_instances(master_path: str, profiles: dict[str, unreal.ToonProfile]) -> list[str]:
    created: list[str] = []
    for spec in LOOKDEV_INSTANCES:
        inst_path = _asset_path(ROOT, spec["name"])
        if unreal.EditorAssetLibrary.does_asset_exist(inst_path):
            unreal.EditorAssetLibrary.delete_asset(inst_path)

        factory = unreal.MaterialInstanceConstantFactoryNew()
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        parent = unreal.load_asset(master_path)
        instance = asset_tools.create_asset(
            spec["name"], ROOT, unreal.MaterialInstanceConstant, factory
        )
        if not instance:
            raise RuntimeError(f"Failed to create instance {spec['name']}")

        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
        _set_instance_vector(instance, "BaseTint", spec["tint"])
        _set_instance_vector(instance, "AccentTint", spec["accent"])
        _set_instance_scalar(instance, "SDF_BandScale", spec["band_scale"])
        _set_instance_scalar(instance, "SDF_BandStrength", spec["band_strength"])
        _set_instance_toon_profile(instance, profiles[spec["profile"]])

        _save_package(instance)
        created.append(inst_path)
        unreal.log(f"Toon58: created instance {inst_path}")

    return created


def build_all() -> None:
    unreal.log("=== Melodia Toon58 look-dev build start ===")
    profiles = create_toon_profiles()
    master = build_master_material(profiles)
    instances = build_instances(master, profiles)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("=== Melodia Toon58 look-dev build complete ===")
    unreal.log(f"Master: {master}")
    for path in instances:
        unreal.log(f"  Instance: {path}")


if __name__ == "__main__":
    build_all()
