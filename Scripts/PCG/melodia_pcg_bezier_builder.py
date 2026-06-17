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


def save_graph(graph, asset_path: str) -> None:
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    pkg = unreal.find_package(asset_path)
    if pkg:
        unreal.EditorLoadingAndSavingUtils.reload_packages([pkg])


def create_or_load_graph(asset_name: str) -> unreal.PCGGraph:
    asset_path = f"{GRAPH_PACKAGE}/{asset_name}"
    ensure_directory(GRAPH_PACKAGE)
    existing = unreal.load_asset(asset_path)
    if existing:
        return existing
    factory = unreal.PCGGraphFactory()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    return asset_tools.create_asset(asset_name, GRAPH_PACKAGE, unreal.PCGGraph, factory)


def clear_graph(graph) -> None:
    keep = {graph.get_input_node(), graph.get_output_node()}
    graph.remove_nodes([n for n in graph.get_nodes() if n not in keep])


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


def configure_spawner_meshes(spawner_settings, mesh_paths: list[str]) -> None:
    entries = []
    for path in mesh_paths:
        entry = mesh_entry(path)
        if entry:
            entries.append(entry)
    if not entries:
        entry = mesh_entry("/Game/_PROJECT/MelusinasHouse/SM_wallhi_mid")
        if entry:
            entries.append(entry)
    mesh_sel = unreal.PCGMeshSelectorWeighted()
    mesh_sel.set_editor_property("mesh_entries", entries)
    spawner_settings.set_editor_property("mesh_selector_type", unreal.PCGMeshSelectorWeighted)
    spawner_settings.set_editor_property("mesh_selector_parameters", mesh_sel)


def add_spawner(graph, catalog: dict, x: int, y: int, collection: str, tags: list[str] | None = None):
    node, settings = add_node(graph, "PCGStaticMeshSpawnerSettings", x, y)
    paths = _mesh_paths_from_collection(catalog, collection, tags)
    configure_spawner_meshes(settings, paths)
    return node


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
    terrace_settings.set_editor_property("b_emit_separate_pins", True)

    path_spawner = add_spawner(graph, catalog, 150, -220, "PCGCol_Baroque_Walls", ["short"])
    tile_spawner = add_spawner(graph, catalog, 150, 0, "PCGCol_Baroque_Cornice")
    rail_spawner = add_spawner(graph, catalog, 150, 220, "PCGCol_Baroque_Walls", ["short"])

    out = graph.get_output_node()
    graph.add_edge(terrace_node, "Out_Path", path_spawner, "In")
    graph.add_edge(terrace_node, "Out_Terrace", tile_spawner, "In")
    graph.add_edge(terrace_node, "Out_Railing", rail_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, tile_spawner, "Out", out)
    wire_to_output(graph, rail_spawner, "Out", out)

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
    cloister_settings.set_editor_property("b_emit_separate_pins", True)

    path_spawner = add_spawner(graph, catalog, 120, -200, "PCGCol_Baroque_Walls", ["short"])
    tile_spawner = add_spawner(graph, catalog, 120, 0, "PCGCol_Baroque_Cornice")
    col_spawner = add_spawner(graph, catalog, 120, 200, "PCGCol_Baroque_Columns")

    out = graph.get_output_node()
    graph.add_edge(cloister_node, "Out_Path", path_spawner, "In")
    graph.add_edge(cloister_node, "Out_Terrace", tile_spawner, "In")
    graph.add_edge(cloister_node, "Out_Column", col_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, tile_spawner, "Out", out)
    wire_to_output(graph, col_spawner, "Out", out)

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
    col_settings.set_editor_property("b_dual_row", True)

    path_spawner = add_spawner(graph, catalog, 100, -80, "PCGCol_Baroque_Walls", ["short"])
    col_spawner = add_spawner(graph, catalog, 100, 100, "PCGCol_Baroque_Columns")

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(col_node, "Out", col_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, col_spawner, "Out", out)

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
    sweep_settings.set_editor_property("sweep_role", unreal.EPCGArchitecturalRole.CORNICE)
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
    terrace_settings.set_editor_property("b_emit_separate_pins", True)

    path_spawner = add_spawner(graph, catalog, 120, -180, "PCGCol_Baroque_Walls", ["short"])
    tile_spawner = add_spawner(graph, catalog, 120, 0, "PCGCol_Baroque_Cornice")
    rail_spawner = add_spawner(graph, catalog, 120, 180, "PCGCol_Baroque_Walls", ["short"])

    out = graph.get_output_node()
    graph.add_edge(terrace_node, "Out_Path", path_spawner, "In")
    graph.add_edge(terrace_node, "Out_Terrace", tile_spawner, "In")
    graph.add_edge(terrace_node, "Out_Railing", rail_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, tile_spawner, "Out", out)
    wire_to_output(graph, rail_spawner, "Out", out)

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

    path_spawner = add_spawner(graph, catalog, 100, -80, "PCGCol_Baroque_Walls", ["short"])
    orn_spawner = add_spawner(graph, catalog, 100, 100, "PCGCol_Baroque_Cornice")

    out = graph.get_output_node()
    graph.add_edge(path_node, "Out", path_spawner, "In")
    graph.add_edge(scatter_node, "Out", orn_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, orn_spawner, "Out", out)

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

    path_spawner = add_spawner(graph, catalog, 100, -60, "PCGCol_Baroque_Walls", ["short"])
    orn_spawner = add_spawner(graph, catalog, 100, 120, "PCGCol_Baroque_Cornice")

    out = graph.get_output_node()
    graph.add_edge(spline_node, "Out", path_spawner, "In")
    graph.add_edge(scatter_node, "Out", orn_spawner, "In")
    wire_to_output(graph, path_spawner, "Out", out)
    wire_to_output(graph, orn_spawner, "Out", out)

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
