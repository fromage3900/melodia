"""Cornice molding generator — cyma recta + corona + soffit."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core.profile_curves import cyma_recta
from ...core import constants as C


@register_generator
class CorniceGenerator(BaseGenerator):
    generator_id = "molding_cornice"
    generator_name = "Cornice"
    category = "Moldings"
    description = "Cornice profile with cyma recta, corona, and soffit"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Width", ParamType.FLOAT, 400, 50, 2000, "cm", "Dimensions"),
            ParameterDefinition("projection", "Projection", ParamType.FLOAT, 30, 5, 100, "Horizontal projection cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT, 25, 5, 80, "Vertical height cm", "Dimensions"),
            ParameterDefinition("length", "Length", ParamType.FLOAT, 400, 50, 4000, "Extrusion length", "Dimensions"),
            ParameterDefinition("segments", "Curve Segments", ParamType.INT, 16, 4, 32, "Cyma resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        proj = params.get('projection', 30)
        height = params.get('height', 25)
        length = params.get('length', 400)
        segs = params.get('segments', 16)

        # Build cornice profile: fillet + cyma recta + corona
        cyma_pts = cyma_recta(proj * 0.6, height * 0.5, segs)
        profile = [(-width / 2, 0), (width / 2, 0)]
        # Right side cyma
        profile.extend([(width / 2 + x, z) for x, z in cyma_pts])
        # Corona (vertical face at top)
        top_x = width / 2 + proj * 0.6
        profile.append((top_x, height))
        profile.append((-top_x, height))
        # Soffit (underside, return to start)
        profile.append((-width / 2, 0))

        extrude_profile_linear(bm, profile, length, direction='Y',
                               offset=(0, -length / 2, 0))
