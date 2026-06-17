"""
Bezier curve utilities for Melodia Art Nouveau Greybox.

CubicBezier class + BezierChain for multi-segment C1-continuous curves.
"""

import math
from mathutils import Vector


class CubicBezier:
    """Cubic Bezier curve defined by 4 control points."""

    def __init__(self, p0: Vector, p1: Vector, p2: Vector, p3: Vector):
        self.p0 = p0
        self.p1 = p1
        self.p2 = p2
        self.p3 = p3

    def evaluate(self, t: float) -> Vector:
        u = 1.0 - t
        return (u ** 3 * self.p0 +
                3 * u ** 2 * t * self.p1 +
                3 * u * t ** 2 * self.p2 +
                t ** 3 * self.p3)

    def tangent(self, t: float) -> Vector:
        u = 1.0 - t
        return (3 * u ** 2 * (self.p1 - self.p0) +
                6 * u * t * (self.p2 - self.p1) +
                3 * t ** 2 * (self.p3 - self.p2)).normalized()

    def split(self, t: float) -> tuple['CubicBezier', 'CubicBezier']:
        p01 = self.p0.lerp(self.p1, t)
        p12 = self.p1.lerp(self.p2, t)
        p23 = self.p2.lerp(self.p3, t)
        p012 = p01.lerp(p12, t)
        p123 = p12.lerp(p23, t)
        pt = p012.lerp(p123, t)
        return CubicBezier(self.p0, p01, p012, pt), CubicBezier(pt, p123, p23, self.p3)

    def reverse(self) -> 'CubicBezier':
        return CubicBezier(self.p3, self.p2, self.p1, self.p0)

    @classmethod
    def from_hermite(cls, p1: Vector, t1: Vector, p2: Vector, t2: Vector) -> 'CubicBezier':
        p0 = p1
        p3 = p2
        p1 = p1 + t1 / 3.0
        p2 = p2 - t2 / 3.0
        return cls(p0, p1, p2, p3)

    @classmethod
    def fit_from_points(cls, points: list[Vector]) -> 'CubicBezier':
        if len(points) < 4:
            while len(points) < 4:
                points.append(points[-1] if points else Vector())
        return cls(points[0], points[1], points[2], points[3])

    def sample(self, n: int) -> list[Vector]:
        return [self.evaluate(i / max(n - 1, 1)) for i in range(n)]

    def arc_length(self, samples: int = 50) -> float:
        pts = self.sample(samples)
        return sum((pts[i + 1] - pts[i]).length for i in range(len(pts) - 1))


class BezierChain:
    """Connected sequence of CubicBezier segments with C1 continuity."""

    def __init__(self, segments: list[CubicBezier] = None):
        self.segments: list[CubicBezier] = segments or []

    def add_segment(self, seg: CubicBezier):
        self.segments.append(seg)

    def evaluate(self, t: float) -> Vector:
        t = max(0.0, min(1.0, t))
        n = len(self.segments)
        if n == 0:
            return Vector()
        idx = min(int(t * n), n - 1)
        local_t = (t * n) - idx
        return self.segments[idx].evaluate(local_t)

    def tangent(self, t: float) -> Vector:
        t = max(0.0, min(1.0, t))
        n = len(self.segments)
        if n == 0:
            return Vector((0, 0, 1))
        idx = min(int(t * n), n - 1)
        local_t = (t * n) - idx
        return self.segments[idx].tangent(local_t)

    def sample(self, n: int) -> list[Vector]:
        return [self.evaluate(i / max(n - 1, 1)) for i in range(n)]

    def arc_length(self, samples_per_seg: int = 20) -> float:
        return sum(seg.arc_length(samples_per_seg) for seg in self.segments)

    def split_at(self, t: float) -> tuple['BezierChain', 'BezierChain']:
        n = len(self.segments)
        idx = min(int(t * n), n - 1)
        local_t = (t * n) - idx
        left_seg, right_seg = self.segments[idx].split(local_t)
        left = BezierChain(self.segments[:idx] + [left_seg])
        right = BezierChain([right_seg] + self.segments[idx + 1:])
        return left, right
