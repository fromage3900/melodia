"""Mosaic floor: Art Nouveau floor pattern."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box


@register_generator
class MosaicFloorGenerator(BaseGenerator):
    generator_id = 'mosaic_floor'
    generator_name = 'Mosaic Floor'
    category = 'Surfaces'
    description = 'Art Nouveau mosaic floor pattern'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('height', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('depth', ParamType.FLOAT, default=3, min_val=1, max_val=10),
            ParameterDefinition('pattern', ParamType.ENUM, default='whiplash_border',
                                enum_items=[('whiplash_border', 'Whiplash Border', 'S-curve border'), ('floral_medallion', 'Floral Medallion', 'Center floral motif'), ('flowing_lines', 'Flowing Lines', 'Parallel flowing lines')]),
            ParameterDefinition('tile_size', ParamType.FLOAT, default=20, min_val=5, max_val=80),
            ParameterDefinition('border_width', ParamType.FLOAT, default=40, min_val=10, max_val=150),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.1, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=8, min_val=4, max_val=16),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        make_box(bm, params['width'], params['height'], params['depth'])
        return bm
