"""Barrel vault generator — semicircular tunnel vault."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear
from ...core import constants as C


@register_generator
class BarrelVaultGenerator(BaseGenerator):
    generator_id = "vault_barrel"
    generator_name = "Barrel Vault"
    category = "Vaults"
    description = "Semicircular barrel/tunnel vault"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Span Width", ParamType.FLOAT, 400, 100, 2000, "cm", "Dimensions"),
            ParameterDefinition("length", "Length", ParamType.FLOAT, 600, 100, 4000, "Vault run length", "Dimensions"),
            ParameterDefinition("thickness", "Shell Thickness", ParamType.FLOAT, C.VAULT_THICKNESS, 5, 50, "cm", "Structure"),
            ParameterDefinition("segments", "Arc Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        length = params.get('length', 600)
        thickness = params.get('thickness', C.VAULT_THICKNESS)
        segs = params.get('segments', 24)
        radius = width / 2.0

        # Outer arc
        profile = []
        for i in range(segs + 1):
            t = i / segs
            angle = math.pi * t
            x = radius * math.cos(angle)
            z = radius * math.sin(angle)
            profile.append((x, z))
        # Inner arc (reverse)
        inner_r = radius - thickness
        for i in range(segs, -1, -1):
            t = i / segs
            angle = math.pi * t
            x = inner_r * math.cos(angle)
            z = inner_r * math.sin(angle)
            profile.append((x, z))
        profile.append(profile[0])

        extrude_profile_linear(bm, profile, length, direction='Y',
                               offset=(0, -length / 2, 0))
