"""Butterfly/Liberty panel: winged decorative relief."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box


@register_generator
class ButterflyPanelGenerator(BaseGenerator):
    generator_id = 'butterfly_panel'
    generator_name = 'Butterfly Panel'
    category = 'Ornaments'
    description = 'Winged decorative relief panel'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=100, min_val=30, max_val=300),
            ParameterDefinition('height', ParamType.FLOAT, default=80, min_val=30, max_val=250),
            ParameterDefinition('depth', ParamType.FLOAT, default=6, min_val=1, max_val=20),
            ParameterDefinition('wing_style', ParamType.ENUM, default='butterfly',
                                enum_items=[('butterfly', 'Butterfly', '4 wing shapes'), ('liberty', 'Liberty', 'Abstract flowing wings'), ('dragonfly', 'Dragonfly', '4 narrow wings')]),
            ParameterDefinition('wing_spread', ParamType.FLOAT, default=0.7, min_val=0.2, max_val=1),
            ParameterDefinition('body_prominence', ParamType.FLOAT, default=0.5, min_val=0.1, max_val=1),
            ParameterDefinition('antennae', ParamType.BOOL, default=True),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w = params['width']
        h = params['height']
        d = params['depth']
        # Base plate
        make_box(bm, w, h, d)
        # Wing shapes as raised panels
        make_box(bm, w * params['wing_spread'], h * 0.3, d + 3, Vector((0, h * 0.2, 0)))
        make_box(bm, w * params['wing_spread'], h * 0.3, d + 3, Vector((0, -h * 0.2, 0)))
        return bm
