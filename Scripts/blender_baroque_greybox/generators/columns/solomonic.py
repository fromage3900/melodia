"""
Solomonic (baroque twisted) column generator.

Features: helical twisted shaft, Corinthian capital.
The shaft spirals upward in a corkscrew pattern.
"""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_plinth, build_torus_base
from .corinthian import build_acanthus_capital
from ...core.mesh_builder import revolve_profile, sweep_along_curve
from ...core import constants as C


def build_twisted_shaft(bm, height, radius_bottom, radius_top,
                        twist_turns, fluting_count, fluting_depth,
                        segments=32, z_offset=0.0):
    """
    Build a Solomonic twisted shaft.
    The shaft radius follows a helical path as it rises.
    """
    n_rings = segments
    ring_resolution = segments

    # Build the twisted shaft as stacked rings with angular offset
    all_verts = []
    for i in range(n_rings + 1):
        t = i / n_rings
        z = z_offset + height * t

        # Taper
        r = radius_bottom + (radius_top - radius_bottom) * t

        # Entasis (subtle swelling)
        r += radius_bottom * 0.02 * math.sin(math.pi * t)

        # Twist angle at this height
        twist_angle = twist_turns * math.pi * 2.0 * t

        ring = []
        for j in range(ring_resolution):
            angle = (j / ring_resolution) * math.pi * 2.0 + twist_angle

            # Apply fluting
            r_mod = r
            if fluting_count > 0 and fluting_depth > 0:
                flute_disp = fluting_depth * math.sin(
                    (angle - twist_angle) * fluting_count)
                r_mod = max(r - max(0, flute_disp), 1.0)

            x = r_mod * math.cos(angle)
            z_v = z
            y_v = r_mod * math.sin(angle)
            v = bm.verts.new((x, y_v, z_v))
            ring.append(v)
        all_verts.append(ring)

    bm.verts.ensure_lookup_table()

    # Connect rings with faces
    for i in range(len(all_verts) - 1):
        for j in range(ring_resolution):
            j_next = (j + 1) % ring_resolution
            try:
                bm.faces.new([
                    all_verts[i][j],
                    all_verts[i][j_next],
                    all_verts[i + 1][j_next],
                    all_verts[i + 1][j],
                ])
            except ValueError:
                pass

    # Cap top and bottom
    if ring_resolution >= 3:
        try:
            bm.faces.new(list(reversed(all_verts[0])))
        except ValueError:
            pass
        try:
            bm.faces.new(all_verts[-1])
        except ValueError:
            pass


@register_generator
class SolomonicColumnGenerator(BaseGenerator):
    generator_id = "column_solomonic"
    generator_name = "Solomonic (Twisted) Column"
    category = "Columns"
    description = "Baroque twisted column with helical shaft and Corinthian capital"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("shaft_height", "Shaft Height", ParamType.FLOAT,
                                C.COLUMN_SHAFT_HEIGHT, 100, 2000, "cm", "Dimensions"),
            ParameterDefinition("radius_bottom", "Bottom Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_BOTTOM, 10, 100, "cm", "Dimensions"),
            ParameterDefinition("radius_top", "Top Radius", ParamType.FLOAT,
                                C.COLUMN_RADIUS_TOP, 8, 100, "cm", "Dimensions"),
            ParameterDefinition("twist_turns", "Twist Turns", ParamType.FLOAT,
                                2.0, 0.5, 6.0, "Number of full spiral turns", "Twist"),
            ParameterDefinition("fluting_count", "Fluting Count", ParamType.INT,
                                8, 0, 40, "Spiral flutes along shaft", "Detail"),
            ParameterDefinition("fluting_depth", "Fluting Depth", ParamType.FLOAT,
                                4.0, 0, 15, "Depth of spiral flutes", "Detail"),
            ParameterDefinition("capital_height", "Capital Height", ParamType.FLOAT,
                                70, 20, 300, "Corinthian capital height", "Capital"),
            ParameterDefinition("segments", "Resolution", ParamType.INT,
                                48, 16, 96, "Vertical and radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        h = params.get('shaft_height', C.COLUMN_SHAFT_HEIGHT)
        r_bot = params.get('radius_bottom', C.COLUMN_RADIUS_BOTTOM)
        r_top = params.get('radius_top', C.COLUMN_RADIUS_TOP)
        turns = params.get('twist_turns', 2.0)
        flutes = params.get('fluting_count', 8)
        flute_d = params.get('fluting_depth', 4.0)
        cap_h = params.get('capital_height', 70)
        segs = params.get('segments', 48)

        z = 0.0

        # Base plinth
        plinth_h = 30
        build_plinth(bm, r_bot * 3.0, r_bot * 3.0, plinth_h, z)
        z += plinth_h

        # Base moldings
        build_torus_base(bm, r_bot * 1.15, r_bot * 0.22, segs, z)
        z += r_bot * 0.4

        # Twisted shaft
        build_twisted_shaft(bm, h, r_bot, r_top, turns, flutes, flute_d,
                            segments=segs, z_offset=z)
        z += h

        # Corinthian capital
        build_acanthus_capital(bm, r_top, cap_h, leaf_rows=2,
                               segments=segs, z_offset=z)
