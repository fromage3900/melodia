"""Plant-stem column generator: tapered shaft with node swellings + branching top."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .column_base import build_stem_shaft, build_branching_top, build_organic_base
from ...core import constants as C


@register_generator
class StemColumnGenerator(BaseGenerator):
    generator_id = 'stem_column'
    generator_name = 'Stem Column'
    category = 'Columns'
    description = 'Plant-stem column with organic node swellings and branching top'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('shaft_height', ParamType.FLOAT, default=C.COLUMN_SHAFT_HEIGHT, min_val=100, max_val=2000, description='Shaft height in cm'),
            ParameterDefinition('radius_bottom', ParamType.FLOAT, default=C.COLUMN_RADIUS_BOTTOM, min_val=5, max_val=100, description='Base radius'),
            ParameterDefinition('radius_top', ParamType.FLOAT, default=C.COLUMN_RADIUS_TOP, min_val=5, max_val=100, description='Top radius'),
            ParameterDefinition('node_count', ParamType.INT, default=C.STEM_DEFAULT_NODES, min_val=0, max_val=10, description='Number of node swellings'),
            ParameterDefinition('node_bulge', ParamType.FLOAT, default=C.STEM_NODE_BULGE, min_val=0, max_val=0.5, description='Node bulge amount'),
            ParameterDefinition('branch_count', ParamType.INT, default=C.STEM_BRANCH_COUNT, min_val=1, max_val=8, description='Number of branches'),
            ParameterDefinition('branch_angle', ParamType.FLOAT, default=C.STEM_BRANCH_ANGLE, min_val=0, max_val=90, description='Branch angle from vertical'),
            ParameterDefinition('branch_length', ParamType.FLOAT, default=120, min_val=20, max_val=500, description='Branch length'),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1, description='Organic asymmetry'),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=64, description='Profile resolution'),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        height = params['shaft_height']
        r_bot = params['radius_bottom']
        r_top = params['radius_top']
        node_count = params['node_count']
        node_bulge = params['node_bulge']
        branch_count = params['branch_count']
        branch_angle = params['branch_angle']
        branch_length = params['branch_length']
        segments = params['segments']

        # Base
        build_organic_base(bm, r_bot, segments, z_offset=0)
        # Shaft
        build_stem_shaft(bm, height, r_bot, r_top, node_count, node_bulge, segments, z_offset=15)
        # Branching top
        build_branching_top(bm, r_top, branch_count, branch_angle, branch_length, ctx.rng, z_offset=15 + height)
        return bm
