"""Vine tendril: spiral thick-to-thin sweep with sub-tendrils."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import sweep_along_curve
from ...core.bezier import CubicBezier


@register_generator
class VineTendrilGenerator(BaseGenerator):
    generator_id = 'vine_tendril'
    generator_name = 'Vine Tendril'
    category = 'Ornaments'
    description = 'Spiraling organic tendril with sub-branches'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('length', ParamType.FLOAT, default=200, min_val=50, max_val=800),
            ParameterDefinition('turns', ParamType.FLOAT, default=2.5, min_val=0.5, max_val=8),
            ParameterDefinition('radius_start', ParamType.FLOAT, default=4, min_val=1, max_val=20),
            ParameterDefinition('radius_end', ParamType.FLOAT, default=1, min_val=0.5, max_val=10),
            ParameterDefinition('curl_tightness', ParamType.FLOAT, default=0.6, min_val=0.1, max_val=1),
            ParameterDefinition('sub_tendrils', ParamType.INT, default=2, min_val=0, max_val=8),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.2, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=48, min_val=16, max_val=96),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        length = params['length']
        turns = params['turns']
        r_start = params['radius_start']
        r_end = params['radius_end']
        n_sub = params['sub_tendrils']
        seg = params['segments']

        # Main spiral path
        path = []
        for i in range(seg + 1):
            t = i / seg
            angle = t * turns * math.pi * 2
            r = r_start + (r_end - r_start) * t
            path.append(Vector((math.cos(angle) * r * 10, math.sin(angle) * r * 10, t * length)))

        # Sweep with tapering cross-section
        def cross_section(t):
            r = r_start * (1 - t * 0.7)
            return [(math.cos(a) * r, math.sin(a) * r) for a in [0, 1.57, 3.14, 4.71]]
        from ...core.mesh_builder import sweep_variable_section
        sweep_variable_section(bm, cross_section, path, seg // 2)
        return bm
