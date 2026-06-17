"""
Batch generator — multi-generator orchestration.

Parses a layout description and runs multiple generators in sequence,
applying transforms (location, rotation, scale) to each.
"""

import bpy
from ..generators.base_generator import get_generator, GeneratorContext
from ..core.geometry_modes import GeometryMode
from ..core.seed_manager import create_rng
from .layout_parser import parse_layout


def run_batch(layout_data, mode=GeometryMode.VALID, seed=42, collection=None):
    """
    Run a batch generation from parsed layout data.

    Args:
        layout_data: dict from parse_layout() or raw JSON dict
        mode: GeometryMode
        seed: base seed (incremented per element)
        collection: target Blender collection

    Returns:
        list of generated bpy.types.Object
    """
    if isinstance(layout_data, dict):
        elements = parse_layout(layout_data)
    else:
        elements = layout_data

    if collection is None:
        collection = bpy.context.scene.collection

    rng = create_rng(seed)
    results = []

    for elem in elements:
        gen_id = elem.get('type', '')
        params = elem.get('params', {})
        location = elem.get('location', (0, 0, 0))
        rotation = elem.get('rotation', (0, 0, 0))
        scale = elem.get('scale', (1, 1, 1))
        elem_seed = elem.get('seed', seed + len(results))

        gen_cls = get_generator(gen_id)
        if gen_cls is None:
            print(f"[Melodia] Batch: generator '{gen_id}' not found, skipping")
            continue

        ctx = GeneratorContext(
            scene=bpy.context.scene,
            collection=collection,
            mode=mode,
            seed=elem_seed,
        )

        try:
            gen = gen_cls()
            obj = gen.generate(params, ctx)
            if obj:
                # Apply transform
                obj.location = location
                obj.rotation_euler = rotation
                obj.scale = scale
                results.append(obj)
        except Exception as e:
            print(f"[Melodia] Batch: failed to generate '{gen_id}': {e}")

    return results


def run_batch_from_json(json_string, mode=GeometryMode.VALID, seed=42,
                        collection=None):
    """
    Run batch generation from a JSON string.

    Args:
        json_string: JSON layout description
        mode: GeometryMode
        seed: base seed
        collection: target collection

    Returns:
        list of generated objects
    """
    import json
    data = json.loads(json_string)
    return run_batch(data, mode=mode, seed=seed, collection=collection)
