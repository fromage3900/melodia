"""
Melodia Art Nouveau Greybox — Blender Python Addon
====================================================
Advanced greybox tooling for organic Art Nouveau architecture inspired by
Horta, Gaudi, and Mucha. Generates parametric, flowing architectural geometry
and exports clean FBX for UE5 import.

Two geometry modes:
  VALID      — watertight, exportable, manifold mesh
  IMPOSSIBLE — non-manifold, overlapping geometry allowed
"""

bl_info = {
    "name": "Melodia Art Nouveau Greybox",
    "author": "Melodia Studio",
    "version": (1, 0, 0),
    "blender": (4, 2, 0),
    "location": "View3D > Sidebar > Melodia Art Nouveau",
    "description": "Parametric Art Nouveau architecture greybox generator",
    "category": "Mesh",
}

import importlib
import sys

# ---------------------------------------------------------------------------
# Sub-module list (order matters for dependencies)
# ---------------------------------------------------------------------------
_SUBMODULES = [
    # Core
    "core.constants",
    "core.seed_manager",
    "core.math_utils",
    "core.bezier",
    "core.profile_curves",
    "core.golden",
    "core.mesh_builder",
    "core.mesh_cleanup",
    "core.uv_utils",
    "core.fbx_export",
    "core.naming",
    "core.geometry_modes",
    # Generators
    "generators.base_generator",
    "generators.columns.column_base",
    "generators.columns.stem_column",
    "generators.columns.tree_column",
    "generators.walls.wall_base",
    "generators.walls.whiplash_wall",
    "generators.walls.curved_facade",
    "generators.arches.organic_arch",
    "generators.arches.doorway",
    "generators.ornaments.ornament_base",
    "generators.ornaments.floral_capital",
    "generators.ornaments.vine_tendril",
    "generators.ornaments.peacock_fan",
    "generators.ornaments.lily_bracket",
    "generators.ornaments.butterfly_panel",
    "generators.surfaces.surface_base",
    "generators.surfaces.stained_glass",
    "generators.surfaces.tessellation",
    "generators.surfaces.iron_railing",
    "generators.surfaces.mosaic_floor",
    "generators.vaults.gaudi_vault",
    # UI
    "ui.ui_utils",
    "ui.main_panel",
    "ui.panel_columns",
    "ui.panel_walls",
    "ui.panel_arches",
    "ui.panel_ornaments",
    "ui.panel_surfaces",
    "ui.panel_vaults",
    "ui.panel_presets",
    "ui.panel_batch",
    # Presets + Batch
    "presets.preset_manager",
    "batch.batch_generator",
    "batch.layout_parser",
]


def register():
    """Register all addon classes with Blender."""
    import bpy
    from . import addon_prefs
    addon_prefs.register()

    # Import and register all submodules
    package = __package__
    for name in _SUBMODULES:
        full = f"{package}.{name}"
        if full in sys.modules:
            mod = importlib.import_module(full)
            importlib.reload(mod)
        else:
            mod = importlib.import_module(full)

        if hasattr(mod, "register"):
            mod.register()


def unregister():
    """Unregister all addon classes from Blender."""
    from . import addon_prefs
    addon_prefs.unregister()

    package = __package__
    for name in reversed(_SUBMODULES):
        full = f"{package}.{name}"
        if full in sys.modules:
            mod = sys.modules[full]
            if hasattr(mod, "unregister"):
                mod.unregister()
