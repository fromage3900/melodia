"""Preset management: save, load, apply, list builtin presets."""

import json
import os


def get_preset_dir() -> str:
    return os.path.join(os.path.dirname(os.path.dirname(__file__)), 'presets', 'defaults')


def list_builtin_presets() -> list[str]:
    preset_dir = get_preset_dir()
    if not os.path.exists(preset_dir):
        return []
    return [f.replace('.json', '') for f in os.listdir(preset_dir) if f.endswith('.json')]


def save_preset(name: str, data: dict, custom_dir: str = None):
    d = custom_dir or get_preset_dir()
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, f'{name}.json'), 'w') as f:
        json.dump(data, f, indent=2)


def load_preset(name: str) -> dict:
    preset_dir = get_preset_dir()
    path = os.path.join(preset_dir, f'{name}.json')
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return json.load(f)


def apply_preset(name: str, generator_instances: dict):
    data = load_preset(name)
    if not data:
        return
    for gen_id, params in data.get('generators', {}).items():
        if gen_id in generator_instances:
            generator_instances[gen_id].update(params)


def get_default_preset_data() -> dict:
    return {"generators": {}, "metadata": {"version": "1.0"}}
