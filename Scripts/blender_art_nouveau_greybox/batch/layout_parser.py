"""Layout parser: elements, grid, ring/circular formats."""

import math


def parse_layout(data: dict) -> dict:
    layout_type = data.get('type', 'elements')
    if layout_type == 'elements':
        return _parse_elements(data)
    elif layout_type == 'grid':
        return _parse_grid(data)
    elif layout_type == 'ring':
        return _parse_ring(data)
    return _parse_elements(data)


def _parse_elements(data: dict) -> dict:
    return {'elements': data.get('elements', [])}


def _parse_grid(data: dict) -> dict:
    rows = data.get('rows', 1)
    cols = data.get('cols', 1)
    spacing_x = data.get('spacing_x', 400)
    spacing_y = data.get('spacing_y', 400)
    elements = data.get('elements', [])
    result = []
    for r in range(rows):
        for c in range(cols):
            elem = elements[(r * cols + c) % len(elements)] if elements else {}
            result.append({
                **elem,
                'location': (c * spacing_x, r * spacing_y, elem.get('location', (0, 0, 0))[2] if isinstance(elem.get('location'), tuple) else 0),
            })
    return {'elements': result}


def _parse_ring(data: dict) -> dict:
    count = data.get('count', len(data.get('elements', [])))
    radius = data.get('radius', 400)
    elements = data.get('elements', [])
    result = []
    for i in range(count):
        angle = (i / count) * math.pi * 2
        elem = elements[i % len(elements)] if elements else {}
        x = math.cos(angle) * radius
        y = math.sin(angle) * radius
        result.append({
            **elem,
            'location': (x, y, 0),
            'rotation': (0, 0, -angle),
        })
    return {'elements': result}
