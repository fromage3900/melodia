"""
Vector math utilities for architectural geometry generation.

Provides interpolation, rotation, spline evaluation, and normal computation
helpers used throughout the addon. All coordinates in centimeters.
"""

import math
from mathutils import Vector, Matrix


# ---------------------------------------------------------------------------
# Scalar helpers
# ---------------------------------------------------------------------------

def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def lerp(a: float, b: float, t: float) -> float:
    """Linear interpolation from a to b by factor t."""
    return a + (b - a) * t


def smoothstep(edge0: float, edge1: float, x: float) -> float:
    """Hermite smoothstep interpolation."""
    t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def smootherstep(edge0: float, edge1: float, x: float) -> float:
    """Perlin's smootherstep."""
    t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


def map_range(value: float, from_min: float, from_max: float,
              to_min: float, to_max: float) -> float:
    """Remap value from one range to another."""
    if abs(from_max - from_min) < 1e-12:
        return to_min
    t = (value - from_min) / (from_max - from_min)
    return to_min + t * (to_max - to_min)


# ---------------------------------------------------------------------------
# 2D / 3D rotation
# ---------------------------------------------------------------------------

def rotate_2d(x: float, z: float, angle_rad: float) -> tuple:
    """Rotate a 2D point (x, z) around origin by angle_rad."""
    c = math.cos(angle_rad)
    s = math.sin(angle_rad)
    return (x * c - z * s, x * s + z * c)


def rotate_3d(vec: Vector, axis: Vector, angle_rad: float) -> Vector:
    """Rotate a 3D vector around an arbitrary axis."""
    mat = Matrix.Rotation(angle_rad, 3, axis)
    return mat @ vec


def rotate_around_point_2d(x: float, z: float,
                           cx: float, cz: float,
                           angle_rad: float) -> tuple:
    """Rotate (x, z) around center (cx, cz)."""
    dx, dz = x - cx, z - cz
    rx, rz = rotate_2d(dx, dz, angle_rad)
    return (rx + cx, rz + cz)


# ---------------------------------------------------------------------------
# Spline evaluation
# ---------------------------------------------------------------------------

def catmull_rom(p0: Vector, p1: Vector, p2: Vector, p3: Vector,
                t: float) -> Vector:
    """Evaluate Catmull-Rom spline at parameter t ∈ [0, 1]."""
    t2 = t * t
    t3 = t2 * t
    return 0.5 * (
        (2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
    )


def hermite(p0: Vector, m0: Vector, p1: Vector, m1: Vector,
            t: float) -> Vector:
    """Evaluate cubic Hermite spline at parameter t ∈ [0, 1]."""
    t2 = t * t
    t3 = t2 * t
    h00 = 2.0 * t3 - 3.0 * t2 + 1.0
    h10 = t3 - 2.0 * t2 + t
    h01 = -2.0 * t3 + 3.0 * t2
    h11 = t3 - t2
    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1


def quadratic_bezier(p0: Vector, p1: Vector, p2: Vector,
                     t: float) -> Vector:
    """Evaluate quadratic bezier at parameter t."""
    u = 1.0 - t
    return u * u * p0 + 2.0 * u * t * p1 + t * t * p2


# ---------------------------------------------------------------------------
# Normal / tangent computation
# ---------------------------------------------------------------------------

def compute_normal(v0: Vector, v1: Vector, v2: Vector) -> Vector:
    """Compute face normal from three vertices (counter-clockwise winding)."""
    e1 = v1 - v0
    e2 = v2 - v0
    n = e1.cross(e2)
    if n.length > 1e-12:
        n.normalize()
    return n


def compute_tangent_from_normals(n0: Vector, n1: Vector) -> Vector:
    """Approximate tangent direction from two adjacent normals."""
    t = n1 - n0
    if t.length > 1e-12:
        t.normalize()
    return t


# ---------------------------------------------------------------------------
# Polygon / shape helpers
# ---------------------------------------------------------------------------

def circle_points(radius: float, segments: int,
                  center: Vector = None) -> list:
    """Generate points on a circle in the XZ plane (Y up)."""
    if center is None:
        center = Vector((0, 0, 0))
    pts = []
    for i in range(segments):
        angle = (i / segments) * math.pi * 2.0
        x = center.x + radius * math.cos(angle)
        z = center.z + radius * math.sin(angle)
        pts.append(Vector((x, center.y, z)))
    return pts


def arc_points(radius: float, start_angle: float, end_angle: float,
               segments: int, center: Vector = None) -> list:
    """Generate points on an arc in the XZ plane."""
    if center is None:
        center = Vector((0, 0, 0))
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = lerp(start_angle, end_angle, t)
        x = center.x + radius * math.cos(angle)
        z = center.z + radius * math.sin(angle)
        pts.append(Vector((x, center.y, z)))
    return pts


def rectangle_points(width: float, depth: float,
                     center: Vector = None) -> list:
    """Generate 4 corner points of a rectangle in the XZ plane."""
    if center is None:
        center = Vector((0, 0, 0))
    hw, hd = width * 0.5, depth * 0.5
    return [
        Vector((center.x - hw, center.y, center.z - hd)),
        Vector((center.x + hw, center.y, center.z - hd)),
        Vector((center.x + hw, center.y, center.z + hd)),
        Vector((center.x - hw, center.y, center.z + hd)),
    ]


def distribute_along_path(points: list, count: int) -> list:
    """Distribute `count` indices evenly along a list of path points."""
    n = len(points)
    if n == 0 or count == 0:
        return []
    indices = []
    for i in range(count):
        t = i / max(count - 1, 1)
        idx = int(t * (n - 1))
        indices.append(idx)
    return indices
