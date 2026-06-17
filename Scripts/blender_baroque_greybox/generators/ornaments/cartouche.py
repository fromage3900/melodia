"""Cartouche generator — scroll-frame shield ornament."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core import constants as C


@register_generator
class CartoucheGenerator(BaseGenerator):
    generator_id = "ornament_cartouche"
    generator_name = "Cartouche"
    category = "Ornaments"
    description = "Scroll-frame shield ornament (baroque relief)"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Width", ParamType.FLOAT, 80, 20, 300, "cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT, 120, 30, 400, "cm", "Dimensions"),
            ParameterDefinition("depth", "Relief Depth", ParamType.FLOAT, 10, 2, 40, "How far relief protrudes", "Shape"),
            ParameterDefinition("scroll_size", "Scroll Size", ParamType.FLOAT, 20, 5, 60, "Corner scroll radius", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 16, 4, 32, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 80)
        height = params.get('height', 120)
        depth = params.get('depth', 10)
        scroll = params.get('scroll_size', 20)
        segs = params.get('segments', 16)

        hw = width / 2
        hh = height / 2

        # Shield outline with scrolled top and bottom
        profile = []
        # Bottom scroll (left)
        n = segs // 2
        for i in range(n + 1):
            t = i / n
            angle = math.pi * (1.0 + t)
            x = -hw + scroll * 0.5 + scroll * 0.5 * math.cos(angle)
            z = -hh + scroll * 0.5 * math.sin(angle) + scroll * 0.5
            profile.append((x, z))

        # Left side up
        profile.append((-hw, -hh * 0.3))
        profile.append((-hw * 0.9, 0))
        profile.append((-hw, hh * 0.3))

        # Top scroll (left to right)
        for i in range(n + 1):
            t = i / n
            angle = math.pi * t
            x = -hw + scroll + (width - 2 * scroll) * t
            z = hh + scroll * 0.3 * math.sin(angle)
            profile.append((x, z))

        # Right side down
        profile.append((hw, hh * 0.3))
        profile.append((hw * 0.9, 0))
        profile.append((hw, -hh * 0.3))

        # Bottom scroll (right)
        for i in range(n + 1):
            t = i / n
            angle = math.pi * t
            x = hw - scroll * 0.5 + scroll * 0.5 * math.cos(angle)
            z = -hh + scroll * 0.5 * math.sin(angle) + scroll * 0.5
            profile.append((x, z))

        # Close
        profile.append(profile[0])

        extrude_profile_linear(bm, profile, depth, direction='Y',
                               offset=(0, -depth / 2, 0))
