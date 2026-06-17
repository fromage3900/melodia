"""Volute generator — logarithmic spiral with thickness (Ionic scroll)."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core import constants as C


@register_generator
class VoluteGenerator(BaseGenerator):
    generator_id = "ornament_volute"
    generator_name = "Volute"
    category = "Ornaments"
    description = "Ionic volute spiral with thickness"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Outer Radius", ParamType.FLOAT, 30, 5, 100, "cm", "Dimensions"),
            ParameterDefinition("turns", "Turns", ParamType.FLOAT, 2.5, 1, 5, "Spiral turns", "Shape"),
            ParameterDefinition("thickness", "Thickness", ParamType.FLOAT, 8, 2, 30, "Spiral band thickness", "Shape"),
            ParameterDefinition("depth", "Depth", ParamType.FLOAT, 15, 2, 60, "Extrusion depth", "Dimensions"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 64, 16, 128, "Spiral resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 30)
        turns = params.get('turns', 2.5)
        thickness = params.get('thickness', 8)
        depth = params.get('depth', 15)
        segs = params.get('segments', 64)

        # Generate logarithmic spiral points
        b = math.log(C.PHI) / (math.pi / 2.0)
        outer_pts = []
        inner_pts = []

        for i in range(segs + 1):
            t = i / segs
            theta = t * turns * math.pi * 2.0
            r_outer = radius * math.exp(-b * theta / (turns * math.pi * 2.0))
            r_inner = max(r_outer - thickness, 1.0)

            x_o = r_outer * math.cos(theta)
            z_o = r_outer * math.sin(theta)
            x_i = r_inner * math.cos(theta)
            z_i = r_inner * math.sin(theta)

            outer_pts.append((x_o, z_o))
            inner_pts.append((x_i, z_i))

        # Build closed profile
        profile = outer_pts[:]
        profile.extend(reversed(inner_pts))
        profile.append(outer_pts[0])

        extrude_profile_linear(bm, profile, depth, direction='Y',
                               offset=(0, -depth / 2, 0))
