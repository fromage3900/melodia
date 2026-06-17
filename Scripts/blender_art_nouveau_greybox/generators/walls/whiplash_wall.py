"""Whiplash wall: flat panel with signature S-curve relief."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .wall_base import build_wall_panel, apply_whiplash_relief
from ...core import constants as C


@register_generator
class WhiplashWallGenerator(BaseGenerator):
    generator_id = 'whiplash_wall'
    generator_name = 'Whiplash Wall'
    category = 'Walls'
    description = 'Wall panel with Art Nouveau S-curve relief pattern'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=C.WALL_WIDTH, min_val=50, max_val=2000),
            ParameterDefinition('height', ParamType.FLOAT, default=C.STORY_HEIGHT, min_val=50, max_val=2000),
            ParameterDefinition('thickness', ParamType.FLOAT, default=20, min_val=5, max_val=100),
            ParameterDefinition('relief_depth', ParamType.FLOAT, default=8, min_val=1, max_val=50),
            ParameterDefinition('whiplash_amplitude', ParamType.FLOAT, default=C.WHIPLASH_AMPLITUDE, min_val=5, max_val=200),
            ParameterDefinition('whiplash_count', ParamType.INT, default=3, min_val=1, max_val=10),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.2, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=32, min_val=8, max_val=64),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        build_wall_panel(bm, params['width'], params['height'], params['thickness'], params['segments'])
        apply_whiplash_relief(bm, params['whiplash_amplitude'], params['width'] / params['whiplash_count'],
                              params['asymmetry'], ctx.rng)
        return bm
