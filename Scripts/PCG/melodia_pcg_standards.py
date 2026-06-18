"""
Melodia PCG portfolio standards — single placeholder mesh per role, fixed module grid.

All portfolio graphs should use these until final art meshes are authored.
"""
from __future__ import annotations

try:
    import unreal
except ImportError:
    unreal = None  # type: ignore

# When True, builders use one canonical mesh per role (no weighted random variants).
# Off by default — your existing graph assets are authoritative; opt in only for greybox passes.
PORTFOLIO_PLACEHOLDER_MODE = False

# Canonical greybox modules (400 cm wall grid). Do not add random variants here.
PLACEHOLDERS = {
    "wall": "/Game/_PROJECT/MelusinasHouse/SM_wallhi",
    "column": "/Game/_PROJECT/houseassets/SM_Cube_001",
    "cornice": "/Game/_PROJECT/houseassets/SM_MarbleSlabOutline",
    "floor": "/Game/_PROJECT/houseassets/SM_ceilingsquare",
    "bridge": "/Game/_PROJECT/MelusinasHouse/SM_corridor",
    "tower": "/Game/_PROJECT/houseassets/SM_Cube_002",
    "scatter": "/Game/_PROJECT/houseassets/SM_Cube_001",
}

MODULE_CM = 400.0
MODULE_HEIGHT_CM = 600.0
DEFAULT_UNIFORM_SCALE = 1.0
SCATTER_UNIFORM_SCALE = 0.35

# DreamWalls v2 — predictable grid ISM count: layers * ((2*extent/cell)+1)^2
DREAMWALLS_EXTENT_CM = 2400.0
DREAMWALLS_LAYER_COUNT = 3
ALLOWED_YAW_DEG = (0.0, 90.0, 180.0)


def dreamwalls_expected_ism(
    *,
    extent_cm: float = DREAMWALLS_EXTENT_CM,
    cell_cm: float = MODULE_CM,
    layers: int = DREAMWALLS_LAYER_COUNT,
) -> int:
    pts_per_axis = int((2.0 * extent_cm / cell_cm)) + 1
    return layers * pts_per_axis * pts_per_axis


def placeholder_path(role: str) -> str:
    return PLACEHOLDERS.get(role, PLACEHOLDERS["wall"])


def _is_transform_points_settings(settings) -> bool:
    if unreal is None:
        return False
    try:
        return "TransformPoints" in settings.get_class().get_name()
    except Exception:
        return False


def _safe_set_vector(settings, prop: str, value) -> bool:
    if unreal is None:
        return False
    try:
        settings.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def apply_uniform_transform(settings, scale: float = DEFAULT_UNIFORM_SCALE, yaw_deg: float = 0.0) -> None:
    """Lock PCGTransformPointsSettings to uniform module scale (not mesh spawners)."""
    if unreal is None or not _is_transform_points_settings(settings):
        return
    s = float(scale)
    vec = unreal.Vector(s, s, s)
    rot = unreal.Rotator(0, yaw_deg, 0)
    _safe_set_vector(settings, "scale_min", vec)
    _safe_set_vector(settings, "scale_max", vec)
    for prop, val in (
        ("absolute_scale", False),
        ("uniform_scale", True),
        ("rotation_min", rot),
        ("rotation_max", rot),
        ("absolute_rotation", False),
    ):
        try:
            settings.set_editor_property(prop, val)
        except Exception:
            pass


def configure_placeholder_spawner(spawner_settings, role: str = "wall") -> None:
    if unreal is None:
        return
    mesh = unreal.load_asset(placeholder_path(role))
    if not mesh:
        unreal.log_warning(f"placeholder missing for role={role} path={placeholder_path(role)}")
        return
    entry = unreal.PCGMeshSelectorWeightedEntry()
    desc = entry.get_editor_property("descriptor")
    desc.set_editor_property("static_mesh", mesh)
    entry.set_editor_property("descriptor", desc)
    entry.set_editor_property("weight", 1)
    selector = spawner_settings.get_editor_property("mesh_selector_parameters")
    selector.set_editor_property("mesh_entries", [entry])


def configure_portfolio_ornament_scatter(scatter_settings) -> None:
    """Lock Nikki ornament scatter to module grid — no lateral/vertical jitter."""
    if unreal is None or not PORTFOLIO_PLACEHOLDER_MODE:
        return
    scatter_settings.set_editor_property("lateral_jitter", 0.0)
    scatter_settings.set_editor_property("vertical_jitter", 0.0)
