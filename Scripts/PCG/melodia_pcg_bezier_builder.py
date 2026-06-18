"""
Melodia Bezier PCG graph builder — environment art portfolio toolkit.

Creates ready-to-use PCG graph assets under /Game/_PROJECT/PCG/Graphs/.
All graphs use Melodia custom Bezier elements with mesh spawners wired.

Run inside Unreal Editor after rebuilding MelodiaMelusina_PROD:

    Melodia.BuildPCGGraphs

Or Python Output Log:

    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "melodia_pcg_bezier_builder",
        r"G:/Melodia/Scripts/PCG/melodia_pcg_bezier_builder.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.build_all()
"""

from __future__ import annotations

import importlib.util
import json
import os

try:
    import unreal
except ImportError as exc:  # pragma: no cover
    raise RuntimeError("melodia_pcg_bezier_builder must run inside Unreal Editor") from exc


PROJECT_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
)
CATALOG_PATH = os.path.join(PROJECT_ROOT, "Scripts", "PCG", "baroque_mesh_catalog.json")
GRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs"

_STANDARDS = None


def _standards():
    global _STANDARDS
    if _STANDARDS is None:
        spec = importlib.util.spec_from_file_location(
            "melodia_pcg_standards",
            os.path.join(os.path.dirname(os.path.abspath(__file__)), "melodia_pcg_standards.py"),
        )
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        _STANDARDS = mod
    return _STANDARDS


