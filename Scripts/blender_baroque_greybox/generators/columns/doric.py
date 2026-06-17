"""
Doric column order generator.

The simplest classical order: plain echinus + abacus capital,
no base (or simple stylobate), typically 20 flutes.
"""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_fluted_shaft, build_plinth, build_echinus, build_abacus
from ...core import constants as C


@register_generator
class DoricColumnGenerator(BaseGenerator):
    generator_id = "column_doric"
    generator_name = "Doric Column"
    category = "Columns"
    description = "Classical Doric order column with plain capital and fluted shaft"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("shaft_height", "Shaft Height", ParamType.FLOAT,
                                C.COLUMN_SHAFT_HEIGHT, 100, 2000,
                                "Height of the column shaft in cm", "Dimensions"),
            ParameterDefinition("radius_bottom", "Bottom Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_BOTTOM, 10, 100,
                                "Shaft radius at base in cm", "Dimensions"),
            ParameterDefinition("radius_top", "Top Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_TOP, 8, 100,
                                "Shaft radius at top in cm", "Dimensions"),
            ParameterDefinition("fluting_count", "Fluting Count", ParamType.INT,
                                20, 0, 40, "Number of flutes (0=smooth)", "Detail"),
            ParameterDefinition("fluting_depth", "Fluting Depth", ParamType.FLOAT,
                                C.COLUMN_FLUTING_DEPTH, 0, 10,
                                "Depth of each flute in cm", "Detail"),
            ParameterDefinition("entasis", "Entasis", ParamType.FLOAT,
                                C.COLUMN_ENTASIS, 0, 0.1,
                                "Subtle shaft swelling ratio", "Detail"),
            ParameterDefinition("plinth_height", "Plinth Height", ParamType.FLOAT,
                                20, 0, 100, "Height of base plinth", "Base"),
            ParameterDefinition("capital_height", "Capital Height", ParamType.FLOAT,
                                40, 10, 200, "Total capital height", "Capital"),
            ParameterDefinition("segments", "Radial Segments", ParamType.INT,
                                32, 8, 64, "Radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        h = params.get('shaft_height', C.COLUMN_SHAFT_HEIGHT)
        r_bot = params.get('radius_bottom', C.COLUMN_RADIUS_BOTTOM)
        r_top = params.get('radius_top', C.COLUMN_RADIUS_TOP)
        flutes = params.get('fluting_count', 20)
        flute_d = params.get('fluting_depth', C.COLUMN_FLUTING_DEPTH)
        entasis = params.get('entasis', C.COLUMN_ENTASIS)
        plinth_h = params.get('plinth_height', 20)
        cap_h = params.get('capital_height', 40)
        segs = params.get('segments', 32)

        z = 0.0

        # Plinth (simple square base)
        if plinth_h > 0:
            plinth_w = r_bot * 2.8
            build_plinth(bm, plinth_w, plinth_w, plinth_h, z)
            z += plinth_h

        # Shaft
        build_fluted_shaft(bm, h, r_bot, r_top, flutes, flute_d, entasis,
                           segments=segs, z_offset=z)
        z += h

        # Echinus (convex cushion)
        echinus_h = cap_h * 0.4
        build_echinus(bm, r_top, r_top * 1.3, echinus_h,
                      segments=segs, z_offset=z)
        z += echinus_h

        # Abacus (square slab)
        abacus_h = cap_h * 0.6
        abacus_w = r_top * 2.6
        build_abacus(bm, abacus_w, abacus_h, z)
