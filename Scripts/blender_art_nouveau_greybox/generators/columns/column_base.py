"""Art Nouveau column building blocks."""

import bmesh
from mathutils import Vector
from ...core.mesh_builder import revolve_profile, sweep_variable_section
from ...core.profile_curves import stem_profile, organic_column_profile


def build_stem_shaft(bm: bmesh.types.BMesh, height: float, r_bot: float, r_top: float,
                     node_count: int = 3, node_bulge: float = 0.15,
                     segments: int = 24, z_offset: float = 0.0) -> bmesh.types.BMesh:
    """Shaft with organic node swellings."""
    profile = stem_profile(r_bot, r_top, height, node_count, node_bulge, segments)
    # Shift profile to z_offset
    profile = [(r, z + z_offset) for r, z in profile]
    return revolve_profile(bm, profile, segments)


def build_branching_top(bm: bmesh.types.BMesh, shaft_radius: float,
                        branch_count: int = 3, branch_angle: float = 35,
                        branch_length: float = 120, rng=None,
                        z_offset: float = 0.0) -> bmesh.types.BMesh:
    """Create branching stems at column top."""
    import math
    from ...core.bezier import CubicBezier
    from ...core.math_utils import asymmetric_offset

    angle_rad = math.radians(branch_angle)
    for i in range(branch_count):
        base_angle = (i / branch_count) * math.pi * 2
        if rng:
            base_angle += asymmetric_offset(rng, 0.3)
        # Branch direction
        direction = Vector((math.cos(base_angle) * math.sin(angle_rad),
                            math.sin(base_angle) * math.sin(angle_rad),
                            math.cos(angle_rad)))
        start = Vector((0, 0, z_offset))
        # Bezier curve for branch
        mid = start + direction * branch_length * 0.5 + Vector((0, 0, branch_length * 0.2))
        end = start + direction * branch_length
        curve = CubicBezier(start, mid, end * 0.7 + start * 0.3, end)
        path = curve.sample(20)
        # Cross-section that tapers
        def section(t):
            r = shaft_radius * (1.0 - t * 0.7)
            pts = []
            for j in range(12):
                a = (j / 12) * math.pi * 2
                pts.append((math.cos(a) * r, math.sin(a) * r))
            return pts
        sweep_variable_section(bm, section, path, 16)
    return bm


def build_organic_base(bm: bmesh.types.BMesh, radius: float,
                       segments: int = 24, z_offset: float = 0.0) -> bmesh.types.BMesh:
    """Simplified organic base (no classical torus/scotia)."""
    profile = []
    # Simple stepped base
    profile.append((radius * 1.3, z_offset))
    profile.append((radius * 1.3, z_offset + 5))
    profile.append((radius * 1.1, z_offset + 5))
    profile.append((radius * 1.1, z_offset + 10))
    profile.append((radius, z_offset + 10))
    profile.append((radius, z_offset + 15))
    return revolve_profile(bm, profile, segments)
