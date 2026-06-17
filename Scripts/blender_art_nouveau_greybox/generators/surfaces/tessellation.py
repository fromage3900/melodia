"""Tessellation: Art Nouveau tile pattern."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .surface_base import build_frame
from ...core.mesh_builder import make_box


@register_generator
class TessellationGenerator(BaseGenerator):
    generator_id = 'tessellation'
    generator_name = 'Tessellation Panel'
    category = 'Surfaces'
    description = 'Art Nouveau tile pattern (geometric + organic hybrid)'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('height', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('depth', ParamType.FLOAT, default=3, min_val=1, max_val=10),
            ParameterDefinition('tile_pattern', ParamType.ENUM, default='art_nouveau_hybrid',
                                enum_items=[('art_nouveau_hybrid', 'Art Nouveau Hybrid', 'Curved squares + floral'), ('flowing_squares', 'Flowing Squares', 'Rotated with connections'), ('organic_hex', 'Organic Hex', 'Hex with organic edges')]),
            ParameterDefinition('tile_size', ParamType.FLOAT, default=40, min_val=10, max_val=100),
            ParameterDefinition('relief_depth', ParamType.FLOAT, default=2, min_val=0, max_val=10),
            ParameterDefinition('grout_width', ParamType.FLOAT, default=2, min_val=0.5, max_val=5),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.1, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=8, min_val=4, max_val=16),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w, h, d = params['width'], params['height'], params['depth']
        ts = params['tile_size']
        # Base panel
        make_box(bm, w, h, d)
        return bm
