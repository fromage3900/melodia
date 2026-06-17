"""
Math utilities for Melodia Art Nouveau Greybox.

Includes standard helpers plus Art Nouveau-specific functions:
asymmetric offset, spiral points, plant growth curves.
"""

import math
from mathutils import Vector


# ---------------------------------------------------------------------------
# Basic math helpers
# ---------------------------------------------------------------------------

def clamp(val: float, min_val: float, max_val: float) -> float:
    return max(min_val, min(val, max_val))


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def smoothstep(edge0: float, edge1: float, x: float) -> float:
    t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def smootherstep(edge0: float, edge1: float, x: float) -> float:
    t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


def map_range(val: float, in_min: float, in_max: float, out_min: float, out_max: float) -> float:
    t = clamp((val - in_min) / (in_max - in_min), 0.0, 1.0)
    return lerp(out_min, out_max, t)


# ---------------------------------------------------------------------------
# Rotation helpers
# ---------------------------------------------------------------------------

def rotate_2d(x: float, y: float, angle: float) -> tuple[float, float]:
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    return x * cos_a - y * sin_a, x * sin_a + y * cos_a


def rotate_3d(vec: Vector, axis: Vector, angle: float) -> Vector:
    from mathutils import Matrix
    return Matrix.Rotation(angle, 3, axis) @ vec


# ---------------------------------------------------------------------------
# Curve helpers
# ---------------------------------------------------------------------------

def catmull_rom(p0: Vector, p1: Vector, p2: Vector, p3: Vector, t: float) -> Vector:
    t2 = t * t
    t3 = t2 * t
    return (
        0.5 * ((2.0 * p1)
               + (-p0 + p2) * t
               + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
               + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3)
    )


def hermite(p1: Vector, t1: Vector, p2: Vector, t2: Vector, t: float) -> Vector:
    t2 = t * t
    t3 = t2 * t
    h00 = 2.0 * t3 - 3.0 * t2 + 1.0
    h10 = t3 - 2.0 * t2 + t
    h01 = -2.0 * t3 + 3.0 * t2
    h11 = t3 - t2
    return h00 * p1 + h10 * t1 + h01 * p2 + h11 * t2


def quadratic_bezier(p0: Vector, p1: Vector, p2: Vector, t: float) -> Vector:
    u = 1.0 - t
    return u * u * p0 + 2.0 * u * t * p1 + t * t * p2


# ---------------------------------------------------------------------------
# Geometry helpers
# ---------------------------------------------------------------------------

def compute_normal(a: Vector, b: Vector, c: Vector) -> Vector:
    return (b - a).cross(c - a).normalized()


def circle_points(radius: float, segments: int) -> list[Vector]:
    angle_step = (2.0 * math.pi) / segments
    return [Vector((math.cos(i * angle_step) * radius, math.sin(i * angle_step) * radius, 0.0)) for i in range(segments)]


def arc_points(radius: float, segments: int, start_angle: float = 0.0, sweep: float = math.pi) -> list[Vector]:
    angle_step = sweep / segments
    return [Vector((math.cos(start_angle + i * angle_step) * radius, math.sin(start_angle + i * angle_step) * radius, 0.0)) for i in range(segments + 1)]


def rectangle_points(width: float, height: float) -> list[Vector]:
    hw, hh = width / 2, height / 2
    return [Vector((-hw, -hh, 0)), Vector((hw, -hh, 0)), Vector((hw, hh, 0)), Vector((-hw, hh, 0))]


# ---------------------------------------------------------------------------
# Path distribution
# ---------------------------------------------------------------------------

def distribute_along_path(path: list[Vector], count: int) -> list[tuple[Vector, Vector]]:
    """Return (position, direction) pairs evenly spaced along path."""
    if len(path) < 2:
        return [(path[0], Vector((0, 0, 1)))] if path else []
    total_len = sum((path[i + 1] - path[i]).length for i in range(len(path) - 1))
    if total_len == 0:
        return [(path[0], Vector((0, 0, 1)))]
    spacing = total_len / (count - 1) if count > 1 else 0
    result = []
    accumulated = 0
    seg_idx = 0
    seg_remaining = (path[1] - path[0]).length
    for i in range(count):
        target = i * spacing
        while seg_remaining < target - accumulated + 1e-6 and seg_idx < len(path) - 2:
            accumulated += seg_remaining
            seg_idx += 1
            seg_remaining = (path[seg_idx + 1] - path[seg_idx]).length
        t = (target - accumulated) / seg_remaining if seg_remaining > 0 else 0
        pos = path[seg_idx].lerp(path[seg_idx + 1], clamp(t, 0, 1))
        direction = (path[seg_idx + 1] - path[seg_idx]).normalized()
        result.append((pos, direction))
    return result


# ---------------------------------------------------------------------------
# Art Nouveau-specific functions
# ---------------------------------------------------------------------------

def asymmetric_offset(rng, amount: float) -> float:
    """Return a random deviation within [-amount, amount]."""
    return rng.uniform(-amount, amount)


def spiral_point(radius: float, angle: float, growth_rate: float) -> Vector:
    """Point on a logarithmic spiral."""
    r = radius * math.exp(growth_rate * angle)
    return Vector((math.cos(angle) * r, math.sin(angle) * r, 0.0))


def plant_growth_curve(t: float, vigor: float = 1.0) -> float:
    """Sigmoid growth modifier for organic scaling."""
    return 1.0 / (1.0 + math.exp(-vigor * (t - 0.5) * 10))
