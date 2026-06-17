"""Groin vault generator — two barrel vaults intersecting at right angles."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box, extrude_profile_linear
from ...core import constants as C


@register_generator
class GroinVaultGenerator(BaseGenerator):
    generator_id = "vault_groin"
    generator_name = "Groin Vault"
    category = "Vaults"
    description = "Cross vault from two intersecting barrel vaults"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Bay Width", ParamType.FLOAT, 400, 100, 1200, "cm", "Dimensions"),
            ParameterDefinition("height", "Crown Height", ParamType.FLOAT, 300, 100, 800, "cm", "Dimensions"),
            ParameterDefinition("thickness", "Shell Thickness", ParamType.FLOAT, C.VAULT_THICKNESS, 5, 40, "cm", "Structure"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        height = params.get('height', 300)
        thickness = params.get('thickness', C.VAULT_THICKNESS)
        segs = params.get('segments', 24)
        hw = width / 2.0

        # Build groin vault as four curved triangular surfaces
        # Each quadrant is a barrel vault clipped to a triangle
        for quad in range(4):
            qx = 1 if quad < 2 else -1
            qy = 1 if quad % 2 == 0 else -1

            # Generate surface points
            verts = []
            for i in range(segs + 1):
                row = []
                for j in range(segs + 1):
                    t_x = i / segs
                    t_y = j / segs
                    # Barrel vault in X direction
                    angle_x = math.pi * t_x
                    z_x = height * math.sin(angle_x) * 0.5
                    # Barrel vault in Y direction
                    angle_y = math.pi * t_y
                    z_y = height * math.sin(angle_y) * 0.5
                    # Groin = max of both vaults
                    z = max(z_x, z_y)
                    x = qx * hw * (2 * t_x - 1)
                    y = qy * hw * (2 * t_y - 1)
                    v = bm.verts.new((x, y, z))
                    row.append(v)
                verts.append(row)

            bm.verts.ensure_lookup_table()
            for i in range(segs):
                for j in range(segs):
                    try:
                        bm.faces.new([verts[i][j], verts[i][j+1],
                                      verts[i+1][j+1], verts[i+1][j]])
                    except ValueError:
                        pass
