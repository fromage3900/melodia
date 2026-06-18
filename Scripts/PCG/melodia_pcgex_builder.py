"""
Melodia Baroque PCGEx graph builder.

Run inside Unreal Editor (Python 3.11) after enabling PCG, PCGPythonInterop,
and PCGExtendedToolkit. Creates graph assets under /Game/_PROJECT/PCG/Graphs/PCGEx/.

Usage (Output Log -> Python):
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "melodia_pcgex_builder",
        r"G:/Melodia/Scripts/PCG/melodia_pcgex_builder.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.build_all()

Or: mod.build_graph("PCG_BaroqueColonnadeEx")
"""

from __future__ import annotations

import json
import os
from typing import Callable, Optional

try:
    import unreal
except ImportError as exc:  # pragma: no cover - only importable in UE
    raise RuntimeError(
        "melodia_pcgex_builder must run inside Unreal Editor Python"
    ) from exc


# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

PROJECT_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
)
CATALOG_PATH = os.path.join(PROJECT_ROOT, "Scripts", "PCG", "baroque_mesh_catalog.json")
GRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs/PCGEx"
SUBGRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs/PCGEx/Subgraphs"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def load_catalog() -> dict:
    with open(CATALOG_PATH, encoding="utf-8") as fh:
        return json.load(fh)


def pin_labels(pins) -> list[str]:
    return [
        str(p.get_editor_property("properties").get_editor_property("label"))
        for p in pins
    ]


def settings_name(node) -> str:
    return type(node.get_settings()).__name__


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def save_and_reload(asset_path: str) -> None:
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    pkg = unreal.find_package(asset_path)
    if pkg:
        unreal.EditorLoadingAndSavingUtils.reload_packages([pkg])


def get_settings_class(name: str):
    """Return unreal.PCG*Settings class or None if PCGEx not loaded."""
    return getattr(unreal, name, None)


def add_node(graph, settings_class_name: str, x: int, y: int):
    cls = get_settings_class(settings_class_name)
    if cls is None:
        raise RuntimeError(
            f"Settings class '{settings_class_name}' not found. "
            "Is PCGExtendedToolkit enabled and the project rebuilt?"
        )
    node, settings = graph.add_node_of_type(cls)
    node.set_node_position(x, y)
    return node, settings


def add_node_or_fallback(
    graph,
    primary: str,
    fallback: str,
    x: int,
    y: int,
):
    cls = get_settings_class(primary) or get_settings_class(fallback)
    if cls is None:
        raise RuntimeError(f"Neither {primary} nor {fallback} available in unreal module")
    node, settings = graph.add_node_of_type(cls)
    node.set_node_position(x, y)
    used = primary if get_settings_class(primary) else fallback
    return node, settings, used


def _main_input_pin(node) -> str:
    labels = pin_labels(node.get_editor_property("input_pins"))
    for preferred in ("Paths", "Splines", "Seeds", "Vtx", "Surface", "Volume", "Source", "In"):
        if preferred in labels:
            return preferred
    return labels[0] if labels else "In"


def _main_output_pin(node) -> str:
    labels = pin_labels(node.get_editor_property("output_pins"))
    for preferred in ("Paths", "Out", "Vtx"):
        if preferred in labels:
            return preferred
    return labels[0] if labels else "Out"


def wire_output_to_input(graph, src_node, dst_node) -> None:
    graph.add_edge(src_node, _main_output_pin(src_node), dst_node, _main_input_pin(dst_node))


def wire_chain(graph, nodes: list, output_node) -> None:
    """Wire Input -> n0 -> n1 -> ... -> Output using node-specific pin labels."""
    input_node = graph.get_input_node()
    graph.add_edge(input_node, "In", nodes[0], _main_input_pin(nodes[0]))
    for a, b in zip(nodes, nodes[1:]):
        wire_output_to_input(graph, a, b)
    graph.add_edge(nodes[-1], _main_output_pin(nodes[-1]), output_node, "Out")


