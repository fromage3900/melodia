"""Lily pad bracket: organic corbel with radial vein relief."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile, make_box


@register_generator
class LilyBracketGenerator(BaseGenerator):
    generator_id = 'lily_bracket'
    generator_name = 'Lily Pad Bracket'
    category = 'Ornaments'
    description = 'Lily pad shaped corbel/bracket'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('radius', ParamType.FLOAT, default=40, min_val=10, max_val=100),
            ParameterDefinition('depth', ParamType.FLOAT, default=30, min_val=5, max_val=80),
            ParameterDefinition('notch_angle', ParamType.FLOAT, default=30, min_val=0, max_val=90),
            ParameterDefinition('vein_count', ParamType.INT, default=8, min_val=4, max_val=24),
            ParameterDefinition('vein_relief', ParamType.FLOAT, default=3, min_val=0, max_val=15),
            ParameterDefinition('curl_edges', ParamType.FLOAT, default=0.2, min_val=0, max_val=1),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        r = params['radius']
        d = params['depth']
        seg = params['segments']
        # Half lily pad profile
        profile = [(0, 0), (r, 0), (r * 0.9, d * 0.3), (r * 0.6, d * 0.6), (r * 0.2, d), (0, d)]
        revolve_profile(bm, profile, seg, angle=math.pi * 1.5)
        # Base block for mounting
        make_box(bm, r * 0.4, r * 0.4, d * 0.3)
        return bm
