"""Shared ornament utilities."""

import math
import bmesh
from mathutils import Vector
from ...core.mesh_builder import revolve_profile, sweep_along_curve
from ...core.profile_curves import petal_profile


def build_petal_ring(bm: bmesh.types.BMesh, radius: float, petal_count: int,
                     petal_length: float, petal_width: float, curl: float = 0.4,
                     z_offset: float = 0.0) -> bmesh.types.BMesh:
    """Ring of petals radiating outward."""
    for i in range(petal_count):
        angle = (i / petal_count) * math.pi * 2
        profile = petal_profile(petal_width, petal_length, curl, 12)
        # Rotate profile to petal position
        rotated = []
        for r, z in profile:
            x = r * math.cos(angle)
            y = r * math.sin(angle)
            rotated.append((x, y, z + z_offset))
        # Create petal as extruded profile
        from ...core.mesh_builder import extrude_profile_linear
        flat_profile = [(r, z) for r, z in zip([abs(p[0]) for p in rotated], [p[2] for p in rotated])]
        extrude_profile_linear(bm, flat_profile, petal_width * 0.3, 8)
    return bm


def build_spiral_tendril(bm: bmesh.types.BMesh, start_pos: Vector, direction: Vector,
                         length: float, radius_start: float, radius_end: float,
                         turns: float, rng=None) -> bmesh.types.BMesh:
    """Spiral tendril via sweep."""
    segments = int(turns * 24)
    path = []
    for i in range(segments + 1):
        t = i / segments
        angle = t * turns * math.pi * 2
        r = radius_start + (radius_end - radius_start) * t
        x = start_pos.x + math.cos(angle) * r
        y = start_pos.y + math.sin(angle) * r
        z = start_pos.z + t * length
        path.append(Vector((x, y, z)))
    from ...core.mesh_builder import sweep_along_curve
    cross = [(r * 0.3, r * 0.3) for r in [radius_start * 0.5, radius_start * 0.5, radius_start * 0.5, radius_start * 0.5]]
    sweep_along_curve(bm, [(0.3, 0.3), (-0.3, 0.3), (-0.3, -0.3), (0.3, -0.3)], path)
    return bm


def build_fan_motif(bm: bmesh.types.BMesh, radius: float, ray_count: int,
                    spread_angle: float, z_offset: float = 0.0) -> bmesh.types.BMesh:
    """Radiating fan/feather pattern."""
    from ...core.mesh_builder import make_box
    for i in range(ray_count):
        angle = -spread_angle / 2 + (i / max(ray_count - 1, 1)) * spread_angle
        x = radius * math.cos(angle)
        y = radius * math.sin(angle)
        make_box(bm, 3, radius * 0.8, 2, Vector((x * 0.5, y * 0.5, z_offset)))
    return bm


def apply_asymmetry(bm: bmesh.types.BMesh, rng, amount: float) -> bmesh.types.BMesh:
    """Post-process vertex positions with random deviation."""
    for v in bm.verts:
        v.co.x += rng.uniform(-amount, amount) * 5
        v.co.y += rng.uniform(-amount, amount) * 5
    return bm