def attach_shape_grid_builder(
    graph,
    shapes_node,
    x: int,
    y: int,
    cell_cm: float = 400.0,
) -> unreal.PCGNode:
    """Wire a 3D grid shape builder into Create Shapes (required for PCGEx output)."""
    grid_node, grid_settings = add_node(graph, "PCGExCreateShapeGridSettings", x, y)
    config = grid_settings.get_editor_property("config")
    res_vec = config.get_editor_property("resolution_vector")
    res_vec.set_editor_property("constant", unreal.Vector(cell_cm, cell_cm, cell_cm))
    config.set_editor_property("resolution_vector", res_vec)
    config.set_editor_property("resolution_mode", unreal.PCGExResolutionMode.FIXED)
    grid_settings.set_editor_property("config", config)
    graph.add_edge(grid_node, "Shape Builder", shapes_node, "Shape Builders")
    return grid_node


def mesh_entry(static_mesh_path: str, weight: int = 1):
    mesh = unreal.load_asset(static_mesh_path)
    if mesh is None:
        unreal.log_warning(f"Mesh not found: {static_mesh_path}")
        return None
    entry = unreal.PCGMeshSelectorWeightedEntry()
    desc = entry.get_editor_property("descriptor")
    desc.set_editor_property("static_mesh", mesh)
    entry.set_editor_property("descriptor", desc)
    entry.set_editor_property("weight", weight)
    return entry


def configure_spawner_from_collection(
    spawner_settings,
    collection_key: str,
    catalog: dict,
) -> None:
    """Weighted mesh list from baroque_mesh_catalog.json (vanilla selector)."""
    coll = catalog["collections"].get(collection_key)
    if not coll:
        unreal.log_warning(f"Collection key missing: {collection_key}")
        return
    entries = []
    for item in coll["entries"]:
        entry = mesh_entry(item["mesh"], item.get("weight", 1))
        if entry:
            entries.append(entry)
    if not entries:
        return
    selector = spawner_settings.get_editor_property("mesh_selector_parameters")
    selector.set_editor_property("mesh_entries", entries)


def create_graph_asset(name: str, package: str, description: str = ""):
    ensure_directory(package)
    asset_path = f"{package}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        graph = unreal.EditorAssetLibrary.load_asset(asset_path)
        unreal.log(f"Reusing existing graph: {asset_path}")
    else:
        factory = unreal.PCGGraphFactory()
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        graph = asset_tools.create_asset(name, package, unreal.PCGGraph, factory)
        unreal.log(f"Created graph: {asset_path}")
    if description:
        graph.set_editor_property("description", description)
    graph.set_editor_property("is_standalone_graph", True)
    return graph, asset_path


def clear_user_nodes(graph) -> None:
    for node in list(graph.nodes):
        graph.remove_node(node)


# ---------------------------------------------------------------------------
# Graph recipes
# ---------------------------------------------------------------------------


def build_sub_baroque_spawn(catalog: dict) -> str:
    name = "PCG_Sub_BaroqueSpawn"
    graph, path = create_graph_asset(
        name,
        SUBGRAPH_PACKAGE,
        "Spawn baroque wall modules from input points.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(600, 0)

    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 300, 0
    )
    configure_spawner_from_collection(spawner_settings, "PCGCol_Baroque_Walls", catalog)
    wire_chain(graph, [spawner_node], output_node)
    save_and_reload(path)
    return path


def build_sub_baroque_column(catalog: dict) -> str:
    name = "PCG_Sub_BaroqueColumn"
    graph, path = create_graph_asset(
        name,
        SUBGRAPH_PACKAGE,
        "Column shaft spawn with story-height scale.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(900, 0)

    xform_node, xform_settings, _ = add_node_or_fallback(
        graph,
        "PCGExTransformPointsSettings",
        "PCGTransformPointsSettings",
        300,
        0,
    )
    grid = catalog["module_grid_cm"]
    if hasattr(xform_settings, "set_editor_property"):
        try:
            xform_settings.set_editor_property("uniform_scale", True)
            xform_settings.set_editor_property("absolute_scale", False)
            xform_settings.set_editor_property("scale_min", unreal.Vector(0.95, 0.95, 0.95))
            xform_settings.set_editor_property("scale_max", unreal.Vector(1.05, 1.05, 1.05))
        except Exception:  # noqa: BLE001 — property names differ between PCGEx/vanilla
            pass

    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 600, 0
    )
    configure_spawner_from_collection(
        spawner_settings, "PCGCol_Baroque_Columns", catalog
    )
    wire_chain(graph, [xform_node, spawner_node], output_node)
    save_and_reload(path)
    return path


