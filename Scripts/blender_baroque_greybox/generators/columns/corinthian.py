"""
Corinthian column order generator.

Features: ornate acanthus leaf capital (two rows of leaves + caulicoli),
base moldings, typically 24 flutes.
"""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_fluted_shaft, build_plinth, build_torus_base, build_abacus
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


def build_acanthus_capital(bm, radius, height, leaf_rows=2,
                           segments=24, z_offset=0.0):
    """
    Build Corinthian acanthus leaf capital.
    Two rows of stylized leaves + caulicoli (small volutes) + abacus.
    """
    # Lower row of leaves
    leaf_h = height * 0.4
    n_leaves = segments
    for i in range(n_leaves):
        angle = (i / n_leaves) * math.pi * 2.0
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)

        # Each leaf is a curved protrusion
        leaf_profile = []
        n_pts = 8
        for j in range(n_pts + 1):
            t = j / n_pts
            # Leaf shape: starts at shaft, curves outward, comes back
            r = radius + radius * 0.6 * math.sin(math.pi * t)
            z = leaf_h * t * 0.8
            leaf_profile.append((r, z))

        # Revolve a single leaf and rotate to position
        # Simplified: create ring of leaf-like protrusions
        leaf_r = radius * (1.0 + 0.5 * math.sin(math.pi * 0.5))
        x = (radius + radius * 0.3) * cos_a
        y = 0
        z_pos = z_offset + leaf_h * 0.3

        # Create leaf as a small box-like protrusion
        leaf_size = radius * 0.4
        v0 = bm.verts.new((x - leaf_size * 0.3 * sin_a,
                           y - leaf_size * 0.5,
                           z_pos))
        v1 = bm.verts.new((x + leaf_size * 0.3 * sin_a,
                           y - leaf_size * 0.5,
                           z_pos))
        v2 = bm.verts.new((x + leaf_size * 0.5 * cos_a,
                           y,
                           z_pos + leaf_h * 0.5))
        v3 = bm.verts.new((x + leaf_size * 0.3 * sin_a,
                           y + leaf_size * 0.5,
                           z_pos))
        v4 = bm.verts.new((x - leaf_size * 0.3 * sin_a,
                           y + leaf_size * 0.5,
                           z_pos))

    # Upper row of leaves (smaller, higher)
    upper_leaf_h = height * 0.35
    upper_r = radius * 0.95
    profile_upper = []
    n_pts = 12
    for j in range(n_pts + 1):
        t = j / n_pts
        r = upper_r + radius * 0.4 * math.sin(math.pi * t)
        z = height * 0.4 + upper_leaf_h * t
        profile_upper.append((r, z))
    profile_upper.append((radius * 0.8, height * 0.85))
    revolve_profile(bm, profile_upper, segments=segments,
                    offset=(0, z_offset, 0))

    # Caulicoli (small scrolling elements at corners)
    cauli_r = radius * 0.15
    cauli_h = height * 0.2
    for i in range(4):
        angle = (i / 4.0) * math.pi * 2.0 + math.pi / 4.0
        cx = radius * 0.9 * math.cos(angle)
        cz = radius * 0.9 * math.sin(angle)
        cy = z_offset + height * 0.75
        # Small volute as a torus segment
        cauli_profile = [
            (cauli_r, 0),
            (cauli_r * 1.5, cauli_h * 0.3),
            (cauli_r * 0.8, cauli_h * 0.7),
            (cauli_r * 1.2, cauli_h),
        ]
        revolve_profile(bm, cauli_profile, segments=8,
                        offset=(cx, cy, cz))

    # Abacus (distinctive Corinthian: concave-sided, flowered)
    abacus_h = height * 0.15
    abacus_profile = [
        (radius * 1.3, height * 0.85),
        (radius * 1.35, height * 0.85 + abacus_h * 0.3),
        (radius * 1.2, height * 0.85 + abacus_h * 0.7),
        (radius * 1.3, height),
    ]
    revolve_profile(bm, abacus_profile, segments=segments,
                    offset=(0, z_offset, 0))


@register_generator
class CorinthianColumnGenerator(BaseGenerator):
    generator_id = "column_corinthian"
    generator_name = "Corinthian Column"
    category = "Columns"
    description = "Corinthian order with ornate acanthus leaf capital"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("shaft_height", "Shaft Height", ParamType.FLOAT,
                                C.COLUMN_SHAFT_HEIGHT, 100, 2000, "cm", "Dimensions"),
            ParameterDefinition("radius_bottom", "Bottom Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_BOTTOM, 10, 100, "cm", "Dimensions"),
            ParameterDefinition("radius_top", "Top Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_TOP, 8, 100, "cm", "Dimensions"),
            ParameterDefinition("fluting_count", "Fluting Count", ParamType.INT,
                                24, 0, 40, "Number of flutes", "Detail"),
            ParameterDefinition("fluting_depth", "Fluting Depth", ParamType.FLOAT,
                                C.COLUMN_FLUTING_DEPTH, 0, 10, "cm", "Detail"),
            ParameterDefinition("capital_height", "Capital Height", ParamType.FLOAT,
                                70, 20, 300, "cm", "Capital"),
            ParameterDefinition("leaf_rows", "Leaf Rows", ParamType.INT,
                                2, 1, 3, "Rows of acanthus leaves", "Capital"),
            ParameterDefinition("segments", "Radial Segments", ParamType.INT,
                                32, 8, 64, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        h = params.get('shaft_height', C.COLUMN_SHAFT_HEIGHT)
        r_bot = params.get('radius_bottom', C.COLUMN_RADIUS_BOTTOM)
        r_top = params.get('radius_top', C.COLUMN_RADIUS_TOP)
        flutes = params.get('fluting_count', 24)
        flute_d = params.get('fluting_depth', C.COLUMN_FLUTING_DEPTH)
        cap_h = params.get('capital_height', 70)
        leaf_rows = params.get('leaf_rows', 2)
        segs = params.get('segments', 32)

        z = 0.0

        # Base plinth
        plinth_h = 25
        build_plinth(bm, r_bot * 2.8, r_bot * 2.8, plinth_h, z)
        z += plinth_h

        # Base moldings
        build_torus_base(bm, r_bot * 1.1, r_bot * 0.2, segs, z)
        z += r_bot * 0.35
        build_torus_base(bm, r_bot * 1.0, r_bot * 0.15, segs, z)
        z += r_bot * 0.25

        # Shaft
        build_fluted_shaft(bm, h, r_bot, r_top, flutes, flute_d, 0.02,
                           segments=segs, z_offset=z)
        z += h

        # Acanthus capital
        build_acanthus_capital(bm, r_top, cap_h, leaf_rows, segs, z)
