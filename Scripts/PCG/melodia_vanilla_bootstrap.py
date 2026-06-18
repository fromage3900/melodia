"""Apply volume-input bootstrap to legacy vanilla PCG graphs."""
from __future__ import annotations

import importlib.util
import os

import unreal

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location(
    "melodia_pcg_bezier_builder",
    os.path.join(SCRIPTS, "melodia_pcg_bezier_builder.py"),
)
bez = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bez)

VANILLA_GRAPHS = [
    "/Game/_PROJECT/PCG/Graphs/PCG_BaroqueColonnade",
    "/Game/_PROJECT/PCG/Graphs/PCG_BaroqueRuins",
    "/Game/_PROJECT/PCG/Graphs/PCG_Balustrade",
    "/Game/_PROJECT/PCG/Graphs/PCG_CathedralNave",
    "/Game/_PROJECT/PCG/Graphs/PCG_Cloister",
    "/Game/_PROJECT/PCG/Graphs/PCG_OvergrownRuins",
    "/Game/_PROJECT/PCG/Graphs/PCG_GreyboxBlockout",
    "/Game/_PROJECT/PCG/Graphs/PCG_PenroseShrine",
    "/Game/_PROJECT/PCG/Graphs/PCG_EscherDecks",
    "/Game/_PROJECT/PCG/Graphs/PCG_BridgeArchipelago",
    "/Game/_PROJECT/PCG/Graphs/PCG_FloatingStairways",
    "/Game/_PROJECT/PCG/Graphs/PCG_MelodiaForest",
    "/Game/_PROJECT/PCG/Graphs/PCG_MelodiaForest_Landscape",
    "/Game/_PROJECT/PCG/Graphs/PCG_MeadowFalloff",
    "/Game/_PROJECT/PCG/Graphs/PCG_ForestScatter_BS",
    "/Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden",
    "/Game/_PROJECT/PCG/Graphs/PCG_WallGardenPath",
    "/Game/_PROJECT/PCG/Graphs/PCG_SplinePath",
]

for path in VANILLA_GRAPHS:
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning(f"bootstrap skip missing {path}")
        continue
    graph = unreal.load_asset(path)
    bez.ensure_volume_input_bootstrap(graph)
    bez.save_graph(graph, path, bootstrap_volume=False)
    unreal.log(f"bootstrapped {path}")

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("Vanilla volume bootstrap complete")
