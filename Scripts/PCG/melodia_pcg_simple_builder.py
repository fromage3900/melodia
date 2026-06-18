"""
Simple Melodia PCG scatter graphs — volume → your mesh collections.

No Bezier paths, no hero setup, no portfolio layer.
Drop a PCG Volume in a level, assign a graph from /Game/_PROJECT/PCG/Graphs/Simple/, Generate.

Run in editor Output Log:
    py "G:/Melodia/Scripts/PCG/melodia_pcg_simple_builder.py"

Or console:
    Melodia.BuildSimplePCG
"""

from __future__ import annotations

import importlib.util
import os

try:
    import unreal
except ImportError as exc:
    raise RuntimeError("Run inside Unreal Editor") from exc

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
SIMPLE_PACKAGE = "/Game/_PROJECT/PCG/Graphs/Simple"


def _bez():
    spec = importlib.util.spec_from_file_location(
        "melodia_pcg_bezier_builder",
        os.path.join(SCRIPTS, "melodia_pcg_bezier_builder.py"),
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _ensure_simple_dir(bez) -> None:
    bez.ensure_directory(SIMPLE_PACKAGE)


def _create_graph(bez, asset_name: str):
    _ensure_simple_dir(bez)
    path = f"{SIMPLE_PACKAGE}/{asset_name}"
    existing = unreal.load_asset(path)
    if existing:
        existing.set_editor_property("is_standalone_graph", True)
        return existing, path
    factory = unreal.PCGGraphFactory()
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    graph = tools.create_asset(asset_name, SIMPLE_PACKAGE, unreal.PCGGraph, factory)
    graph.set_editor_property("is_standalone_graph", True)
    return graph, path


def build_volume_voxel_scatter(
    bez,
    asset_name: str,
    collection: str,
    *,
    voxel_cm: float = 400.0,
    tags: list[str] | None = None,
) -> str:
    """Input → volume voxels → static mesh spawner (weighted meshes from catalog)."""
    graph, asset_path = _create_graph(bez, asset_name)
    bez.clear_graph(graph)
    catalog = bez.load_catalog()
    out = graph.get_output_node()

    sampler, sampler_settings = bez.add_node(graph, "PCGVolumeSamplerSettings", -200, 0)
    sampler_settings.set_editor_property(
        "voxel_size", unreal.Vector(voxel_cm, voxel_cm, voxel_cm)
    )
    sampler_settings.set_editor_property("unbounded", False)

    spawner = bez.add_spawner(graph, catalog, 200, 0, collection, tags)

    graph.add_edge(sampler, "Out", spawner, "In")
    bez.wire_to_output(graph, spawner, "Out", out)
    bez.save_graph(graph, asset_path, bootstrap_volume=True)
    unreal.log(f"Simple PCG OK: {asset_path} ({collection}, voxel={voxel_cm}cm)")
    return asset_path


def build_landscape_scatter(
    bez,
    asset_name: str,
    collection: str,
    *,
    scatter_kind: str,
    target_count: int = 120,
    max_slope: float = 35.0,
    min_slope: float = 0.0,
) -> str:
    """Volume bounds → Melodia landscape scatter → spawner. Needs landscape in level."""
    graph, asset_path = _create_graph(bez, asset_name)
    bez.clear_graph(graph)
    catalog = bez.load_catalog()
    out = graph.get_output_node()
    kind_enum = getattr(unreal, "EMelodiaLandscapeScatterKind", None)

    sampler, sampler_settings = bez.add_node(graph, "PCGVolumeSamplerSettings", -400, 0)
    sampler_settings.set_editor_property("unbounded", False)
    sampler_settings.set_editor_property("voxel_size", unreal.Vector(800, 800, 800))

    scatter, scatter_settings = bez.add_node(
        graph, "PCGMelodiaLandscapeScatterSettings", 0, 0
    )
    scatter_settings.set_editor_property("target_count", target_count)
    scatter_settings.set_editor_property("bounds_margin", 800.0)
    scatter_settings.set_editor_property("max_slope_degrees", max_slope)
    scatter_settings.set_editor_property("min_slope_degrees", min_slope)
    if kind_enum is not None:
        scatter_settings.set_editor_property(
            "scatter_kind", getattr(kind_enum, scatter_kind)
        )

    spawner = bez.add_spawner(graph, catalog, 400, 0, collection)

    graph.add_edge(sampler, "Out", scatter, "In")
    graph.add_edge(scatter, "Out", spawner, "In")
    bez.wire_to_output(graph, spawner, "Out", out)
    bez.save_graph(graph, asset_path, bootstrap_volume=True)
    unreal.log(f"Simple PCG OK: {asset_path} (landscape {scatter_kind}, count={target_count})")
    return asset_path


def build_fixed_grid_scatter(
    bez,
    asset_name: str,
    collection: str,
    *,
    cell_cm: float = 400.0,
    extent_cm: float = 2000.0,
    tags: list[str] | None = None,
) -> str:
    """Regular grid → spawner. Place PCG Volume to clip/bounds; no volume sampler."""
    graph, asset_path = _create_graph(bez, asset_name)
    bez.clear_graph(graph)
    catalog = bez.load_catalog()
    out = graph.get_output_node()

    grid, grid_settings = bez.add_node(graph, "PCGCreatePointsGridSettings", -200, 0)
    grid_settings.set_editor_property("cell_size", unreal.Vector(cell_cm, cell_cm, 1.0))
    grid_settings.set_editor_property("grid_extents", unreal.Vector(extent_cm, extent_cm, 0.0))

    spawner = bez.add_spawner(graph, catalog, 200, 0, collection, tags)

    graph.add_edge(grid, "Out", spawner, "In")
    bez.wire_to_output(graph, spawner, "Out", out)
    bez.save_graph(graph, asset_path, bootstrap_volume=False)
    unreal.log(f"Simple PCG OK: {asset_path} (grid {cell_cm}cm, extent={extent_cm}cm)")
    return asset_path


SIMPLE_GRAPHS = [
    # Volume voxel scatter — drop PCG Volume anywhere, assign graph, Generate
    ("PCG_Simple_WallScatter", "volume_voxel", {"collection": "PCGCol_Baroque_Walls", "voxel_cm": 400}),
    ("PCG_Simple_ColumnScatter", "volume_voxel", {"collection": "PCGCol_Baroque_Columns", "voxel_cm": 400}),
    ("PCG_Simple_CorniceScatter", "volume_voxel", {"collection": "PCGCol_Baroque_Cornice", "voxel_cm": 300}),
    ("PCG_Simple_TowerScatter", "volume_voxel", {"collection": "PCGCol_Baroque_Towers", "voxel_cm": 800, "tags": ["tower"]}),
    ("PCG_Simple_BridgeScatter", "volume_voxel", {"collection": "PCGCol_Baroque_Bridges", "voxel_cm": 600}),
    # Fixed grids — good for blockout walls/columns at module spacing
    ("PCG_Simple_WallGrid", "grid", {"collection": "PCGCol_Baroque_Walls", "cell_cm": 400, "extent_cm": 2400}),
    ("PCG_Simple_ColumnGrid", "grid", {"collection": "PCGCol_Baroque_Columns", "cell_cm": 400, "extent_cm": 2000}),
    # Landscape — level must have landscape painted
    ("PCG_Simple_MeadowScatter", "landscape", {
        "collection": "PCGCol_Environment_GroundCover",
        "scatter_kind": "GROUND_COVER",
        "target_count": 160,
        "max_slope": 22.0,
    }),
    ("PCG_Simple_RockScatter", "landscape", {
        "collection": "PCGCol_Environment_Rocks",
        "scatter_kind": "ROCKS",
        "target_count": 80,
        "max_slope": 42.0,
    }),
]


def build_all() -> list[str]:
    bez = _bez()
    paths: list[str] = []
    for name, kind, kwargs in SIMPLE_GRAPHS:
        try:
            if kind == "volume_voxel":
                paths.append(build_volume_voxel_scatter(bez, name, **kwargs))
            elif kind == "grid":
                paths.append(build_fixed_grid_scatter(bez, name, **kwargs))
            elif kind == "landscape":
                paths.append(build_landscape_scatter(bez, name, **kwargs))
        except Exception as exc:
            unreal.log_error(f"Simple PCG failed {name}: {exc}")
    unreal.log(f"Melodia simple PCG: {len(paths)} graphs under {SIMPLE_PACKAGE}")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    return paths


if __name__ == "__main__":
    build_all()
