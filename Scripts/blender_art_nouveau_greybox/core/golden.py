"""Golden ratio utilities for Art Nouveau proportions."""

import math
from mathutils import Vector

PHI = (1 + math.sqrt(5)) / 2


def golden_subdivide(total: float) -> tuple[float, float]:
    a = total / PHI
    return a, total - a


def golden_spiral_points(count: int, scale: float = 1.0) -> list[Vector]:
    return [Vector((math.cos(i * PHI) * scale * math.sqrt(i),
                     math.sin(i * PHI) * scale * math.sqrt(i),
                     0.0)) for i in range(count)]


def fibonacci_sphere(radius: float, count: int) -> list[Vector]:
    pts = []
    for i in range(count):
        theta = math.acos(1 - 2 * (i + 0.5) / count)
        phi = 2 * math.pi * i / PHI
        pts.append(Vector((radius * math.sin(theta) * math.cos(phi),
                           radius * math.sin(theta) * math.sin(phi),
                           radius * math.cos(theta))))
    return pts


def golden_section(a: float, b: float) -> float:
    return a + (b - a) / PHI


def proportional_scale(base: float, ratio: float = PHI) -> float:
    return base * ratio
