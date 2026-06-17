"""
Golden ratio utilities for baroque proportions.
"""

import math
from .constants import PHI, GOLDEN_RATIO


def golden_subdivide(width, height, depth=3):
    """
    Recursively subdivide a rectangle using the golden ratio.
    Returns list of (x, y, w, h) rectangles at the deepest level.
    """
    if depth <= 0:
        return [(0, 0, width, height)]

    rects = []
    if width >= height:
        # Split vertically
        split_x = width * GOLDEN_RATIO
        left = golden_subdivide(split_x, height, depth - 1)
        right = golden_subdivide(width - split_x, height, depth - 1)
        for (x, y, w, h) in left:
            rects.append((x, y, w, h))
        for (x, y, w, h) in right:
            rects.append((x + split_x, y, w, h))
    else:
        # Split horizontally
        split_y = height * GOLDEN_RATIO
        bottom = golden_subdivide(width, split_y, depth - 1)
        top = golden_subdivide(width, height - split_y, depth - 1)
        for (x, y, w, h) in bottom:
            rects.append((x, y, w, h))
        for (x, y, w, h) in top:
            rects.append((x, y + split_y, w, h))
    return rects


def golden_spiral_points(turns=3, scale=100.0, segments_per_turn=24):
    """
    Generate points along a golden spiral.
    The spiral grows by factor φ every quarter turn.
    """
    pts = []
    total_segments = int(turns * segments_per_turn)
    b = math.log(PHI) / (math.pi / 2.0)

    for i in range(total_segments + 1):
        theta = (i / segments_per_turn) * math.pi * 0.5
        r = scale * math.exp(b * theta)
        x = r * math.cos(theta)
        y = r * math.sin(theta)
        pts.append((x, y))
    return pts


def fibonacci_sphere(n=100):
    """
    Generate approximately uniformly distributed points on a unit sphere
    using the Fibonacci spiral method.
    Returns list of (x, y, z) tuples.
    """
    pts = []
    golden_angle = math.pi * (3.0 - math.sqrt(5.0))  # ~2.39996 rad

    for i in range(n):
        y = 1.0 - (2.0 * i / (n - 1)) if n > 1 else 0.0
        radius = math.sqrt(1.0 - y * y)
        theta = golden_angle * i
        x = radius * math.cos(theta)
        z = radius * math.sin(theta)
        pts.append((x, y, z))
    return pts


def golden_section(value, ratio=None):
    """Split a value by the golden ratio. Returns (larger, smaller)."""
    if ratio is None:
        ratio = GOLDEN_RATIO
    larger = value * ratio
    smaller = value - larger
    return (larger, smaller)


def proportional_scale(base, levels=3):
    """
    Generate a series of dimensions scaled by the golden ratio.
    Returns list from largest to smallest.
    """
    dims = [base]
    for _ in range(levels - 1):
        dims.append(dims[-1] * GOLDEN_RATIO)
    return dims
