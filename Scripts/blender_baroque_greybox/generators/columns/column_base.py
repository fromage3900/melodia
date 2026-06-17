"""
Shared column shaft + base + capital logic.
"""

import math
import bmesh
from mathutils import Vector
from ...core.mesh_builder import revolve_profile, extrude_profile_linear
from ...core.profile_curves import column_shaft_profile, torus_profile
from ...core import constants as C


def build_fluted_shaft(bm, height, radius_bottom, radius_top,
                       fluting_count, fluting_depth, entasis,
                       segments=32, z_offset=0.0):
    """
    Build a fluted column shaft as a revolved profile with sinusoidal
    radial displacement for flutes.
    """
    # Get the base profile (with entasis)
    profile = column_shaft_profile(radius_bottom, radius_top, height,
                                   entasis, segments=segments)

    # Revolve the profile to create the shaft
    revolve_profile(bm, profile, segments=segments * 2)

    # Apply fluting by displacing vertices radially
    if fluting_count > 0 and fluting_depth > 0:
        bm.verts.ensure_lookup_table()
        for v in bm.verts:
            # Calculate angle around Y axis
            angle = math.atan2(v.z, v.x)
            # Sinusoidal displacement
            flute_disp = fluting_depth * math.sin(angle * fluting_count)
            # Only displace if it reduces radius (concave fluting)
            r = math.sqrt(v.x * v.x + v.z * v.z)
            new_r = max(r - max(0, flute_disp), 0.1)
            if r > 1e-6:
                scale = new_r / r
                v.x *= scale
                v.z *= scale

    # Shift to z_offset
    for v in bm.verts:
        v.co.z += z_offset


def build_plinth(bm, width, depth, height, z_offset=0.0):
    """Build a square/rectangular plinth (base block)."""
    extrude_profile_linear(
        bm,
        [(-width / 2, 0), (width / 2, 0), (width / 2, height),
         (-width / 2, height)],
        depth,
        direction='Y',
        offset=(0, -depth / 2, z_offset)
    )


def build_torus_base(bm, major_r, minor_r, segments=24, z_offset=0.0):
    """Build a torus molding ring (for column bases)."""
    profile = torus_profile(major_r, minor_r, segments=12)
    revolve_profile(bm, profile, segments=segments, offset=(0, z_offset, 0))


def build_abacus(bm, width, height, z_offset=0.0):
    """Build a square abacus slab (top of capital)."""
    extrude_profile_linear(
        bm,
        [(-width / 2, 0), (width / 2, 0), (width / 2, height),
         (-width / 2, height)],
        width,
        direction='Y',
        offset=(0, -width / 2, z_offset)
    )


def build_echinus(bm, radius_bottom, radius_top, height,
                  segments=24, z_offset=0.0):
    """Build an echinus (convex cushion capital element)."""
    profile = []
    n = 16
    for i in range(n + 1):
        t = i / n
        z = height * t
        # Convex curve (quarter ellipse)
        r = radius_bottom + (radius_top - radius_bottom) * math.sin(
            math.pi * 0.5 * t)
        profile.append((r, z))
    revolve_profile(bm, profile, segments=segments, offset=(0, z_offset, 0))
