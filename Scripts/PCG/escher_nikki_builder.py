"""
Melodia Escher / Infinity Nikki PCGEx graph builder.

Companion to melodia_pcgex_builder.py.  Builds PCG graph assets that use the
escher_nikki mesh collections (impossible staircases, floating islands, organic
tendrils, etc.) defined in baroque_mesh_catalog.json.

Run inside Unreal Editor (Python 3.11) after enabling PCG, PCGPythonInterop,
and PCGExtendedToolkit.

Usage (Output Log -> Python):
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "escher_nikki_builder",
        r"G:/Melodia/Scripts/PCG/escher_nikki_builder.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.build_all()

Or: mod.build_graph("PCG_EscherPenroseStairEx")
"""

from __future__ import annotations

import json
import os
from typing import Callable, Optional

try:
    import unreal
except ImportError as exc:  # pragma: no cover - only importable in UE
    raise RuntimeError(
        "escher_nikki_builder must run inside Unreal Editor Python"
    ) from exc


# ---------------------------------------------------------------------------
# Paths (shared with melodia_pcgex_builder)
# ---------------------------------------------------------------------------

PROJECT_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
)
CATALOG_PATH = os.path.join(PROJECT_ROOT, "Scripts", "PCG", "baroque_mesh_catalog.json")
GRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs/PCGEx/EscherNikki"
SUBGRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs/PCGEx/Subgraphs"


# ---------------------------------------------------------------------------
# Helpers (imported pattern from melodia_pcgex_builder)
# ---------------------------------------------------------------------------


def load_catalog() -> dict:
    with open(CATALOG_PATH, encoding="utf-8") as fh:
        return json.load(fh)


def pin_labels(pins) -> list[str]:
    return [
        str(p.get_editor_property("properties").get_editor_property("label"))
        for p in pins
    ]


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def save_and_reload(asset_path: str) -> None:
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    pkg = unreal.find_package(asset_path)
    if pkg:
        unreal.EditorLoadingAndSavingUtils.reload_packages([pkg])


def get_settings_class(name: str):
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


def add_node_or_fallback(graph, primary: str, fallback: str, x: int, y: int):
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
    input_node = graph.get_input_node()
    graph.add_edge(input_node, "In", nodes[0], _main_input_pin(nodes[0]))
    for a, b in zip(nodes, nodes[1:]):
        wire_output_to_input(graph, a, b)
    graph.add_edge(nodes[-1], _main_output_pin(nodes[-1]), output_node, "Out")


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


def configure_spawner_from_escher_collection(
    spawner_settings,
    collection_key: str,
    catalog: dict,
) -> None:
    """Weighted mesh list from escher_nikki section of the catalog."""
    escher = catalog.get("escher_nikki", {})
    coll = escher.get(collection_key)
    if not coll:
        unreal.log_warning(f"Escher collection key missing: {collection_key}")
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
# Graph recipes — Escher / Infinity Nikki
# ---------------------------------------------------------------------------


def build_penrose_stair_ex(catalog: dict) -> str:
    """Escher infinite staircase: helix point distribution + stair mesh spawn.

    Uses the custom C++ UPCGEscherStaircaseSettings element (when available)
    as the point generator, then spawns stair-tread meshes from the Escher
    Nikki stair collection.  Falls back to PCGEx shape grid when the custom
    element isn't compiled in.
    """
    name = "PCG_EscherPenroseStairEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Escher infinite staircase loop with tread mesh spawning.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1600, 0)

    # Try the custom C++ Escher element first; fall back to shape creation
    escher_node, escher_settings, used_escher = add_node_or_fallback(
        graph,
        "PCGEscherStaircaseSettings",      # custom C++ element
        "PCGExCreateShapesSettings",        # PCGEx fallback
        200,
        0,
    )
    if used_escher == "PCGEscherStaircaseSettings":
        try:
            escher_settings.set_editor_property("StepCount", 16)
            escher_settings.set_editor_property("StepHeight", 25.0)
            escher_settings.set_editor_property("LoopRadius", 600.0)
        except Exception:  # noqa: BLE001
            pass

    # Orient points so stair treads face the helix tangent
    orient_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExOrientSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )

    # Spawn stair-tread meshes
    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 800, 0
    )
    configure_spawner_from_escher_collection(
        spawner_settings, "PCGCol_EscherNikki_Stairs", catalog
    )

    # Walkability filter subgraph to mark safe spawn points
    sg_walk, sg_walk_settings = add_node(graph, "PCGSubgraphSettings", 1100, 0)
    walk_path = f"{SUBGRAPH_PACKAGE}/PCG_Sub_WalkabilityFilter"
    walk_asset = unreal.EditorAssetLibrary.load_asset(walk_path)
    if walk_asset:
        sg_walk_settings.set_editor_property("subgraph_override", walk_asset)

    wire_chain(
        graph,
        [escher_node, orient_node, spawner_node, sg_walk],
        output_node,
    )
    save_and_reload(path)
    return path


