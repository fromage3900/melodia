"""One-shot portfolio rebuild: DreamWalls v2 + Bezier placeholders + optional hero kit."""
from __future__ import annotations

import importlib.util
import json
import os

try:
    import unreal
except ImportError as exc:
    raise RuntimeError("Run inside Unreal Editor") from exc

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
MANIFEST_PATH = os.path.join(SCRIPTS, "melodia_pcg_library_manifest.json")


def _load(name: str, filename: str):
    spec = importlib.util.spec_from_file_location(name, os.path.join(SCRIPTS, filename))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _load_manifest() -> dict:
    with open(MANIFEST_PATH, encoding="utf-8") as fh:
        return json.load(fh)


def _log_manifest_summary(manifest: dict) -> None:
    ready = [g for g in manifest.get("graphs", []) if g.get("status") == "portfolio-ready"]
    unreal.log(f"Melodia PCG library manifest v{manifest.get('version')} — {len(ready)} portfolio-ready graphs")
    for entry in ready:
        role = entry.get("aesthetic_role", "?")
        ism = entry.get("expected_ism", "?")
        unreal.log(f"  [{role}] {entry.get('id')} ism={ism}")


def rebuild_portfolio(*, setup_hero: bool = True) -> None:
    manifest = _load_manifest()
    _log_manifest_summary(manifest)

    dream = _load("melodia_dreamwalls_builder", "melodia_dreamwalls_builder.py")
    bez = _load("melodia_pcg_bezier_builder", "melodia_pcg_bezier_builder.py")
    dream.build_dream_walls()
    paths = bez.build_all()

    unreal.log(f"Melodia portfolio rebuild: DreamWalls + {len(paths)} Bezier graphs.")

    if setup_hero:
        hero_path = os.path.join(SCRIPTS, "_setup_portfolio_hero.py")
        if os.path.isfile(hero_path):
            spec = importlib.util.spec_from_file_location("hero", hero_path)
            hero = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(hero)
            if hasattr(hero, "main"):
                hero.main()

    unreal.log("Melodia portfolio placeholder rebuild complete. See melodia_pcg_library_manifest.json")


if __name__ == "__main__":
    rebuild_portfolio()
