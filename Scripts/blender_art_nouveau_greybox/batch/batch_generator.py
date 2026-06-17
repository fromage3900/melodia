"""Batch generation: run multiple generators from layout data."""

import json


def run_batch(layout_data: dict, mode, seed: int, collection: str = ''):
    """Run batch generation from parsed layout data."""
    from ..generators.base_generator import get_generator
    from ..core.seed_manager import create_rng
    from ..core.geometry_modes import GeometryMode

    results = []
    for elem in layout_data.get('elements', []):
        gen_id = elem.get('generator')
        if not gen_id:
            continue
        gen_cls = get_generator(gen_id)
        if not gen_cls:
            continue
        rng = create_rng(seed + len(results))
        ctx = type('GeneratorContext', (), {'mode': mode, 'seed': seed, 'rng': rng, 'collection': collection})()
        params = elem.get('params', {})
        generator = gen_cls()
        obj = generator.preview(ctx, params)
        # Apply transform
        loc = elem.get('location', (0, 0, 0))
        rot = elem.get('rotation', (0, 0, 0))
        if obj:
            obj.location = loc
            obj.rotation_euler = rot
        results.append(obj)
    return results


def run_batch_from_json(json_string: str, mode, seed: int, collection: str = ''):
    """Run batch from JSON string."""
    data = json.loads(json_string)
    from ..batch.layout_parser import parse_layout
    layout = parse_layout(data)
    return run_batch(layout, mode, seed, collection)
