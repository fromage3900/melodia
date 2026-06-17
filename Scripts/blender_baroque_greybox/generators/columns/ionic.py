"""
Ionic column order generator.

Features: volute capital (spiral scrolls), base with torus moldings,
typically 24 flutes.
"""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_fluted_shaft, build_plinth, build_torus_base, build_abacus
from ...core.mesh_builder import revolve_profile
from ...core.profile_curves import torus_profile
from ...core import constants as C


def build_volute_capital(bm, radius, height, segments=24, z_offset=0.0):
    """Build Ionic volute capital with spiral scrolls."""
    volute_r = radius * 1.4
    scroll_turns = 2.5
    scroll_segs = 32

    # Build volute spiral profile (side view)
    profile = []
    for i in range(scroll_segs + 1):
        t = i / scroll_segs
        angle = t * scroll_turns * math.pi * 2.0
        # Spiral: radius decreases as it goes up
        r = volute_r * (1.0 - t * 0.7)
        z = height * t
        profile.append((r, z))

    # Add center connection
    profile.append((radius * 0.3, height))

    # Revolve to create volute shape
    revolve_profile(bm, profile, segments=segments,
                    offset=(0, z_offset, 0))

    # Add connecting echinus band between volutes
    echinus_profile = [
        (radius * 1.1, 0),
        (radius * 1.2, height * 0.1),
        (radius * 1.15, height * 0.5),
        (radius * 1.0, height * 0.9),
        (radius * 0.9, height),
    ]
    revolve_profile(bm, echinus_profile, segments=segments,
                    offset=(0, z_offset, 0))


@register_generator
class IonicColumnGenerator(BaseGenerator):
    generator_id = "column_ionic"
    generator_name = "Ionic Column"
    category = "Columns"
    description = "Ionic order column with volute capital and molded base"

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
            ParameterDefinition("base_moldings", "Base Moldings", ParamType.BOOL,
                                True, description="Add torus + scotia base moldings",
                                category="Base"),
            ParameterDefinition("capital_height", "Capital Height", ParamType.FLOAT,
                                50, 10, 200, "cm", "Capital"),
            ParameterDefinition("segments", "Radial Segments", ParamType.INT,
                                32, 8, 64, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        h = params.get('shaft_height', C.COLUMN_SHAFT_HEIGHT)
        r_bot = params.get('radius_bottom', C.COLUMN_RADIUS_BOTTOM)
        r_top = params.get('radius_top', C.COLUMN_RADIUS_TOP)
        flutes = params.get('fluting_count', 24)
        flute_d = params.get('fluting_depth', C.COLUMN_FLUTING_DEPTH)
        has_base = params.get('base_moldings', True)
        cap_h = params.get('capital_height', 50)
        segs = params.get('segments', 32)

        z = 0.0

        # Base plinth
        plinth_h = 25
        build_plinth(bm, r_bot * 2.8, r_bot * 2.8, plinth_h, z)
        z += plinth_h

        # Base moldings (torus + scotia + torus)
        if has_base:
            build_torus_base(bm, r_bot * 1.1, r_bot * 0.2, segs, z)
            z += r_bot * 0.4
            # Scotia (concave)
            scotia_profile = [
                (r_bot * 1.0, 0),
                (r_bot * 0.85, r_bot * 0.15),
                (r_bot * 0.9, r_bot * 0.3),
                (r_bot * 1.0, r_bot * 0.4),
            ]
            revolve_profile(bm, scotia_profile, segments=segs,
                            offset=(0, z, 0))
            z += r_bot * 0.4
            build_torus_base(bm, r_bot * 1.0, r_bot * 0.15, segs, z)
            z += r_bot * 0.3

        # Shaft
        build_fluted_shaft(bm, h, r_bot, r_top, flutes, flute_d, 0.02,
                           segments=segs, z_offset=z)
        z += h

        # Volute capital
        build_volute_capital(bm, r_top, cap_h, segs, z)
        z += cap_h

        # Abacus
        abacus_w = r_top * 2.8
        build_abacus(bm, abacus_w, cap_h * 0.2, z)
