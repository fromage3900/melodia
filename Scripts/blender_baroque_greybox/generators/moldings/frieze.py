"""
Frieze molding generator — dentil and egg-and-dart patterns.
"""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core.profile_curves import dentil_pattern, egg_and_dart
from ...core import constants as C


@register_generator
class FriezeGenerator(BaseGenerator):
    generator_id = "molding_frieze"
    generator_name = "Frieze"
    category = "Moldings"
    description = "Frieze band with dentil or egg-and-dart pattern"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Width", ParamType.FLOAT,
                                400, 50, 2000, "Total width", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT,
                                40, 10, 120, "Frieze height", "Dimensions"),
            ParameterDefinition("depth", "Depth", ParamType.FLOAT,
                                15, 5, 60, "Projection depth", "Dimensions"),
            ParameterDefinition("pattern", "Pattern", ParamType.ENUM,
                                "dentil", description="Ornament pattern",
                                category="Pattern",
                                enum_items=[
                                    ("dentil", "Dentil", "Rectangular teeth"),
                                    ("egg_dart", "Egg-and-Dart", "Alternating ovoids"),
                                    ("plain", "Plain", "Smooth band"),
                                ]),
            ParameterDefinition("pattern_count", "Pattern Count", ParamType.INT,
                                20, 4, 60, "Number of pattern repeats", "Pattern"),
            ParameterDefinition("length", "Length", ParamType.FLOAT,
                                400, 50, 4000, "Extrusion length", "Dimensions"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        height = params.get('height', 40)
        depth = params.get('depth', 15)
        pattern = params.get('pattern', 'dentil')
        count = params.get('pattern_count', 20)
        length = params.get('length', 400)

        if pattern == 'dentil':
            tooth_w = width / (count * 2)
            gap_w = tooth_w * 0.75
            tooth_h = height * 0.6
            profile = dentil_pattern(tooth_w, gap_w, tooth_h, count)
            # Add base and top
            total_w = count * (tooth_w + gap_w) - gap_w
            full_profile = [
                (-total_w / 2, -height * 0.2),
                (total_w / 2, -height * 0.2),
                (total_w / 2, 0),
            ]
            full_profile.extend([(x - total_w / 2, z) for x, z in profile])
            full_profile.extend([
                (total_w / 2, height * 0.8),
                (-total_w / 2, height * 0.8),
                (-total_w / 2, -height * 0.2),
            ])
            profile = full_profile

        elif pattern == 'egg_dart':
            egg_w = width / (count * 1.5)
            dart_w = egg_w * 0.5
            profile = egg_and_dart(egg_w, dart_w, height * 0.7, count)
            total_w = count * (egg_w + dart_w)
            full_profile = [
                (-total_w / 2, -height * 0.15),
                (total_w / 2, -height * 0.15),
                (total_w / 2, 0),
            ]
            full_profile.extend([(x - total_w / 2, z) for x, z in profile])
            full_profile.extend([
                (total_w / 2, height),
                (-total_w / 2, height),
                (-total_w / 2, -height * 0.15),
            ])
            profile = full_profile
        else:
            # Plain
            profile = [
                (-width / 2, 0),
                (width / 2, 0),
                (width / 2, height),
                (-width / 2, height),
                (-width / 2, 0),
            ]

        extrude_profile_linear(bm, profile, length, direction='Y',
                               offset=(0, -length / 2, 0))