def build_floating_island_ex(catalog: dict) -> str:
    """Floating island: scattered rock + organic meshes on tessellated surface.

    Uses the Penrose tessellation C++ element to distribute points in an
    aperiodic pattern, then spawns rocks and organic (vine/moss) meshes.
    """
    name = "PCG_EscherFloatingIslandEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Floating island with Penrose-distributed rocks and organic details.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1600, 0)

    # Tessellation element for aperiodic surface distribution
    tess_node, tess_settings, used_tess = add_node_or_fallback(
        graph,
        "PCGTessellationSettings",
        "PCGVolumeSamplerSettings",
        200,
        0,
    )
    if used_tess == "PCGTessellationSettings":
        try:
            tess_settings.set_editor_property("TileShape", unreal.EPCGTileShape.PENROSE)
            tess_settings.set_editor_property("TileScale", 200.0)
        except Exception:  # noqa: BLE001
            pass

    # Split into two branches: rocks + organic
    rock_spawner, rock_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 600, -200
    )
    configure_spawner_from_escher_collection(
        rock_settings, "PCGCol_EscherNikki_IslandRocks", catalog
    )

    organic_spawner, organic_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 600, 200
    )
    configure_spawner_from_escher_collection(
        organic_settings, "PCGCol_EscherNikki_Organic", catalog
    )

    # Merge both branches
    merge_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExMergePointsSettings",
        "PCGMergeSettings",
        1000,
        0,
    )

    # Walkability filter
    sg_walk, sg_walk_settings = add_node(graph, "PCGSubgraphSettings", 1300, 0)
    walk_path = f"{SUBGRAPH_PACKAGE}/PCG_Sub_WalkabilityFilter"
    walk_asset = unreal.EditorAssetLibrary.load_asset(walk_path)
    if walk_asset:
        sg_walk_settings.set_editor_property("subgraph_override", walk_asset)

    wire_chain(
        graph,
        [tess_node, rock_spawner, merge_node, sg_walk],
        output_node,
    )
    # Wire organic branch into merge too
    try:
        graph.add_edge(tess_node, "Out", organic_spawner, _main_input_pin(organic_spawner))
        graph.add_edge(organic_spawner, "Out", merge_node, "In")
    except Exception:  # noqa: BLE001
        pass

    save_and_reload(path)
    return path


def build_impossible_archway_ex(catalog: dict) -> str:
    """Impossible archway: recursive arch element + railing/tile spawn.

    Uses the custom C++ UPCGRecursiveArchSettings to generate nested arch
    tiers, then spawns railing and tile meshes from the Escher Nikki
    collections at each tier.
    """
    name = "PCG_EscherImpossibleArchEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Recursive impossible archway with railing and tile details.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1400, 0)

    # Recursive arch generator
    arch_node, arch_settings, used_arch = add_node_or_fallback(
        graph,
        "PCGRecursiveArchSettings",
        "PCGExCreateShapesSettings",
        200,
        0,
    )
    if used_arch == "PCGRecursiveArchSettings":
        try:
            arch_settings.set_editor_property("ArchWidth", 400.0)
            arch_settings.set_editor_property("ArchHeight", 600.0)
            arch_settings.set_editor_property("RecursionDepth", 3)
            arch_settings.set_editor_property("ScaleFactor", 0.618)
        except Exception:  # noqa: BLE001
            pass

    # Spawn railing posts along arch curves
    rail_spawner, rail_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 600, 0
    )
    configure_spawner_from_escher_collection(
        rail_settings, "PCGCol_EscherNikki_Railings", catalog
    )

    # Spawn floor tiles at arch base
    tile_spawner, tile_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 600, 300
    )
    configure_spawner_from_escher_collection(
        tile_settings, "PCGCol_EscherNikki_Tiles", catalog
    )

    merge_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExMergePointsSettings",
        "PCGMergeSettings",
        1000,
        0,
    )

    wire_chain(graph, [arch_node, rail_spawner, merge_node], output_node)
    try:
        graph.add_edge(arch_node, "Out", tile_spawner, _main_input_pin(tile_spawner))
        graph.add_edge(tile_spawner, "Out", merge_node, "In")
    except Exception:  # noqa: BLE001
        pass

    save_and_reload(path)
    return path


