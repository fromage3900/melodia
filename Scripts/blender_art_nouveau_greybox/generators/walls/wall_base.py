"""Shared wall construction utilities."""

import bmesh
from mathutils import Vector
from ...core.mesh_builder import extrude_profile_linear, make_box
from ...core.whiplash import whiplash_relief


def build_wall_panel(bm: bmesh.types.BMesh, width: float, height: float,
                     thickness: float, segments: int = 8) -> bmesh.types.BMesh:
    """Basic flat wall panel."""
    profile = [(0, 0), (width, 0), (width, height), (0, height)]
    extrude_profile_linear(bm, profile, thickness, segments)
    return bm


def apply_whiplash_relief(bm: bmesh.types.BMesh, amplitude: float, wavelength: float,
                          asymmetry: float = 0.0, rng=None) -> bmesh.types.BMesh:
    """Displace wall face vertices using whiplash curve pattern."""
    for v in bm.verts:
        t = v.co.x / wavelength if wavelength > 0 else 0
        import math
        z_offset = amplitude * math.sin(math.pi * 2 * t)
        if asymmetry > 0 and rng:
            z_offset += rng.uniform(-asymmetry * amplitude, asymmetry * amplitude)
        v.co.z += z_offset * 0.3
    return bm


def build_curved_wall_segment(bm: bmesh.types.BMesh, width: float, height: float,
                               thickness: float, curvature: float = 0.1,
                               asymmetry: float = 0.0, segments: int = 16) -> bmesh.types.BMesh:
    """Wall with flowing curved surface."""
    import math
    profile = []
    for i in range(segments + 1):
        t = i / segments
        x = t * width
        z = curvature * width * math.sin(math.pi * t)
        if asymmetry > 0:
            z += (asymmetry * width * 0.1) * math.sin(math.pi * 3 * t)
        profile.append((x, z))
    profile.append((profile[-1][0], height + profile[-1][1]))
    profile.append((profile[0][0], height + profile[0][1]))
    extrude_profile_linear(bm, profile, thickness, segments)
    return bm
