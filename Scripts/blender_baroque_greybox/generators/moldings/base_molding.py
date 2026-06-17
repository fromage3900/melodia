"""Base molding generator — torus + scotia + fillet stack."""

import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile, extrude_profile_linear
from ...core.profile_curves import torus_profile, scotia
from ...core import constants as C


@register_generator
class BaseMoldingGenerator(BaseGenerator):
    generator_id = "molding_base"
    generator_name = "Base Molding"
    category = "Moldings"
    description = "Torus + scotia + fillet base molding stack"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT, 30, 10, 100, "Base radius cm", "Dimensions"),
            ParameterDefinition("height", "Total Height", ParamType.FLOAT, 40, 10, 120, "cm", "Dimensions"),
            ParameterDefinition("width", "Width", ParamType.FLOAT, 400, 50, 2000, "For linear extrusion", "Dimensions"),
            ParameterDefinition("length", "Length", ParamType.FLOAT, 400, 50, 4000, "Extrusion length", "Dimensions"),
            ParameterDefinition("mode", "Shape Mode", ParamType.ENUM, "linear",
                                enum_items=[("linear", "Linear", "Extruded along length"),
                                            ("radial", "Radial", "Revolved around axis")],
                                category="Shape"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 30)
        height = params.get('height', 40)
        width = params.get('width', 400)
        length = params.get('length', 400)
        mode = params.get('mode', 'linear')
        segs = params.get('segments', 24)

        # Build stacked profile: torus + scotia + torus
        t1 = torus_profile(radius * 1.05, radius * 0.18, 10)
        sc = scotia(radius * 0.9, radius * 0.15, 10)
        t2 = torus_profile(radius * 0.95, radius * 0.15, 10)

        # Stack vertically
        h1 = radius * 0.36  # torus 1 height
        h2 = radius * 0.30  # scotia height
        h3 = radius * 0.30  # torus 2 height
        # Remaining is fillet
        h_fillet = height - h1 - h2 - h3

        profile = []
        # Bottom torus
        profile.extend([(x, z) for x, z in t1])
        # Scotia (shifted up)
        profile.extend([(x, z + h1) for x, z in sc])
        # Top torus (shifted up)
        profile.extend([(x, z + h1 + h2) for x, z in t2])
        # Fillet cap
        if h_fillet > 0:
            profile.append((radius * 0.8, h1 + h2 + h3))
            profile.append((radius * 0.8, height))
            profile.append((0, height))

        if mode == 'radial':
            revolve_profile(bm, profile, segments=segs)
        else:
            # Convert to linear extrusion profile
            linear_profile = []
            for x, z in profile:
                linear_profile.append((x, z))
            # Close the profile
            linear_profile.append((linear_profile[0][0], linear_profile[0][1]))
            extrude_profile_linear(bm, linear_profile, length, direction='Y',
                                   offset=(0, -length / 2, 0))