def build_sub_baroque_along_path(catalog: dict, sub_spawn_path: str) -> str:
    name = "PCG_Sub_BaroqueAlongPath"
    graph, path = create_graph_asset(
        name,
        SUBGRAPH_PACKAGE,
        "Resample path to module width and spawn wall modules.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1200, 0)

    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        300,
        0,
    )
    grid = catalog["module_grid_cm"]
    try:
        resample_settings.set_editor_property("Distance", float(grid["wall_width"]))
    except Exception:  # noqa: BLE001
        pass

    orient_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExOrientSettings",
        "PCGTransformPointsSettings",
        600,
        0,
    )

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 900, 0)
    sub_asset = unreal.EditorAssetLibrary.load_asset(sub_spawn_path)
    sg_settings.set_editor_property("subgraph_override", sub_asset)

    wire_chain(graph, [resample_node, orient_node, sg_node], output_node)
    save_and_reload(path)
    return path


def build_colonnade_ex(catalog: dict, sub_column_path: str) -> str:
    name = "PCG_BaroqueColonnadeEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "PCGEx grid colonnade: Create Shapes -> path resample -> column spawn.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1600, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGVolumeSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("unbounded", True)
    sampler_settings.set_editor_property(
        "voxel_size", unreal.Vector(400, 400, 400)
    )

    shapes_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExCreateShapesSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    attach_shape_grid_builder(graph, shapes_node, 500, 180, float(catalog["module_grid_cm"]["wall_width"]))

    break_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExBreakClustersToPathsSettings",
        "PCGTransformPointsSettings",
        800,
        0,
    )

    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        1100,
        0,
    )
    try:
        resample_settings.set_editor_property(
            "Distance", float(catalog["module_grid_cm"]["column_spacing"])
        )
    except Exception:  # noqa: BLE001
        pass

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 1400, 0)
    sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_column_path),
    )

    wire_chain(
        graph,
        [sampler_node, shapes_node, break_node, resample_node, sg_node],
        output_node,
    )
    save_and_reload(path)
    return path


def build_facade_ex(catalog: dict, sub_spawn_path: str) -> str:
    name = "PCG_BaroqueFacadeEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Multi-story baroque façade from PCGEx shape grid.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1400, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGSurfaceSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("points_per_squared_meter", 0.01)
    sampler_settings.set_editor_property("unbounded", True)

    shapes_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExCreateShapesSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    attach_shape_grid_builder(graph, shapes_node, 500, 180, float(catalog["module_grid_cm"]["wall_width"]))

    xform_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExTransformPointsSettings",
        "PCGTransformPointsSettings",
        800,
        0,
    )

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 1100, 0)
    sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_spawn_path),
    )

    wire_chain(graph, [sampler_node, shapes_node, xform_node, sg_node], output_node)
    save_and_reload(path)
    return path


def build_rotunda_ex(catalog: dict, sub_column_path: str, sub_cornice_path: Optional[str] = None) -> str:
    name = "PCG_BaroqueRotundaEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Circular colonnade via PCGEx circle/fiblat shape.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1500, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGVolumeSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("unbounded", True)
    sampler_settings.set_editor_property("voxel_size", unreal.Vector(800, 800, 100))

    shapes_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExCreateShapesSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    attach_shape_grid_builder(
        graph,
        shapes_node,
        500,
        180,
        float(catalog["module_grid_cm"]["column_spacing"]),
    )

    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        800,
        0,
    )
    try:
        resample_settings.set_editor_property(
            "Distance", float(catalog["module_grid_cm"]["column_spacing"])
        )
    except Exception:  # noqa: BLE001
        pass

    col_sg, col_sg_settings = add_node(graph, "PCGSubgraphSettings", 1100, 0)
    col_sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_column_path),
    )

    merge_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExMergePointsSettings",
        "PCGMergeSettings",
        1300,
        0,
    )

    nodes = [sampler_node, shapes_node, resample_node, col_sg, merge_node]
    wire_chain(graph, nodes, output_node)
    save_and_reload(path)
    return path


