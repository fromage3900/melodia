"""
Cubic bezier curve class with evaluation, tangent, sampling, and arc length.
"""

import math
from mathutils import Vector
from .math_utils import lerp


class CubicBezier:
    """Cubic bezier curve defined by 4 control points: P0, P1, P2, P3."""

    __slots__ = ('p0', 'p1', 'p2', 'p3')

    def __init__(self, p0: Vector, p1: Vector, p2: Vector, p3: Vector):
        self.p0 = Vector(p0)
        self.p1 = Vector(p1)
        self.p2 = Vector(p2)
        self.p3 = Vector(p3)

    def evaluate(self, t: float) -> Vector:
        """Evaluate curve position at parameter t ∈ [0, 1]."""
        u = 1.0 - t
        u2 = u * u
        u3 = u2 * u
        t2 = t * t
        t3 = t2 * t
        return (u3 * self.p0 +
                3.0 * u2 * t * self.p1 +
                3.0 * u * t2 * self.p2 +
                t3 * self.p3)

    def tangent(self, t: float) -> Vector:
        """Evaluate tangent vector (first derivative) at parameter t."""
        u = 1.0 - t
        return (3.0 * u * u * (self.p1 - self.p0) +
                6.0 * u * t * (self.p2 - self.p1) +
                3.0 * t * t * (self.p3 - self.p2))

    def tangent_normalized(self, t: float) -> Vector:
        """Evaluate normalized tangent at parameter t."""
        tan = self.tangent(t)
        if tan.length > 1e-12:
            tan.normalize()
        return tan

    def second_derivative(self, t: float) -> Vector:
        """Evaluate second derivative at parameter t."""
        u = 1.0 - t
        return (6.0 * u * (self.p2 - 2.0 * self.p1 + self.p0) +
                6.0 * t * (self.p3 - 2.0 * self.p2 + self.p1))

    def sample(self, n: int) -> list:
        """Sample n points along the curve (including endpoints)."""
        if n < 2:
            return [self.evaluate(0.5)]
        return [self.evaluate(i / (n - 1)) for i in range(n)]

    def sample_with_tangents(self, n: int) -> list:
        """Sample n (position, tangent) tuples along the curve."""
        if n < 2:
            return [(self.evaluate(0.5), self.tangent_normalized(0.5))]
        result = []
        for i in range(n):
            t = i / (n - 1)
            result.append((self.evaluate(t), self.tangent_normalized(t)))
        return result

    def arc_length(self, segments: int = 64) -> float:
        """Approximate arc length using line segments."""
        pts = self.sample(segments)
        length = 0.0
        for i in range(1, len(pts)):
            length += (pts[i] - pts[i - 1]).length
        return length

    def split(self, t: float) -> tuple:
        """De Casteljau split at parameter t. Returns two CubicBezier."""
        p01 = self.p0.lerp(self.p1, t)
        p12 = self.p1.lerp(self.p2, t)
        p23 = self.p2.lerp(self.p3, t)
        p012 = p01.lerp(p12, t)
        p123 = p12.lerp(p23, t)
        p0123 = p012.lerp(p123, t)
        left = CubicBezier(self.p0, p01, p012, p0123)
        right = CubicBezier(p0123, p123, p23, self.p3)
        return left, right

    def reverse(self) -> 'CubicBezier':
        """Return a new bezier with reversed direction."""
        return CubicBezier(self.p3, self.p2, self.p1, self.p0)

    @staticmethod
    def from_hermite(p0: Vector, m0: Vector, p1: Vector, m1: Vector,
                     scale: float = 1.0 / 3.0) -> 'CubicBezier':
        """Construct cubic bezier from Hermite form (points + tangents)."""
        return CubicBezier(
            p0,
            p0 + m0 * scale,
            p1 - m1 * scale,
            p1,
        )

    @staticmethod
    def fit_from_points(points: list) -> list:
        """
        Fit a sequence of cubic beziers through a list of points.
        Returns a list of CubicBezier segments.
        Uses Catmull-Rom style tangent estimation.
        """
        n = len(points)
        if n < 2:
            return []
        if n == 2:
            mid = (points[0] + points[1]) * 0.5
            return [CubicBezier(points[0], mid, mid, points[1])]

        segments = []
        for i in range(n - 1):
            p0 = points[i]
            p3 = points[i + 1]

            # Estimate tangents
            if i == 0:
                m0 = (points[1] - points[0])
            else:
                m0 = (points[i + 1] - points[i - 1]) * 0.5

            if i + 1 == n - 1:
                m1 = (points[n - 1] - points[n - 2])
            else:
                m1 = (points[i + 2] - points[i]) * 0.5

            seg = CubicBezier.from_hermite(p0, m0, p3, m1)
            segments.append(seg)

        return segments

    def __repr__(self):
        return (f"CubicBezier(p0={tuple(self.p0)}, p1={tuple(self.p1)}, "
                f"p2={tuple(self.p2)}, p3={tuple(self.p3)})")
