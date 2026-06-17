"""Floral capital: iris/water_lily/orchid petal rings."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .ornament_base import build_petal_ring
from ...core.mesh_builder import revolve_profile, make_box


@register_generator
class FloralCapitalGenerator(BaseGenerator):
    generator_id = 'floral_capital'
    generator_name = 'Floral Capital'
    category = 'Ornaments'
    description = 'Floral column capital (iris/water lily/orchid)'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('radius', ParamType.FLOAT, default=25, min_val=10, max_val=80),
            ParameterDefinition('height', ParamType.FLOAT, default=60, min_val=20, max_val=150),
            ParameterDefinition('flower_type', ParamType.ENUM, default='iris',
                                enum_items=[('iris', 'Iris', '3 upright + 3 falling petals'), ('water_lily', 'Water Lily', 'Many narrow petals'), ('orchid', 'Orchid', '5 broad petals + lip')]),
            ParameterDefinition('petal_count', ParamType.INT, default=6, min_val=3, max_val=16),
            ParameterDefinition('petal_curl', ParamType.FLOAT, default=0.4, min_val=0, max_val=1),
            ParameterDefinition('leaf_layer', ParamType.BOOL, default=True),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        r = params['radius']
        h = params['height']
        p_count = params['petal_count']
        curl = params['petal_curl']
        seg = params['segments']

        # Base ring
        profile = [(r * 0.9, 0), (r * 1.1, 0), (r * 1.1, 5), (r * 0.9, 5)]
        revolve_profile(bm, profile, seg)
        # Petal ring
        build_petal_ring(bm, r, p_count, h * 0.7, r * 0.4, curl, z_offset=5)
        # Abacus slab
        make_box(bm, r * 2.2, r * 2.2, 8)
        return bm