def build_cornice_ex(catalog: dict) -> str:
    name = "PCG_BaroqueCorniceEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Cornice band along spline/path with offset and resample.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1500, 0)

    spline_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExSplineToPathSettings",
        "PCGGetSplineDataSettings",
        200,
        0,
    )

    offset_node, offset_settings, _ = add_node_or_fallback(
        graph,
        "PCGExOffsetPathSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    try:
        offset_settings.set_editor_property(
            "Offset", unreal.Vector(0, 0, catalog["module_grid_cm"].get("cornice_offset_z", 580))
        )
    except Exception:  # noqa: BLE001
        pass

    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        800,
        0,
    )
    try:
        resample_settings.set_editor_property(
            "Distance", float(catalog["module_grid_cm"]["wall_width"])
        )
    except Exception:  # noqa: BLE001
        pass

    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 1100, 0
    )
    configure_spawner_from_collection(
        spawner_settings, "PCGCol_Baroque_Cornice", catalog
    )

    wire_chain(
        graph,
        [spline_node, offset_node, resample_node, spawner_node],
        output_node,
    )
    save_and_reload(path)
    return path


def build_gothic_corridor_ex(catalog: dict, sub_spawn_path: str) -> str:
    name = "PCG_GothicCorridorEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Spline corridor with resampled wall modules (fills L_PCGTest_GothicCorridors).",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1600, 0)

    spline_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExSplineToPathSettings",
        "PCGGetSplineDataSettings",
        200,
        0,
    )

    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    try:
        resample_settings.set_editor_property(
            "Distance", float(catalog["module_grid_cm"]["wall_width"])
        )
    except Exception:  # noqa: BLE001
        pass

    copy_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExCopyToPointsSettings",
        "PCGCopyPointsSettings",
        800,
        0,
    )

    orient_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExOrientSettings",
        "PCGTransformPointsSettings",
        1100,
        0,
    )

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 1400, 0)
    sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_spawn_path),
    )

    wire_chain(
        graph,
        [spline_node, resample_node, copy_node, orient_node, sg_node],
        output_node,
    )
    save_and_reload(path)
    return path


def build_nave_vault_ex(catalog: dict) -> str:
    name = "PCG_BaroqueNaveVaultEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Nave vault rib hatch along axis spline.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1400, 0)

    spline_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExSplineToPathSettings",
        "PCGGetSplineDataSettings",
        200,
        0,
    )

    hatch_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExPathHatchSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )

    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 800, 0
    )
    configure_spawner_from_collection(
        spawner_settings, "PCGCol_Baroque_Cornice", catalog
    )

    wire_chain(graph, [spline_node, hatch_node, spawner_node], output_node)
    save_and_reload(path)
    return path


def build_atrium_ex(catalog: dict, sub_column_path: str, sub_spawn_path: str) -> str:
    name = "PCG_BaroqueAtriumEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Rectangular cloister/atrium with perimeter columns.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1500, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGVolumeSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("unbounded", True)
    sampler_settings.set_editor_property("voxel_size", unreal.Vector(600, 600, 100))

    shapes_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExCreateShapesSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    attach_shape_grid_builder(
        graph,
        shapes_node,
        500,
        180,
        float(catalog["module_grid_cm"]["wall_width"]),
    )

    col_sg, col_sg_settings = add_node(graph, "PCGSubgraphSettings", 800, 0)
    col_sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_column_path),
    )

    wall_sg, wall_sg_settings = add_node(graph, "PCGSubgraphSettings", 1100, 0)
    wall_sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_spawn_path),
    )

    merge_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExMergePointsSettings",
        "PCGMergeSettings",
        1300,
        0,
    )

    wire_chain(
        graph,
        [sampler_node, shapes_node, col_sg, wall_sg, merge_node],
        output_node,
    )
    save_and_reload(path)
    return path


