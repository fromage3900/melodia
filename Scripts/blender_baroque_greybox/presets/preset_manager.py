"""
Preset manager — JSON preset save/load/apply for generator parameters.
"""

import json
import os


def save_preset(filepath, data):
    """
    Save a preset dictionary to a JSON file.

    Args:
        filepath: output file path
        data: dict of preset data (mode, generators params)
    """
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def load_preset(filepath):
    """
    Load a preset from a JSON file.

    Args:
        filepath: path to JSON preset file

    Returns:
        dict of preset data
    """
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)


def apply_preset(scene, data):
    """
    Apply a loaded preset to the scene.

    Args:
        scene: bpy.types.Scene
        data: dict from load_preset()
    """
    # Apply mode
    if "mode" in data:
        scene.melodia_props.geometry_mode = data["mode"]

    # Apply generator params
    generators = data.get("generators", {})
    for gen_id, params in generators.items():
        attr_name = f"melodia_{gen_id}"
        storage = getattr(scene, attr_name, None)
        if storage is None:
            continue
        for prop_name, value in params.items():
            if hasattr(storage, prop_name):
                try:
                    setattr(storage, prop_name, value)
                except (TypeError, AttributeError):
                    pass


def list_builtin_presets():
    """
    List available built-in presets.

    Returns:
        list of (name, filepath) tuples
    """
    preset_dir = os.path.join(os.path.dirname(__file__), 'defaults')
    presets = []
    if os.path.isdir(preset_dir):
        for f in sorted(os.listdir(preset_dir)):
            if f.endswith('.json'):
                name = f.replace('.json', '')
                presets.append((name, os.path.join(preset_dir, f)))
    return presets


def get_default_preset_data(name):
    """
    Get default preset data by name.

    Args:
        name: preset name (without .json)

    Returns:
        dict or None
    """
    preset_dir = os.path.join(os.path.dirname(__file__), 'defaults')
    filepath = os.path.join(preset_dir, f"{name}.json")
    if os.path.exists(filepath):
        return load_preset(filepath)
    return None
