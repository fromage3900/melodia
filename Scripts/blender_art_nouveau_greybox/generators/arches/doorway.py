"""Art Nouveau doorway: organic arch with integral surround."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.bezier import CubicBezier
from ...core.mesh_builder import sweep_along_curve, make_box
from ...core.profile_curves import whiplash_molding


@register_generator
class ArtNouveauDoorwayGenerator(BaseGenerator):
    generator_id = 'doorway'
    generator_name = 'Art Nouveau Doorway'
    category = 'Arches'
    description = 'Organic arch doorway with integral surround'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=120, min_val=60, max_val=300),
            ParameterDefinition('height', ParamType.FLOAT, default=280, min_val=150, max_val=500),
            ParameterDefinition('depth', ParamType.FLOAT, default=30, min_val=10, max_val=80),
            ParameterDefinition('surround_width', ParamType.FLOAT, default=20, min_val=5, max_val=60),
            ParameterDefinition('surround_molding', ParamType.BOOL, default=True),
            ParameterDefinition('lintel_style', ParamType.ENUM, default='whiplash',
                                enum_items=[('whiplash', 'Whiplash', 'S-curve lintel'), ('floral', 'Floral', 'Floral relief'), ('plain', 'Plain', 'Simple flat')]),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.1, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=24, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w = params['width']
        h = params['height']
        depth = params['depth']
        sw = params['surround_width']
        asym = params['asymmetry']

        # Arch opening
        peak_x = w * 0.5 + (asym - 0.5) * w * 0.2
        start = Vector((-w / 2, 0, 0))
        end = Vector((w / 2, 0, 0))
        cp1 = Vector((-w / 4, 0, h * 0.7))
        cp2 = Vector((w / 4, 0, h * 0.7))
        curve = CubicBezier(start, cp1, cp2, end)
        path = curve.sample(24)

        # Arch thickness
        cross = [(-w * 0.05, -w * 0.05), (w * 0.05, -w * 0.05), (w * 0.05, w * 0.05), (-w * 0.05, w * 0.05)]
        from ...core.mesh_builder import sweep_along_curve
        sweep_along_curve(bm, cross, path)

        # Surround frame
        from ...core.mesh_builder import make_box
        make_box(bm, sw * 2, h, depth, Vector((-w / 2 - sw / 2, 0, h / 2)))
        make_box(bm, sw * 2, h, depth, Vector((w / 2 + sw / 2, 0, h / 2)))
        return bm
