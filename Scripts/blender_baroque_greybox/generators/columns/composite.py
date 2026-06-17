"""
Composite column order generator.

Combines Corinthian acanthus leaves with Ionic volutes.
"""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_fluted_shaft, build_plinth, build_torus_base, build_abacus
from .corinthian import build_acanthus_capital
from .ionic import build_volute_capital
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class CompositeColumnGenerator(BaseGenerator):
    generator_id = "column_composite"
    generator_name = "Composite Column"
    category = "Columns"
    description = "Composite order combining Corinthian leaves + Ionic volutes"

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
                                80, 20, 300, "cm", "Capital"),
            ParameterDefinition("segments", "Radial Segments", ParamType.INT,
                                32, 8, 64, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        h = params.get('shaft_height', C.COLUMN_SHAFT_HEIGHT)
        r_bot = params.get('radius_bottom', C.COLUMN_RADIUS_BOTTOM)
        r_top = params.get('radius_top', C.COLUMN_RADIUS_TOP)
        flutes = params.get('fluting_count', 24)
        flute_d = params.get('fluting_depth', C.COLUMN_FLUTING_DEPTH)
        cap_h = params.get('capital_height', 80)
        segs = params.get('segments', 32)

        z = 0.0

        # Base plinth
        plinth_h = 25
        build_plinth(bm, r_bot * 2.8, r_bot * 2.8, plinth_h, z)
        z += plinth_h

        # Base moldings (Ionic-style)
        build_torus_base(bm, r_bot * 1.1, r_bot * 0.2, segs, z)
        z += r_bot * 0.35
        # Scotia concave band
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
        z += r_bot * 0.25

        # Shaft
        build_fluted_shaft(bm, h, r_bot, r_top, flutes, flute_d, 0.02,
                           segments=segs, z_offset=z)
        z += h

        # Acanthus leaves (lower portion of capital)
        leaf_h = cap_h * 0.6
        build_acanthus_capital(bm, r_top, leaf_h, leaf_rows=2,
                               segments=segs, z_offset=z)
        z += leaf_h

        # Volutes on top (Ionic element)
        volute_h = cap_h * 0.4
        build_volute_capital(bm, r_top * 0.9, volute_h, segs, z)
        z += volute_h

        # Abacus (Composite: more ornate, flowered corners)
        abacus_h = cap_h * 0.1
        abacus_w = r_top * 2.8
        build_abacus(bm, abacus_w, abacus_h, z)
