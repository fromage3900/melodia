"""
Melodia Baroque Greybox — Blender Python Addon
===============================================
Advanced greybox tooling for surreal baroque architecture inspired by
Infinity Nikki and MC Escher. Generates parametric, fully-ornamented
architectural geometry and exports clean FBX for UE5 import.

Two geometry modes:
  VALID      — watertight, exportable, manifold mesh
  IMPOSSIBLE — non-manifold, Escher-style overlapping geometry
"""

bl_info = {
    "name": "Melodia Baroque Greybox",
    "author": "Melodia Studio",
    "version": (1, 0, 0),
    "blender": (4, 2, 0),  # Compatible with 4.2+ through 5.x
    "location": "View3D > Sidebar > Melodia Baroque",
    "description": "Parametric surreal baroque architecture greybox generator",
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
    "generators.columns.doric",
    "generators.columns.ionic",
    "generators.columns.corinthian",
    "generators.columns.composite",
    "generators.columns.solomonic",
    "generators.moldings.architrave",
    "generators.moldings.frieze",
    "generators.moldings.cornice",
    "generators.moldings.base_molding",
    "generators.ornaments.volute",
    "generators.ornaments.acanthus",
    "generators.ornaments.rose_window",
    "generators.ornaments.cartouche",
    "generators.ornaments.finial",
    "generators.ornaments.shell",
    "generators.balustrade",
    "generators.vaults.barrel_vault",
    "generators.vaults.groin_vault",
    "generators.vaults.ribbed_vault",
    "generators.vaults.dome",
    "generators.vaults.coffered",
    "generators.cathedral",
    "generators.curve_architecture",
    "generators.facade",
    "generators.escher.penrose_stairs",
    "generators.escher.mobius_walkway",
    "generators.escher.impossible_bridge",
    "generators.escher.gravity_platform",
    "generators.escher.recursive_arches",
    "generators.escher.klein_volume",
    # UI
    "ui.ui_utils",
    "ui.main_panel",
    "ui.panel_balustrade",
    "ui.panel_cathedral",
    "ui.panel_curves",
    "ui.panel_columns",
    "ui.panel_moldings",
    "ui.panel_ornaments",
    "ui.panel_escher",
    "ui.panel_facade",
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
            # Module already loaded (e.g., during reload)
            mod = importlib.import_module(full)
            importlib.reload(mod)
        else:
            # First-time import
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
