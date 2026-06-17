"""Dome generator — hemispherical dome ceiling."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class DomeGenerator(BaseGenerator):
    generator_id = "vault_dome"
    generator_name = "Dome"
    category = "Vaults"
    description = "Hemispherical dome ceiling"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT, 300, 50, 1500, "cm", "Dimensions"),
            ParameterDefinition("thickness", "Shell Thickness", ParamType.FLOAT, C.VAULT_THICKNESS, 5, 50, "cm", "Structure"),
            ParameterDefinition("height_fraction", "Height Fraction", ParamType.FLOAT, 1.0, 0.3, 1.5, "0.5=half dome, 1.0=full hemisphere", "Shape"),
            ParameterDefinition("base_ring", "Base Ring", ParamType.BOOL, True, "Add thickened base ring", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 32, 8, 64, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 300)
        thickness = params.get('thickness', C.VAULT_THICKNESS)
        h_frac = params.get('height_fraction', 1.0)
        has_base = params.get('base_ring', True)
        segs = params.get('segments', 32)

        max_angle = math.pi * 0.5 * h_frac
        n = segs

        # Outer dome profile
        profile = []
        for i in range(n + 1):
            t = i / n
            angle = max_angle * t
            r = radius * math.cos(angle)
            z = radius * math.sin(angle)
            profile.append((r, z))

        # Inner dome (reversed)
        inner_r = radius - thickness
        for i in range(n, -1, -1):
            t = i / n
            angle = max_angle * t
            r = inner_r * math.cos(angle)
            z = inner_r * math.sin(angle)
            profile.append((r, z))
        profile.append(profile[0])

        revolve_profile(bm, profile, segments=segs)

        # Base ring (thickened edge)
        if has_base:
            ring_w = thickness * 2
            ring_h = thickness * 3
            base_profile = [
                (radius - ring_w, 0),
                (radius + ring_w, 0),
                (radius + ring_w, ring_h),
                (radius - ring_w, ring_h),
                (radius - ring_w, 0),
            ]
            revolve_profile(bm, base_profile, segments=segs)
