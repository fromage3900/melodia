"""Curved facade: multi-story with undulating surface."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear, make_box
from ...core import constants as C


@register_generator
class CurvedFacadeGenerator(BaseGenerator):
    generator_id = 'curved_facade'
    generator_name = 'Curved Facade'
    category = 'Walls'
    description = 'Flowing asymmetric facade with undulating surface and windows'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=1200, min_val=200, max_val=4000),
            ParameterDefinition('story_height', ParamType.FLOAT, default=C.STORY_HEIGHT, min_val=200, max_val=1000),
            ParameterDefinition('stories', ParamType.INT, default=2, min_val=1, max_val=6),
            ParameterDefinition('wall_thickness', ParamType.FLOAT, default=C.FACADE_WALL_THICKNESS, min_val=5, max_val=100),
            ParameterDefinition('undulation_amplitude', ParamType.FLOAT, default=30, min_val=0, max_val=200),
            ParameterDefinition('undulation_frequency', ParamType.FLOAT, default=3, min_val=1, max_val=10),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.25, min_val=0, max_val=1),
            ParameterDefinition('window_enabled', ParamType.BOOL, default=True),
            ParameterDefinition('window_width', ParamType.FLOAT, default=140, min_val=40, max_val=400),
            ParameterDefinition('window_height', ParamType.FLOAT, default=240, min_val=60, max_val=500),
            ParameterDefinition('segments', ParamType.INT, default=48, min_val=16, max_val=128),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w = params['width']
        total_h = params['story_height'] * params['stories']
        thickness = params['wall_thickness']
        amp = params['undulation_amplitude']
        freq = params['undulation_frequency']
        asym = params['asymmetry']
        segments = params['segments']

        # Undulating profile
        profile = []
        for i in range(segments + 1):
            t = i / segments
            x = t * w
            z = amp * math.sin(math.pi * freq * t)
            if asym > 0:
                z += asym * amp * 0.3 * math.sin(math.pi * (freq + 1) * t)
            profile.append((x, z))
        profile.append((profile[-1][0], total_h + profile[-1][1]))
        profile.append((profile[0][0], total_h + profile[0][1]))
        extrude_profile_linear(bm, profile, thickness, segments // 2)
        return bm