def build_gravity_bridge_ex(catalog: dict) -> str:
    """Gravity-defying bridge: gravity zone element + bridge/corridor spawn.

    Uses UPCGGravityZoneSettings to stamp non-standard GravityDir on points,
    then spawns bridge and corridor meshes that appear to defy gravity.
    """
    name = "PCG_EscherGravityBridgeEx"
    graph, path = create_graph_asset(
        name,
        GRAPH_PACKAGE,
        "Gravity-defying bridge with non-standard gravity orientation.",
    )
    clear_user_nodes(graph)
    output_node = graph.get_output_node()
    output_node.set_node_position(1400, 0)

    # Spline path for bridge route
    spline_node, _, _ = add_node_or_fallback(
        graph,
        "PCGExSplineToPathSettings",
        "PCGGetSplineDataSettings",
        200,
        0,
    )

    # Resample to corridor module width
    resample_node, resample_settings, _ = add_node_or_fallback(
        graph,
        "PCGExResamplePathSettings",
        "PCGTransformPointsSettings",
        500,
        0,
    )
    grid = catalog.get("module_grid_cm", {})
    try:
        resample_settings.set_editor_property(
            "Distance", float(grid.get("wall_width", 400))
        )
    except Exception:  # noqa: BLE001
        pass

    # Gravity zone: stamp sideways gravity on all points
    grav_node, grav_settings, used_grav = add_node_or_fallback(
        graph,
        "PCGGravityZoneSettings",
        "PCGTransformPointsSettings",
        800,
        0,
    )
    if used_grav == "PCGGravityZoneSettings":
        try:
            # Sideways gravity (Y-axis) for impossible bridge
            grav_settings.set_editor_property("GravityDir", unreal.Vector(0.0, 1.0, 0.0))
        except Exception:  # noqa: BLE001
            pass

    # Spawn bridge/corridor meshes
    spawner_node, spawner_settings = add_node(
        graph, "PCGStaticMeshSpawnerSettings", 1100, 0
    )
    # Use baroque bridges for now; escher bridge meshes TBD
    coll = catalog["collections"].get("PCGCol_Baroque_Bridges")
    if coll:
        entries = []
        for item in coll["entries"]:
            entry = mesh_entry(item["mesh"], item.get("weight", 1))
            if entry:
                entries.append(entry)
        if entries:
            selector = spawner_settings.get_editor_property("mesh_selector_parameters")
            selector.set_editor_property("mesh_entries", entries)

    wire_chain(
        graph,
        [spline_node, resample_node, grav_node, spawner_node],
        output_node,
    )
    save_and_reload(path)
    return path


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------


def build_all() -> list[str]:
    """Build all Escher / Infinity Nikki PCG graphs."""
    assert hasattr(unreal, "PCGGraph"), "Enable PCG + PCGPythonInterop in Melodia.uproject"

    catalog = load_catalog()
    ensure_directory(SUBGRAPH_PACKAGE)
    ensure_directory(GRAPH_PACKAGE)

    paths: list[str] = []

    paths.append(build_penrose_stair_ex(catalog))
    paths.append(build_floating_island_ex(catalog))
    paths.append(build_impossible_archway_ex(catalog))
    paths.append(build_gravity_bridge_ex(catalog))

    unreal.log(f"escher_nikki_builder: created/updated {len(paths)} graphs.")
    for p in paths:
        unreal.log(f"  -> {p}")
    return paths


def build_graph(graph_name: str) -> str:
    """Build a single Escher/Nikki graph by name."""
    catalog = load_catalog()
    mapping: dict[str, Callable] = {
        "PCG_EscherPenroseStairEx": lambda: build_penrose_stair_ex(catalog),
        "PCG_EscherFloatingIslandEx": lambda: build_floating_island_ex(catalog),
        "PCG_EscherImpossibleArchEx": lambda: build_impossible_archway_ex(catalog),
        "PCG_EscherGravityBridgeEx": lambda: build_gravity_bridge_ex(catalog),
    }
    if graph_name not in mapping:
        raise KeyError(
            f"Unknown graph '{graph_name}'. Known: {sorted(mapping.keys())}"
        )
    path = mapping[graph_name]()
    unreal.log(f"Built {graph_name} -> {path}")
    return path


if __name__ == "__main__":
    build_all()
