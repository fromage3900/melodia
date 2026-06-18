"""
Bootstrap PCGEx mesh collection assets from baroque_mesh_catalog.json.

Run inside Unreal Editor after PCGExtendedToolkit is enabled:

    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "melodia_pcgex_collections",
        r"G:/Melodia/Scripts/PCG/melodia_pcgex_collections.py")
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
    raise RuntimeError("melodia_pcgex_collections must run inside Unreal Editor") from exc


PROJECT_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
)
CATALOG_PATH = os.path.join(PROJECT_ROOT, "Scripts", "PCG", "baroque_mesh_catalog.json")
COLLECTION_PACKAGE = "/Game/_PROJECT/PCG/Collections"


def load_catalog() -> dict:
    with open(CATALOG_PATH, encoding="utf-8") as fh:
        return json.load(fh)


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def pcgex_available() -> bool:
    return hasattr(unreal, "PCGExMeshCollection") and hasattr(unreal, "PCGExMeshCollectionEntry")


def make_entry(mesh_path: str, weight: int, tags: list[str] | None = None):
    entry = unreal.PCGExMeshCollectionEntry()
    mesh = unreal.load_asset(mesh_path)
    if mesh is None:
        unreal.log_warning(f"Mesh missing for collection entry: {mesh_path}")
        return None
    entry.set_editor_property("static_mesh", mesh)
    entry.set_editor_property("weight", int(weight))
    if tags:
        try:
            entry.set_editor_property("tags", [unreal.Name(t) for t in tags])
        except Exception:  # noqa: BLE001
            pass
    return entry


def build_collection(collection_key: str, coll_data: dict) -> str | None:
    if not pcgex_available():
        return None

    ensure_directory(COLLECTION_PACKAGE)
    asset_name = collection_key
    asset_path = f"{COLLECTION_PACKAGE}/{asset_name}"

    coll_cls = unreal.PCGExMeshCollection
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        collection = unreal.EditorAssetLibrary.load_asset(asset_path)
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        collection = asset_tools.create_asset(asset_name, COLLECTION_PACKAGE, coll_cls, None)

    entries = []
    for item in coll_data.get("entries", []):
        entry = make_entry(
            item["mesh"],
            item.get("weight", 1),
            item.get("tags"),
        )
        if entry:
            entries.append(entry)

    if not entries:
        unreal.log_warning(f"No valid entries for {collection_key}")
        return None

    collection.set_editor_property("entries", entries)
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    pkg = unreal.find_package(asset_path)
    if pkg:
        unreal.EditorLoadingAndSavingUtils.reload_packages([pkg])
    unreal.log(f"PCGEx collection OK: {asset_path} ({len(entries)} entries)")
    return asset_path


def build_all() -> list[str]:
    if not pcgex_available():
        unreal.log_error(
            "PCGExMeshCollection not found. Enable PCGExtendedToolkit in Melodia.uproject "
            "and restart the editor, then rebuild."
        )
        return []

    catalog = load_catalog()
    paths: list[str] = []
    for key, data in catalog.get("collections", {}).items():
        if not isinstance(data, dict) or "entries" not in data:
            continue
        path = build_collection(key, data)
        if path:
            paths.append(path)

    unreal.log(f"melodia_pcgex_collections: created/updated {len(paths)} collections.")
    return paths


if __name__ == "__main__":
    build_all()