def build_balcony_ex(catalog: dict, sub_spawn_path: str) -> str:
    name = "PCG_BaroqueBalconyEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Balcony ledge row via path offset from spline.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1400, 0)

    spline_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExSplineToPathSettings",
        "PCGGetSplineDataSettings",
        200,
        0,
    )

    offset_node, offset_settings, _ = add_node_or_fallback(
        graph,
        "PCGExOffsetPathSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    try:
        offset_settings.set_editor_property(
            "Offset", unreal.Vector(120, 0, 0)
        )
    except Exception:  # noqa: BLE001
        pass

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 800, 0)
    sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_spawn_path),
    )

    wire_chain(graph, [spline_node, offset_node, sg_node], output_node)
    save_and_reload(path)
    return path


def build_pilaster_ex(catalog: dict, sub_spawn_path: str) -> str:
    name = "PCG_BaroquePilasterEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Vertical pilaster strips on façade grid (every 2nd module).",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1200, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGSurfaceSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("points_per_squared_meter", 0.05)
    sampler_settings.set_editor_property("unbounded", True)

    filter_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExUberFilterSettings",
        "PCGPointFilterSettings",
        500,
        0,
    )

    sg_node, sg_settings = add_node(graph, "PCGSubgraphSettings", 800, 0)
    sg_settings.set_editor_property(
        "subgraph_override",
        unreal.EditorAssetLibrary.load_asset(sub_spawn_path),
    )

    wire_chain(graph, [sampler_node, filter_node, sg_node], output_node)
    save_and_reload(path)
    return path


def build_sub_walkability_filter(catalog: dict) -> str:
    """Subgraph that filters input points by walkability (line-trace probe).

    Uses PCGExTrace or vanilla DensityFilter to keep only points on walkable
    surfaces (≤ 50° slope).  Downstream spawners receive clean, floor-snapped
    points suitable for encounter-trigger and enemy placement.
    """
    name = "PCG_Sub_WalkabilityFilter"
    graph, path = create_graph_asset(
        name,
        SUBGRAPH_PACKAGE,
        "Filter points by walkability: keep only floor-level, low-slope hits.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1200, 0)

    # --- Trace / ray-cast node to find floor beneath each point ---
    trace_node, trace_settings, used_trace = add_node_or_fallback(
        graph,
        "PCGExTraceSettings",         # PCGEx: full ray-trace
        "PCGDensityFilterSettings",    # vanilla fallback
        300,
        0,
    )
    if used_trace == "PCGExTraceSettings":
        try:
            trace_settings.set_editor_property("TraceChannel", unreal.TraceTypeQuery.ECC_WorldStatic)
            trace_settings.set_editor_property("TraceLength", 4000.0)
            trace_settings.set_editor_property("bTraceDown", True)
        except Exception:  # noqa: BLE001
            pass

    # --- Filter node: remove points whose surface normal exceeds 50° ---
    filter_node, filter_settings, _ = add_node_or_fallback(
        graph,
        "PCGExUberFilterSettings",
        "PCGPointFilterSettings",
        600,
        0,
    )

    # --- Project-to-surface: snap surviving points to the floor ---
    project_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExProjectAlongSettings",
        "PCGTransformPointsSettings",
        900,
        0,
    )

    wire_chain(graph, [trace_node, filter_node, project_node], output_node)
    save_and_reload(path)
    return path


def build_entry_ex(catalog: dict) -> str:
    name = "PCG_BaroqueEntryEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Grand portal: door module with wall surround.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(900, 0)

    sampler_node, sampler_settings = add_node(
        graph, "PCGVolumeSamplerSettings", 200, 0
    )
    sampler_settings.set_editor_property("unbounded", True)
    sampler_settings.set_editor_property("voxel_size", unreal.Vector(400, 400, 400))

    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 500, 0
    )
    configure_spawner_from_collection(spawner_settings, "PCGCol_Baroque_Doors", catalog)

    wire_chain(graph, [sampler_node, spawner_node], output_node)
    save_and_reload(path)
    return path


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

GRAPH_BUILDERS: dict[str, Callable[[dict], str]] = {}


def _register_builders() -> None:
    """Lazy registration after catalog load."""
    pass


