"""Organic arch: asymmetric flowing arch via bezier sweep."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.bezier import CubicBezier
from ...core.mesh_builder import sweep_along_curve
from ...core import constants as C


@register_generator
class OrganicArchGenerator(BaseGenerator):
    generator_id = 'organic_arch'
    generator_name = 'Organic Arch'
    category = 'Arches'
    description = 'Asymmetric flowing arch (not semicircular)'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=C.ARCH_DEFAULT_WIDTH, min_val=50, max_val=800),
            ParameterDefinition('height', ParamType.FLOAT, default=C.ARCH_DEFAULT_HEIGHT, min_val=100, max_val=1000),
            ParameterDefinition('depth', ParamType.FLOAT, default=C.ARCH_DEFAULT_DEPTH, min_val=5, max_val=100),
            ParameterDefinition('thickness', ParamType.FLOAT, default=20, min_val=5, max_val=80),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('curvature', ParamType.FLOAT, default=0.6, min_val=0.1, max_val=1.0, description='How bent vs pointed'),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=64),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w = params['width']
        h = params['height']
        depth = params['depth']
        thickness = params['thickness']
        asym = params['asymmetry']
        curvature = params['curvature']

        # Asymmetric peak position
        peak_x = w * 0.5 + (asym - 0.5) * w * 0.3
        # Bezier arch path
        start = Vector((-w / 2, 0, 0))
        peak = Vector((peak_x - w / 2, 0, h))
        end = Vector((w / 2, 0, 0))
        # Control points for organic curve
        cp1 = Vector((peak_x * 0.3 - w / 2, 0, h * curvature))
        cp2 = Vector((w / 2 - (w - peak_x) * 0.3, 0, h * curvature))
        curve = CubicBezier(start, cp1, cp2, end)
        path = curve.sample(32)

        # Cross-section (rectangular)
        half_t = thickness / 2
        cross = [(-half_t, -half_t), (half_t, -half_t), (half_t, half_t), (-half_t, half_t)]
        sweep_along_curve(bm, cross, path)
        return bm
