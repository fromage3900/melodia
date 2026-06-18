"""
Rebuild PCG_DreamWalls — clean greybox Escher wall field at 400 cm module grid.

Portfolio mode: single SM_wallhi placeholder, scale 1.0, three orthogonal layers
(0° / 90° / 180° with half-module offset). No random mesh variants.

Run in Unreal Editor:
    G:/Melodia/Scripts/PCG/melodia_dreamwalls_builder.py
"""

from __future__ import annotations

import importlib.util
import os

try:
    import unreal
except ImportError as exc:
    raise RuntimeError("melodia_dreamwalls_builder must run inside Unreal Editor") from exc

PROJECT_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
)
GRAPH_PACKAGE = "/Game/_PROJECT/PCG/Graphs"
ASSET_NAME = "PCG_DreamWalls"


def _load_helpers():
    spec = importlib.util.spec_from_file_location(
        "melodia_pcg_bezier_builder",
        os.path.join(PROJECT_ROOT, "Scripts", "PCG", "melodia_pcg_bezier_builder.py"),
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _load_standards():
    spec = importlib.util.spec_from_file_location(
        "melodia_pcg_standards",
        os.path.join(PROJECT_ROOT, "Scripts", "PCG", "melodia_pcg_standards.py"),
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _add_wall_layer(
    graph,
    helpers,
    standards,
    *,
    x: int,
    y: int,
    cell_cm: float,
    extent_cm: float,
    rot_yaw: float,
    translate_cm: tuple[float, float, float],
    out_node,
) -> None:
    grid_node, grid_settings = helpers.add_node(graph, "PCGCreatePointsGridSettings", x - 400, y)
    grid_settings.set_editor_property("cell_size", unreal.Vector(cell_cm, cell_cm, 1.0))
    grid_settings.set_editor_property("grid_extents", unreal.Vector(extent_cm, extent_cm, 0.0))

    xform_node, xform_settings = helpers.add_node(graph, "PCGTransformPointsSettings", x, y)
    standards.apply_uniform_transform(xform_settings, standards.DEFAULT_UNIFORM_SCALE, rot_yaw)
    tx, ty, tz = translate_cm
    if tx or ty or tz:
        offset = unreal.Vector(tx, ty, tz)
        try:
            xform_settings.set_editor_property("offset_min", offset)
            xform_settings.set_editor_property("offset_max", offset)
        except Exception:
            pass

    spawner_node, spawner_settings = helpers.add_node(
        graph, "PCGStaticMeshSpawnerSettings", x + 200, y
    )
    standards.configure_placeholder_spawner(spawner_settings, "wall")

    graph.add_edge(grid_node, "Out", xform_node, "In")
    graph.add_edge(xform_node, "Out", spawner_node, "In")
    helpers.wire_to_output(graph, spawner_node, "Out", out_node)


def build_dream_walls() -> str:
    helpers = _load_helpers()
    standards = _load_standards()
    module = standards.MODULE_CM
    extent = standards.DREAMWALLS_EXTENT_CM
    half = module * 0.5

    graph = helpers.create_or_load_graph(ASSET_NAME)
    helpers.clear_graph(graph)
    out_node = graph.get_output_node()

    layers = [
        # Primary orthogonal wall field — readable 400 cm modules
        dict(y=0, rot_yaw=0.0, translate=(0.0, 0.0, 0.0)),
        # Second plane at 90° — Escher overlap without scale chaos
        dict(y=220, rot_yaw=90.0, translate=(0.0, 0.0, 0.0)),
        # Third plane at 180° + half-module shift — stair/penrose motif read
        dict(y=440, rot_yaw=180.0, translate=(half, half, 0.0)),
    ]

    for layer in layers:
        _add_wall_layer(
            graph,
            helpers,
            standards,
            x=900,
            y=layer["y"],
            cell_cm=module,
            extent_cm=extent,
            rot_yaw=layer["rot_yaw"],
            translate_cm=layer["translate"],
            out_node=out_node,
        )

    asset_path = f"{GRAPH_PACKAGE}/{ASSET_NAME}"
    helpers.save_graph(graph, asset_path, bootstrap_volume=False)
    expected = standards.dreamwalls_expected_ism(extent_cm=extent, cell_cm=module, layers=len(layers))
    unreal.log(
        f"Melodia DreamWalls v2 OK: {asset_path} "
        f"(placeholder={standards.PLACEHOLDERS['wall']}, module={module}cm, "
        f"extent={extent}cm, layers={len(layers)}, expected_ism≈{expected})"
    )
    return asset_path


if __name__ == "__main__":
    build_dream_walls()
