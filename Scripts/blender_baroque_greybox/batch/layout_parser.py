"""
Layout parser — parse layout description JSON into element list.

Supports two formats:
1. Simple: {"elements": [{"type": "...", "params": {...}, ...}]}
2. Grid: {"grid": {"rows": N, "cols": M, "spacing": [x,y], "pattern": [...]}}
"""

import math


def parse_layout(data):
    """
    Parse a layout description dict into a flat list of element dicts.

    Args:
        data: dict with either "elements" or "grid" key

    Returns:
        list of element dicts: {type, params, location, rotation, scale, seed}
    """
    if "elements" in data:
        return _parse_elements(data["elements"])
    elif "grid" in data:
        return _parse_grid(data["grid"])
    elif "ring" in data:
        return _parse_ring(data["ring"])
    else:
        # Try to interpret as a single element
        if "type" in data:
            return [data]
        return []


def _parse_elements(elements):
    """Parse a simple element list."""
    result = []
    for elem in elements:
        entry = {
            'type': elem.get('type', ''),
            'params': elem.get('params', {}),
            'location': tuple(elem.get('location', (0, 0, 0))),
            'rotation': tuple(elem.get('rotation', (0, 0, 0))),
            'scale': tuple(elem.get('scale', (1, 1, 1))),
        }
        if 'seed' in elem:
            entry['seed'] = elem['seed']
        result.append(entry)
    return result


def _parse_grid(grid):
    """
    Parse a grid layout into positioned elements.

    Grid format:
    {
        "rows": 3,
        "cols": 4,
        "spacing": [400, 400],  # cm between elements
        "origin": [0, 0, 0],
        "pattern": [
            {"type": "column_doric", "params": {...}},
            {"type": "column_ionic", "params": {...}},
            ...
        ]
    }
    """
    rows = grid.get('rows', 1)
    cols = grid.get('cols', 1)
    spacing = grid.get('spacing', [400, 400])
    origin = grid.get('origin', [0, 0, 0])
    pattern = grid.get('pattern', [])

    if not pattern:
        return []

    result = []
    idx = 0
    for r in range(rows):
        for c in range(cols):
            # Pick pattern element (cycling)
            pat_elem = pattern[idx % len(pattern)]

            x = origin[0] + c * spacing[0]
            y = origin[1] + r * spacing[1]
            z = origin[2] if len(origin) > 2 else 0

            entry = {
                'type': pat_elem.get('type', ''),
                'params': dict(pat_elem.get('params', {})),
                'location': (x, y, z),
                'rotation': tuple(pat_elem.get('rotation', (0, 0, 0))),
                'scale': tuple(pat_elem.get('scale', (1, 1, 1))),
            }
            result.append(entry)
            idx += 1

    return result


def _parse_ring(ring):
    """
    Parse a ring/circular layout.

    Ring format:
    {
        "radius": 600,
        "count": 8,
        "pattern": [{"type": "column_corinthian", "params": {...}}],
        "origin": [0, 0, 0]
    }
    """
    radius = ring.get('radius', 600)
    count = ring.get('count', 8)
    pattern = ring.get('pattern', [])
    origin = ring.get('origin', [0, 0, 0])

    if not pattern:
        return []

    result = []
    for i in range(count):
        angle = (i / count) * math.pi * 2
        x = origin[0] + radius * math.cos(angle)
        y = origin[1] + radius * math.sin(angle)
        z = origin[2] if len(origin) > 2 else 0

        pat_elem = pattern[i % len(pattern)]

        # Rotate element to face center
        rot_z = angle + math.pi / 2

        entry = {
            'type': pat_elem.get('type', ''),
            'params': dict(pat_elem.get('params', {})),
            'location': (x, y, z),
            'rotation': (0, 0, rot_z),
            'scale': tuple(pat_elem.get('scale', (1, 1, 1))),
        }
        result.append(entry)

    return result
