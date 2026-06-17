"""
Architrave molding generator — layered fascia bands.
"""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core import constants as C


@register_generator
class ArchitraveGenerator(BaseGenerator):
    generator_id = "molding_architrave"
    generator_name = "Architrave"
    category = "Moldings"
    description = "Layered fascia band molding (architrave)"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Width", ParamType.FLOAT,
                                400, 50, 2000, "Total width in cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT,
                                30, 5, 100, "Total height in cm", "Dimensions"),
            ParameterDefinition("depth", "Depth", ParamType.FLOAT,
                                20, 5, 80, "Projection depth in cm", "Dimensions"),
            ParameterDefinition("bands", "Fascia Bands", ParamType.INT,
                                3, 1, 5, "Number of stepped fascia bands", "Detail"),
            ParameterDefinition("length", "Length", ParamType.FLOAT,
                                400, 50, 4000, "Extrusion length in cm", "Dimensions"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        height = params.get('height', 30)
        depth = params.get('depth', 20)
        bands = params.get('bands', 3)
        length = params.get('length', 400)

        # Build stepped profile
        profile = []
        band_h = height / bands
        band_d = depth / bands

        # Start from bottom-left
        profile.append((-width / 2, 0))

        for i in range(bands):
            y = i * band_h
            d = depth - i * band_d
            # Step up
            profile.append((-width / 2, y))
            profile.append((-width / 2 + d, y))
            profile.append((-width / 2 + d, y + band_h))

        # Top
        top_d = depth - (bands - 1) * band_d
        profile.append((-width / 2 + top_d, height))
        profile.append((width / 2 - top_d, height))

        # Mirror down right side
        for i in range(bands - 1, -1, -1):
            y = i * band_h
            d = depth - i * band_d
            profile.append((width / 2 - d, y + band_h))
            profile.append((width / 2 - d, y))
            profile.append((width / 2, y))

        profile.append((width / 2, 0))
        profile.append((-width / 2, 0))

        extrude_profile_linear(bm, profile, length, direction='Y',
                               offset=(0, -length / 2, 0))
