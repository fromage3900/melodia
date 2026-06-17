"""Wrought iron railing: organic balustrade with whiplash balusters."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box, sweep_along_curve
from ...core.bezier import CubicBezier


@register_generator
class IronRailingGenerator(BaseGenerator):
    generator_id = 'iron_railing'
    generator_name = 'Wrought Iron Railing'
    category = 'Surfaces'
    description = 'Art Nouveau organic wrought iron railing'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('length', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('height', ParamType.FLOAT, default=100, min_val=30, max_val=200),
            ParameterDefinition('baluster_count', ParamType.INT, default=8, min_val=2, max_val=32),
            ParameterDefinition('baluster_style', ParamType.ENUM, default='whiplash',
                                enum_items=[('whiplash', 'Whiplash', 'S-curve balusters'), ('vine', 'Vine', 'Helical with leaves'), ('scroll', 'Scroll', 'C-scroll pattern')]),
            ParameterDefinition('rail_height', ParamType.FLOAT, default=8, min_val=2, max_val=20),
            ParameterDefinition('rail_depth', ParamType.FLOAT, default=4, min_val=1, max_val=15),
            ParameterDefinition('newel_enabled', ParamType.BOOL, default=True),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=16, min_val=8, max_val=32),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        length = params['length']
        height = params['height']
        n_balusters = params['baluster_count']
        rh, rd = params['rail_height'], params['rail_depth']
        # Top rail
        make_box(bm, length, rd, rh, Vector((0, 0, height)))
        # Bottom rail
        make_box(bm, length, rd, rh, Vector((0, 0, rh)))
        # Newel posts
        if params['newel_enabled']:
            make_box(bm, rd * 2, rd * 2, height + rh, Vector((-length / 2, 0, height / 2)))
            make_box(bm, rd * 2, rd * 2, height + rh, Vector((length / 2, 0, height / 2)))
        return bm