COLLECTION_PLACEHOLDER_ROLE = {
    "PCGCol_Baroque_Walls": "wall",
    "PCGCol_Baroque_Columns": "column",
    "PCGCol_Baroque_Cornice": "cornice",
    "PCGCol_Baroque_Roof": "floor",
    "PCGCol_Baroque_Doors": "wall",
    "PCGCol_Baroque_Bridges": "bridge",
    "PCGCol_Baroque_Towers": "tower",
    "PCGCol_Environment_Rocks": "scatter",
    "PCGCol_Environment_GroundCover": "scatter",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def load_catalog() -> dict:
    with open(CATALOG_PATH, encoding="utf-8") as fh:
        return json.load(fh)


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def get_settings_class(name: str):
    return getattr(unreal, name, None)


def add_node(graph, settings_class_name: str, x: int, y: int):
    cls = get_settings_class(settings_class_name)
    if cls is None:
        raise RuntimeError(
            f"Settings class '{settings_class_name}' not found. "
            "Rebuild MelodiaMelusina_PROD and restart the editor."
        )
    node, settings = graph.add_node_of_type(cls)
    node.set_node_position(x, y)
    return node, settings


def get_output_pin_labels(node) -> list[str]:
    labels: list[str] = []
    for pin in node.get_editor_property("output_pins"):
        props = pin.get_editor_property("properties")
        labels.append(str(props.get_editor_property("label")))
    return labels


def get_input_pin_labels(node) -> list[str]:
    labels: list[str] = []
    for pin in node.get_editor_property("input_pins"):
        props = pin.get_editor_property("properties")
        labels.append(str(props.get_editor_property("label")))
    return labels


def _sampler_input_pin(sampler_node) -> str:
    labels = get_input_pin_labels(sampler_node)
    if "Volume" in labels:
        return "Volume"
    if "Surface" in labels:
        return "Surface"
    return "In"


def _pin_has_edges(pin) -> bool:
    return len(pin.get_editor_property("edges")) > 0


def _find_volume_sampler_node(graph) -> unreal.PCGNode | None:
    for node in graph.nodes:
        if type(node.get_settings()).__name__ == "PCGVolumeSamplerSettings":
            return node
    return None


def _edge_target_node(edge):
    for prop in ("input_pin", "InputPin", "target_pin", "TargetPin"):
        try:
            pin = edge.get_editor_property(prop)
            if pin:
                return pin.get_editor_property("node")
        except Exception:
            pass
    return None


def ensure_volume_input_bootstrap(graph) -> None:
    """Wire Input -> volume sampler -> unwired In pins on graph nodes (required for Bezier elements)."""
    input_node = graph.get_input_node()
    out_node = graph.get_output_node()
    sampler_node = _find_volume_sampler_node(graph)

    if sampler_node is None:
        input_has_edges = any(
            _pin_has_edges(pin) for pin in input_node.get_editor_property("output_pins")
        )
        if input_has_edges:
            return

        sampler_node, sampler_settings = add_node(graph, "PCGVolumeSamplerSettings", -950, 420)
        sampler_settings.set_editor_property("unbounded", True)
        sampler_settings.set_editor_property("voxel_size", unreal.Vector(400, 400, 400))

        src_labels = get_output_pin_labels(input_node)
        src_pin = src_labels[0] if src_labels else "In"
        graph.add_edge(input_node, src_pin, sampler_node, _sampler_input_pin(sampler_node))

    consumers: list = []
    keep = {input_node, out_node, sampler_node}
    for node in graph.nodes:
        if node in keep:
            continue
        if "In" not in get_input_pin_labels(node):
            continue
        for pin in node.get_editor_property("input_pins"):
            props = pin.get_editor_property("properties")
            if str(props.get_editor_property("label")) != "In":
                continue
            if not _pin_has_edges(pin):
                graph.add_edge(sampler_node, "Out", node, "In")
                consumers.append(node)
            break

    if consumers:
        try:
            graph.remove_edge(sampler_node, "Out", out_node, "Out")
        except Exception:
            pass
        return

    # Grid-only / self-contained graphs (e.g. DreamWalls): never bypass content with sampler->output.
    # An orphan volume sampler is harmless; wiring it to Output replaces real ISM output with empty points.


def save_graph(graph, asset_path: str, *, bootstrap_volume: bool = True) -> None:
    if bootstrap_volume:
        ensure_volume_input_bootstrap(graph)
    graph.set_editor_property("is_standalone_graph", True)
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    pkg = unreal.find_package(asset_path)
    if pkg:
        unreal.EditorLoadingAndSavingUtils.reload_packages([pkg])


def create_or_load_graph(asset_name: str) -> unreal.PCGGraph:
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    ensure_directory(GRAPH_PACKAGE)
    existing = unreal.load_asset(asset_path)
    if existing:
        existing.set_editor_property("is_standalone_graph", True)
        return existing
    factory = unreal.PCGGraphFactory()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    graph = asset_tools.create_asset(asset_name, GRAPH_PACKAGE, unreal.PCGGraph, factory)
    graph.set_editor_property("is_standalone_graph", True)
    return graph


def clear_graph(graph) -> None:
    keep = {graph.get_input_node(), graph.get_output_node()}
    for node in list(graph.nodes):
        if node not in keep:
            graph.remove_node(node)


def mesh_entry(mesh_path: str, weight: int = 1):
    mesh = unreal.load_asset(mesh_path)
    if not mesh:
        unreal.log_warning(f"Mesh missing: {mesh_path}")
        return None
    entry = unreal.PCGMeshSelectorWeightedEntry()
    desc = entry.get_editor_property("descriptor")
    desc.set_editor_property("static_mesh", mesh)
    entry.set_editor_property("descriptor", desc)
    entry.set_editor_property("weight", weight)
    return entry


def _mesh_paths_from_collection(catalog: dict, collection_key: str, tags: list[str] | None = None) -> list[str]:
    coll = catalog.get("collections", {}).get(collection_key, {})
    paths = []
    for item in coll.get("entries", []):
        if tags and not any(t in item.get("tags", []) for t in tags):
            continue
        paths.append(item["mesh"])
    return paths


def configure_spawner_meshes(spawner_settings, mesh_paths: list[str], *, collection: str | None = None) -> None:
    std = _standards()
    if std.PORTFOLIO_PLACEHOLDER_MODE and collection:
        role = COLLECTION_PLACEHOLDER_ROLE.get(collection, "wall")
        std.configure_placeholder_spawner(spawner_settings, role)
        return
    entries = []
    for path in mesh_paths:
        entry = mesh_entry(path)
        if entry:
            entries.append(entry)
    if not entries:
        entry = mesh_entry("/Game/_PROJECT/MelusinasHouse/SM_wallhi")
        if entry:
            entries.append(entry)
    if not entries:
        unreal.log_warning("configure_spawner_meshes: no valid mesh entries — spawner will produce nothing")
        return
    selector = spawner_settings.get_editor_property("mesh_selector_parameters")
    selector.set_editor_property("mesh_entries", entries)


def add_spawner(graph, catalog: dict, x: int, y: int, collection: str, tags: list[str] | None = None):
    node, settings = add_node(graph, "PCGStaticMeshSpawnerSettings", x, y)
    paths = _mesh_paths_from_collection(catalog, collection, tags)
    configure_spawner_meshes(settings, paths, collection=collection)
    return node


def apply_portfolio_ornament_scatter(scatter_settings) -> None:
    """Disable scatter jitter when portfolio placeholder mode is active."""
    _standards().configure_portfolio_ornament_scatter(scatter_settings)


def apply_preset(settings, preset_name: str) -> None:
    """preset_name matches EMelodiaBezierLayoutPreset without prefix."""
    enum = getattr(unreal, "EMelodiaBezierLayoutPreset", None)
    if enum is None:
        return
    value = getattr(enum, preset_name, None)
    if value is None:
        return
    settings.set_editor_property("layout_preset", value)
    if hasattr(settings, "apply_layout_preset"):
        settings.apply_layout_preset()


def wire_to_output(graph, source_node, source_pin: str, output_node) -> None:
    graph.add_edge(source_node, source_pin, output_node, "Out")


def get_output_pin_labels(node) -> list[str]:
    labels: list[str] = []
    for pin in node.get_editor_property("output_pins"):
        props = pin.get_editor_property("properties")
        labels.append(str(props.get_editor_property("label")))
    return labels


def wire_separable_bezier_node(
    graph,
    source_node,
    source_settings,
    catalog: dict,
    out_node,
    spawners: list[tuple[int, str, list[str] | None, str]],
    *,
    scatter_pin: str = "Out_Path",
) -> None:
    """Wire terrace/cloister nodes; fall back to combined Out when separate pins are unavailable."""
    source_settings.set_editor_property("emit_separate_pins", True)
    labels = get_output_pin_labels(source_node)

    if "Out_Path" in labels:
        for y, collection, tags, pin_name in spawners:
            if pin_name not in labels:
                continue
            spawner = add_spawner(graph, catalog, 150, y, collection, tags)
            graph.add_edge(source_node, pin_name, spawner, "In")
            wire_to_output(graph, spawner, "Out", out_node)
        bounds_pin = scatter_pin if scatter_pin in labels else labels[0]
        attach_environment_scatter(graph, catalog, source_node, bounds_pin, out_node)
        return

    # Older editor module: runtime emits combined data on the default Out pin only.
    source_settings.set_editor_property("emit_separate_pins", False)
    y, collection, tags, _pin = spawners[0] if spawners else (0, "PCGCol_Baroque_Walls", ["short"], "Out")
    spawner = add_spawner(graph, catalog, 150, y, collection, tags)
    graph.add_edge(source_node, "Out", spawner, "In")
    wire_to_output(graph, spawner, "Out", out_node)
    attach_environment_scatter(graph, catalog, source_node, "Out", out_node)


def attach_environment_scatter(
    graph,
    catalog: dict,
    bounds_source_node,
    bounds_source_pin: str,
    out_node,
    *,
    y: int = 420,
    rock_count: int = 72,
    ground_count: int = 48,
) -> None:
    """Add terrain-aware rock + ground-cover scatter branch wired to graph output."""
    scatter_kind = getattr(unreal, "EMelodiaLandscapeScatterKind", None)

    rock_scatter, rock_settings = add_node(graph, "PCGMelodiaLandscapeScatterSettings", 120, y)
    rock_settings.set_editor_property("target_count", rock_count)
    rock_settings.set_editor_property("bounds_margin", 2000.0)
    rock_settings.set_editor_property("max_slope_degrees", 42.0)
    if scatter_kind is not None:
        rock_settings.set_editor_property("scatter_kind", scatter_kind.ROCKS)

    rock_spawner = add_spawner(graph, catalog, 420, y, "PCGCol_Environment_Rocks")

    ground_scatter, ground_settings = add_node(graph, "PCGMelodiaLandscapeScatterSettings", 120, y + 200)
    ground_settings.set_editor_property("target_count", ground_count)
    ground_settings.set_editor_property("bounds_margin", 2000.0)
    ground_settings.set_editor_property("min_slope_degrees", 0.0)
    ground_settings.set_editor_property("max_slope_degrees", 28.0)
    if scatter_kind is not None:
        ground_settings.set_editor_property("scatter_kind", scatter_kind.GROUND_COVER)

    ground_spawner = add_spawner(graph, catalog, 420, y + 200, "PCGCol_Environment_GroundCover")

    graph.add_edge(bounds_source_node, bounds_source_pin, rock_scatter, "In")
    graph.add_edge(bounds_source_node, bounds_source_pin, ground_scatter, "In")
    graph.add_edge(rock_scatter, "Out", rock_spawner, "In")
    graph.add_edge(ground_scatter, "Out", ground_spawner, "In")
    wire_to_output(graph, rock_spawner, "Out", out_node)
    wire_to_output(graph, ground_spawner, "Out", out_node)


def add_terrain_snap_node(graph, x: int, y: int):
    """Optional graph-level landscape snap (Bezier nodes also project in C++)."""
    node, settings = add_node(graph, "PCGMelodiaProjectLandscapeSettings", x, y)
    terrain = settings.get_editor_property("terrain_projection")
    terrain.set_editor_property("project_to_landscape", True)
    terrain.set_editor_property("preserve_design_height_offset", True)
    settings.set_editor_property("terrain_projection", terrain)
    return node


# ---------------------------------------------------------------------------
# Graph recipes
# ---------------------------------------------------------------------------


def build_portfolio_terrace_bezier_graph() -> str:
    asset_name = "PCG_PortfolioTerraceBezier"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    terrace_node, terrace_settings = add_node(graph, "PCGBezierTerraceSettings", -500, 0)
    apply_preset(terrace_settings, "PORTFOLIO_TERRACE")
    terrace_settings.set_editor_property("terrace_count", 8)

    out = graph.get_output_node()
    wire_separable_bezier_node(
        graph,
        terrace_node,
        terrace_settings,
        catalog,
        out,
        [
            (-220, "PCGCol_Baroque_Walls", ["short"], "Out_Path"),
            (0, "PCGCol_Baroque_Cornice", None, "Out_Terrace"),
            (220, "PCGCol_Baroque_Walls", ["short"], "Out_Railing"),
        ],
    )

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_path_portfolio_graph() -> str:
    asset_name = "PCG_BezierPathPortfolio"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -400, -100)
    apply_preset(path_settings, "GARDEN_PROMENADE")

    sweep_node, sweep_settings = add_node(graph, "PCGBezierSweepSettings", -400, 120)
    apply_preset(sweep_settings, "GARDEN_PROMENADE")
    sweep_settings.set_editor_property("profile_half_width", 70.0)

    path_spawner = add_spawner(graph, catalog, 120, -100, "PCGCol_Baroque_Walls", ["short"])
    rail_spawner = add_spawner(graph, catalog, 120, 120, "PCGCol_Baroque_Walls", ["short"])

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(sweep_node, "Out", rail_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, rail_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_cloister_ring_graph() -> str:
    asset_name = "PCG_BezierCloisterRing"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    cloister_node, cloister_settings = add_node(graph, "PCGBezierCloisterSettings", -450, 0)
    apply_preset(cloister_settings, "CLOISTER_RING")

    out = graph.get_output_node()
    wire_separable_bezier_node(
        graph,
        cloister_node,
        cloister_settings,
        catalog,
        out,
        [
            (-200, "PCGCol_Baroque_Walls", ["short"], "Out_Path"),
            (0, "PCGCol_Baroque_Cornice", None, "Out_Terrace"),
            (200, "PCGCol_Baroque_Columns", None, "Out_Column"),
        ],
    )

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_colonnade_avenue_graph() -> str:
    asset_name = "PCG_BezierColonnadeAvenue"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -550, -80)
    apply_preset(path_settings, "COLONNADE_AVENUE")

    col_node, col_settings = add_node(graph, "PCGBezierColonnadeSettings", -550, 100)
    apply_preset(col_settings, "COLONNADE_AVENUE")
    col_settings.set_editor_property("dual_row", True)

    path_spawner = add_spawner(graph, catalog, 100, -80, "PCGCol_Baroque_Walls", ["short"])
    col_spawner = add_spawner(graph, catalog, 100, 100, "PCGCol_Baroque_Columns")

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(col_node, "Out", col_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, col_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_garden_promenade_graph() -> str:
    asset_name = "PCG_BezierGardenPromenade"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -600, -120)
    apply_preset(path_settings, "GARDEN_PROMENADE")

    scatter_node, scatter_settings = add_node(graph, "PCGBezierOrnamentScatterSettings", -600, 40)
    apply_preset(scatter_settings, "GARDEN_PROMENADE")
    scatter_settings.set_editor_property("scatter_count", 64)
    apply_portfolio_ornament_scatter(scatter_settings)

    sweep_node, sweep_settings = add_node(graph, "PCGBezierSweepSettings", -600, 200)
    apply_preset(sweep_settings, "GARDEN_PROMENADE")
    sweep_settings.set_editor_property("profile_height_offset", 40.0)

    path_spawner = add_spawner(graph, catalog, 100, -120, "PCGCol_Baroque_Walls", ["short"])
    orn_spawner = add_spawner(graph, catalog, 100, 40, "PCGCol_Baroque_Cornice")
    rail_spawner = add_spawner(graph, catalog, 100, 200, "PCGCol_Baroque_Walls", ["short"])

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(scatter_node, "Out", orn_spawner, "In")
    graph.add_edge(sweep_node, "Out", rail_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, orn_spawner, "Out", out)
    wire_to_output(graph, rail_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out, rock_count=96, ground_count=64)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_bridge_span_graph() -> str:
    asset_name = "PCG_BezierBridgeSpan"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -500, -100)
    apply_preset(path_settings, "BRIDGE_SPAN")

    sweep_node, sweep_settings = add_node(graph, "PCGBezierSweepSettings", -500, 80)
    apply_preset(sweep_settings, "BRIDGE_SPAN")
    sweep_settings.set_editor_property("profile_half_width", 100.0)
    sweep_settings.set_editor_property("profile_height_offset", 140.0)

    bridge_spawner = add_spawner(graph, catalog, 100, -100, "PCGCol_Baroque_Bridges")
    rail_spawner = add_spawner(graph, catalog, 100, 80, "PCGCol_Baroque_Walls", ["short"])

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", bridge_spawner, "In")
    graph.add_edge(sweep_node, "Out", rail_spawner, "In")
    wire_to_output(graph, bridge_spawner, "Out", out)
    wire_to_output(graph, rail_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out, rock_count=48, ground_count=32)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_cathedral_axis_graph() -> str:
    asset_name = "PCG_BezierCathedralAxis"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -650, -120)
    apply_preset(path_settings, "CATHEDRAL_NAVE_AXIS")

    col_node, col_settings = add_node(graph, "PCGBezierColonnadeSettings", -650, 40)
    apply_preset(col_settings, "CATHEDRAL_NAVE_AXIS")
    col_settings.set_editor_property("column_spacing", 400.0)
    col_settings.set_editor_property("row_offset", 350.0)

    sweep_node, sweep_settings = add_node(graph, "PCGBezierSweepSettings", -650, 200)
    apply_preset(sweep_settings, "CATHEDRAL_NAVE_AXIS")
    sweep_settings.set_editor_property("profile_height_offset", 500.0)

    floor_spawner = add_spawner(graph, catalog, 100, -120, "PCGCol_Baroque_Walls", ["short"])
    col_spawner = add_spawner(graph, catalog, 100, 40, "PCGCol_Baroque_Columns")
    corn_spawner = add_spawner(graph, catalog, 100, 200, "PCGCol_Baroque_Cornice")

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", floor_spawner, "In")
    graph.add_edge(col_node, "Out", col_spawner, "In")
    graph.add_edge(sweep_node, "Out", corn_spawner, "In")
    wire_to_output(graph, floor_spawner, "Out", out)
    wire_to_output(graph, col_spawner, "Out", out)
    wire_to_output(graph, corn_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_vista_terrace_graph() -> str:
    asset_name = "PCG_BezierVistaTerrace"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    terrace_node, terrace_settings = add_node(graph, "PCGBezierTerraceSettings", -500, 0)
    apply_preset(terrace_settings, "PORTFOLIO_TERRACE")
    terrace_settings.set_editor_property("terrace_count", 5)
    terrace_settings.set_editor_property("terrace_width", 900.0)
    terrace_settings.set_editor_property("step_drop", 50.0)

    out = graph.get_output_node()
    wire_separable_bezier_node(
        graph,
        terrace_node,
        terrace_settings,
        catalog,
        out,
        [
            (-180, "PCGCol_Baroque_Walls", ["short"], "Out_Path"),
            (0, "PCGCol_Baroque_Cornice", None, "Out_Terrace"),
            (180, "PCGCol_Baroque_Walls", ["short"], "Out_Railing"),
        ],
    )

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_ornament_gallery_graph() -> str:
    asset_name = "PCG_BezierOrnamentGallery"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -550, -80)
    apply_preset(path_settings, "GARDEN_PROMENADE")

    scatter_node, scatter_settings = add_node(graph, "PCGBezierOrnamentScatterSettings", -550, 100)
    apply_preset(scatter_settings, "GARDEN_PROMENADE")
    scatter_settings.set_editor_property("scatter_count", 96)
    scatter_settings.set_editor_property("lateral_jitter", 180.0)
    apply_portfolio_ornament_scatter(scatter_settings)

    path_spawner = add_spawner(graph, catalog, 100, -80, "PCGCol_Baroque_Walls", ["short"])
    orn_spawner = add_spawner(graph, catalog, 100, 100, "PCGCol_Baroque_Cornice")

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(scatter_node, "Out", orn_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, orn_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, path_node, "Out", out, rock_count=80, ground_count=56)

    save_graph(graph, asset_path)
    return asset_path