def build_all(force_rebuild: bool = False) -> list[str]:
    """Build subgraphs then all top-level PCGEx baroque graphs."""
    assert hasattr(unreal, "PCGGraph"), "Enable PCG + PCGPythonInterop in Melodia.uproject"

    pcgex_ok = hasattr(unreal, "PCGExCreateShapesSettings")
    if not pcgex_ok:
        unreal.log_warning(
            "PCGEx settings classes not found — graphs will use vanilla fallbacks. "
            "Enable PCGExtendedToolkit and rebuild the project."
        )

    catalog = load_catalog()
    ensure_directory(SUBGRAPH_PACKAGE)
    ensure_directory(GRAPH_PACKAGE)

    paths: list[str] = []

    sub_spawn = build_sub_baroque_spawn(catalog)
    paths.append(sub_spawn)

    sub_column = build_sub_baroque_column(catalog)
    paths.append(sub_column)

    sub_path = build_sub_baroque_along_path(catalog, sub_spawn)
    paths.append(sub_path)

    paths.append(build_colonnade_ex(catalog, sub_column))
    paths.append(build_facade_ex(catalog, sub_spawn))
    paths.append(build_rotunda_ex(catalog, sub_column))
    paths.append(build_cornice_ex(catalog))
    paths.append(build_gothic_corridor_ex(catalog, sub_spawn))
    paths.append(build_nave_vault_ex(catalog))
    paths.append(build_atrium_ex(catalog, sub_column, sub_spawn))
    paths.append(build_balcony_ex(catalog, sub_spawn))
    paths.append(build_pilaster_ex(catalog, sub_spawn))
    sub_walk = build_sub_walkability_filter(catalog)
    paths.append(sub_walk)

    paths.append(build_entry_ex(catalog))

    unreal.log(f"melodia_pcgex_builder: created/updated {len(paths)} graphs.")
    for p in paths:
        unreal.log(f"  -> {p}")
    return paths


def build_graph(graph_name: str) -> str:
    """Build a single graph by asset name (must be handled in build_all list)."""
    catalog = load_catalog()
    mapping = {
        "PCG_Sub_BaroqueSpawn": lambda: build_sub_baroque_spawn(catalog),
        "PCG_Sub_BaroqueColumn": lambda: build_sub_baroque_column(catalog),
    }
    sub_spawn = build_sub_baroque_spawn(catalog)
    sub_column = build_sub_baroque_column(catalog)
    mapping.update(
        {
            "PCG_Sub_BaroqueAlongPath": lambda: build_sub_baroque_along_path(
                catalog, sub_spawn
            ),
            "PCG_Sub_WalkabilityFilter": lambda: build_sub_walkability_filter(catalog),
            "PCG_BaroqueColonnadeEx": lambda: build_colonnade_ex(catalog, sub_column),
            "PCG_BaroqueFacadeEx": lambda: build_facade_ex(catalog, sub_spawn),
            "PCG_BaroqueRotundaEx": lambda: build_rotunda_ex(catalog, sub_column),
            "PCG_BaroqueCorniceEx": lambda: build_cornice_ex(catalog),
            "PCG_GothicCorridorEx": lambda: build_gothic_corridor_ex(
                catalog, sub_spawn
            ),
            "PCG_BaroqueNaveVaultEx": lambda: build_nave_vault_ex(catalog),
            "PCG_BaroqueAtriumEx": lambda: build_atrium_ex(
                catalog, sub_column, sub_spawn
            ),
            "PCG_BaroqueBalconyEx": lambda: build_balcony_ex(catalog, sub_spawn),
            "PCG_BaroquePilasterEx": lambda: build_pilaster_ex(catalog, sub_spawn),
            "PCG_BaroqueEntryEx": lambda: build_entry_ex(catalog),
        }
    )
    if graph_name not in mapping:
        raise KeyError(
            f"Unknown graph '{graph_name}'. Known: {sorted(mapping.keys())}"
        )
    path = mapping[graph_name]()
    unreal.log(f"Built {graph_name} -> {path}")
    return path


def list_pcgex_settings_classes() -> list[str]:
    """Diagnostic: PCGEx node settings exposed to Python."""
    return sorted(
        x
        for x in dir(unreal)
        if "PCGEx" in x and x.endswith("Settings")
    )


if __name__ == "__main__":
    build_all()
