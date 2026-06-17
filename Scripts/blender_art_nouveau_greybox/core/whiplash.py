"""
Whiplash curve generator — the signature Art Nouveau S-curve motif.

Compound S-curves with inflection points, implemented as chains of
2-4 cubic bezier segments with alternating curvature direction.
"""

import math
from mathutils import Vector
from .bezier import CubicBezier, BezierChain


class WhiplashCurve:
    """Generates compound S-curves with configurable inflection points."""

    @staticmethod
    def generate(length: float, amplitude: float, phases: int = 2,
                 asymmetry: float = 0.0, segments: int = 32,
                 rng=None) -> list[Vector]:
        """
        Generate a 2D whiplash curve in the XZ plane.

        Args:
            length: Total curve length along X axis
            amplitude: Peak deviation from center line
            phases: Number of S-curve inflection cycles (2 = one full S)
            asymmetry: 0-1, random deviation amount
            segments: Number of sample points
            rng: Seeded random instance
        """
        points = []
        for i in range(segments):
            t = i / max(segments - 1, 1)
            x = t * length
            base_z = amplitude * math.sin(phases * math.pi * t)
            # Asymmetric deviation
            if asymmetry > 0 and rng:
                base_z += rng.uniform(-asymmetry * amplitude, asymmetry * amplitude)
            points.append(Vector((x, 0.0, base_z)))
        return points

    @staticmethod
    def generate_3d(length: float, amplitude_xy: float, amplitude_z: float,
                    phases: int = 2, twist: float = 0.0,
                    segments: int = 32, rng=None) -> list[Vector]:
        """Generate a 3D whiplash curve with twist."""
        points = []
        for i in range(segments):
            t = i / max(segments - 1, 1)
            x = t * length
            phase = phases * math.pi * t
            y = amplitude_xy * math.sin(phase) * math.cos(twist * t)
            z = amplitude_z * math.sin(phase) * math.sin(twist * t)
            points.append(Vector((x, y, z)))
        return points

    @staticmethod
    def as_bezier_chain(length: float, amplitude: float, phases: int = 2,
                        asymmetry: float = 0.0, rng=None) -> BezierChain:
        """Generate a whiplash curve as a BezierChain for smooth sweeps."""
        segments_per_phase = max(phases, 2)
        beziers = []
        for i in range(segments_per_phase):
            t_start = i / segments_per_phase
            t_end = (i + 1) / segments_per_phase
            x0 = t_start * length
            x1 = (t_start + 0.33) * length
            x2 = (t_start + 0.66) * length
            x3 = t_end * length
            phase = phases * math.pi
            z0 = amplitude * math.sin(phase * t_start)
            z3 = amplitude * math.sin(phase * t_end)
            # Control points for smooth inflection
            z1 = z0 + amplitude * 0.5 * (1 if i % 2 == 0 else -1)
            z2 = z3 - amplitude * 0.5 * (1 if i % 2 == 0 else -1)
            if asymmetry > 0 and rng:
                z1 += rng.uniform(-asymmetry * amplitude, asymmetry * amplitude)
                z2 += rng.uniform(-asymmetry * amplitude, asymmetry * amplitude)
            beziers.append(CubicBezier(
                Vector((x0, 0, z0)),
                Vector((x1, 0, z1)),
                Vector((x2, 0, z2)),
                Vector((x3, 0, z3)),
            ))
        return BezierChain(beziers)

    @staticmethod
    def relief_pattern(width: float, height: float, depth: float,
                       wave_count: int = 3, asymmetry: float = 0.0,
                       rng=None) -> list[tuple[float, float, float]]:
        """
        Generate a whiplash relief displacement field.
        Returns list of (x, y, z_offset) for surface displacement.
        """
        displacements = []
        steps_x = max(int(width / 10), 8)
        steps_y = max(int(height / 10), 8)
        for i in range(steps_x + 1):
            for j in range(steps_y + 1):
                x = (i / steps_x) * width - width / 2
                y = (j / steps_y) * height - height / 2
                t_x = i / steps_x
                z = depth * math.sin(wave_count * math.pi * t_x)
                if asymmetry > 0 and rng:
                    z += rng.uniform(-asymmetry * depth, asymmetry * depth)
                displacements.append((x, y, z))
        return displacements


def whiplash_2d(length: float, amplitude: float, asymmetry: float = 0.0,
                segments: int = 32, rng=None) -> list[tuple[float, float]]:
    """Convenience: return 2D whiplash as (x, z) tuples for profile use."""
    points = WhiplashCurve.generate(length, amplitude, phases=2,
                                     asymmetry=asymmetry, segments=segments, rng=rng)
    return [(p.x, p.z) for p in points]


def whiplash_relief(depth: float, width: float, height: float,
                    asymmetry: float = 0.0, rng=None) -> list[tuple[float, float, float]]:
    """Convenience: return 3D relief displacement field."""
    return WhiplashCurve.relief_pattern(width, height, depth,
                                         wave_count=3, asymmetry=asymmetry, rng=rng)