def build_bezier_spline_garden_graph() -> str:
    asset_name = "PCG_BezierSplineGarden"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    spline_node, spline_settings = add_node(graph, "PCGBezierSplineSampleSettings", -500, -60)
    spline_settings.set_editor_property("spline_actor_tag", "Melodia.Portfolio.PathSpline")

    scatter_node, scatter_settings = add_node(graph, "PCGBezierOrnamentScatterSettings", -500, 120)
    apply_preset(scatter_settings, "GARDEN_PROMENADE")
    apply_portfolio_ornament_scatter(scatter_settings)

    path_spawner = add_spawner(graph, catalog, 100, -60, "PCGCol_Baroque_Walls", ["short"])
    orn_spawner = add_spawner(graph, catalog, 100, 120, "PCGCol_Baroque_Cornice")

    out = graph.get_output_node()
    graph.add_edge(spline_node, "Out", path_spawner, "In")
    graph.add_edge(scatter_node, "Out", orn_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, orn_spawner, "Out", out)
    attach_environment_scatter(graph, catalog, spline_node, "Out", out)

    save_graph(graph, asset_path)
    return asset_path


def build_portfolio_environment_graph() -> str:
    """Standalone terrain scatter graph for portfolio zones (rocks + ground cover)."""
    asset_name = "PCG_PortfolioEnvironment"
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    graph = create_or_load_graph(asset_name)
    clear_graph(graph)
    catalog = load_catalog()

    path_node, path_settings = add_node(graph, "PCGBezierPathSettings", -500, 0)
    apply_preset(path_settings, "GARDEN_PROMENADE")

    out = graph.get_output_node()
    attach_environment_scatter(
        graph,
        catalog,
        path_node,
        "Out",
        out,
        rock_count=160,
        ground_count=120,
    )

    save_graph(graph, asset_path)
    return asset_path


GRAPH_BUILDERS = [
    build_portfolio_terrace_bezier_graph,
    build_bezier_path_portfolio_graph,
    build_bezier_cloister_ring_graph,
    build_bezier_colonnade_avenue_graph,
    build_bezier_garden_promenade_graph,
    build_bezier_bridge_span_graph,
    build_bezier_cathedral_axis_graph,
    build_bezier_vista_terrace_graph,
    build_bezier_ornament_gallery_graph,
    build_bezier_spline_garden_graph,
    build_portfolio_environment_graph,
]


def build_all() -> list[str]:
    paths = []
    for builder in GRAPH_BUILDERS:
        try:
            path = builder()
            paths.append(path)
            unreal.log(f"Melodia Bezier builder OK: {path}")
        except Exception as exc:  # pragma: no cover
            unreal.log_error(f"Melodia Bezier builder failed {builder.__name__}: {exc}")
    unreal.log(f"Melodia Bezier PCG builder finished ({len(paths)} graphs). Place AMelodiaPCGLevelKit in levels.")
    return paths


if __name__ == "__main__":
    build_all()
