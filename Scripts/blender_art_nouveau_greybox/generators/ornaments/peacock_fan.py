"""Peacock fan: radiating feather rays with eye spots."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .ornament_base import build_fan_motif
from ...core.mesh_builder import make_box


@register_generator
class PeacockFanGenerator(BaseGenerator):
    generator_id = 'peacock_fan'
    generator_name = 'Peacock Fan'
    category = 'Ornaments'
    description = 'Radiating peacock feather fan motif'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('radius', ParamType.FLOAT, default=80, min_val=20, max_val=200),
            ParameterDefinition('ray_count', ParamType.INT, default=12, min_val=5, max_val=32),
            ParameterDefinition('spread_angle', ParamType.FLOAT, default=120, min_val=30, max_val=180),
            ParameterDefinition('feather_width', ParamType.FLOAT, default=8, min_val=2, max_val=30),
            ParameterDefinition('feather_curl', ParamType.FLOAT, default=0.3, min_val=0, max_val=1),
            ParameterDefinition('depth', ParamType.FLOAT, default=5, min_val=1, max_val=20),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.1, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=16, min_val=8, max_val=32),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        build_fan_motif(bm, params['radius'], params['ray_count'],
                        math.radians(params['spread_angle']), z_offset=0)
        # Base plate
        make_box(bm, params['radius'] * 0.5, params['radius'] * 0.5, params['depth'])
        return bm
