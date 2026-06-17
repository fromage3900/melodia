"""Stained glass panel: geometric/organic window with lead-line patterns."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from .surface_base import build_frame
from ...core.mesh_builder import make_box


@register_generator
class StainedGlassGenerator(BaseGenerator):
    generator_id = 'stained_glass'
    generator_name = 'Stained Glass Panel'
    category = 'Surfaces'
    description = 'Art Nouveau stained glass window with lead-line pattern'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=160, min_val=60, max_val=500),
            ParameterDefinition('height', ParamType.FLOAT, default=280, min_val=100, max_val=800),
            ParameterDefinition('depth', ParamType.FLOAT, default=3, min_val=1, max_val=10),
            ParameterDefinition('frame_width', ParamType.FLOAT, default=8, min_val=2, max_val=30),
            ParameterDefinition('pattern', ParamType.ENUM, default='iris',
                                enum_items=[('iris', 'Iris', 'Iris petal pattern'), ('water_lily', 'Water Lily', 'Radiating petals'), ('abstract', 'Abstract', 'Voronoi-like cells'), ('radial', 'Radial', 'Sunburst')]),
            ParameterDefinition('panel_count', ParamType.INT, default=12, min_val=4, max_val=40),
            ParameterDefinition('lead_width', ParamType.FLOAT, default=2, min_val=0.5, max_val=8),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.15, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w, h, d = params['width'], params['height'], params['depth']
        fw = params['frame_width']
        build_frame(bm, w, h, d, fw)
        # Glass panel (flat plate inside frame)
        make_box(bm, w - fw * 2, h - fw * 2, d - 1)
        return bm
