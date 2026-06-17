"""Ribbed vault generator — groin vault with raised diagonal ribs."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import extrude_profile_linear, make_box
from ...core import constants as C


@register_generator
class RibbedVaultGenerator(BaseGenerator):
    generator_id = "vault_ribbed"
    generator_name = "Ribbed Vault"
    category = "Vaults"
    description = "Groin vault with raised diagonal and transverse ribs"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Bay Width", ParamType.FLOAT, 400, 100, 1200, "cm", "Dimensions"),
            ParameterDefinition("height", "Crown Height", ParamType.FLOAT, 300, 100, 800, "cm", "Dimensions"),
            ParameterDefinition("rib_width", "Rib Width", ParamType.FLOAT, C.VAULT_RIB_WIDTH, 5, 30, "cm", "Ribs"),
            ParameterDefinition("rib_depth", "Rib Depth", ParamType.FLOAT, C.VAULT_RIB_DEPTH, 5, 50, "cm", "Ribs"),
            ParameterDefinition("diagonal_ribs", "Diagonal Ribs", ParamType.BOOL,
                                True, description="Add diagonal groin ribs", category="Ribs"),
            ParameterDefinition("transverse_ribs", "Transverse Ribs", ParamType.BOOL,
                                True, description="Add transverse arch ribs", category="Ribs"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 400)
        height = params.get('height', 300)
        rib_w = params.get('rib_width', C.VAULT_RIB_WIDTH)
        rib_d = params.get('rib_depth', C.VAULT_RIB_DEPTH)
        has_diag = params.get('diagonal_ribs', True)
        has_trans = params.get('transverse_ribs', True)
        segs = params.get('segments', 24)
        hw = width / 2.0

        # Vault shell (simplified)
        verts = []
        for i in range(segs + 1):
            row = []
            for j in range(segs + 1):
                t_x = i / segs
                t_y = j / segs
                z_x = height * math.sin(math.pi * t_x) * 0.5
                z_y = height * math.sin(math.pi * t_y) * 0.5
                z = max(z_x, z_y)
                x = hw * (2 * t_x - 1)
                y = hw * (2 * t_y - 1)
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

        # Diagonal ribs (corner to corner arches)
        if has_diag:
            for diag in range(2):
                for i in range(segs + 1):
                    t = i / segs
                    if diag == 0:
                        x = hw * (2 * t - 1)
                        y = hw * (2 * t - 1)
                    else:
                        x = hw * (2 * t - 1)
                        y = -hw * (2 * t - 1)
                    z = height * math.sin(math.pi * t)
                    # Rib as box segments
                    if i < segs:
                        t2 = (i + 1) / segs
                        if diag == 0:
                            x2 = hw * (2 * t2 - 1)
                            y2 = hw * (2 * t2 - 1)
                        else:
                            x2 = hw * (2 * t2 - 1)
                            y2 = -hw * (2 * t2 - 1)
                        z2 = height * math.sin(math.pi * t2)
                        make_box(bm,
                                 (min(x, x2) - rib_w/2, min(y, y2) - rib_w/2, z - rib_d),
                                 (max(x, x2) + rib_w/2, max(y, y2) + rib_w/2, z + rib_w/2))

        # Transverse ribs (along edges)
        if has_trans:
            for edge in range(4):
                for i in range(segs + 1):
                    t = i / segs
                    if edge == 0:
                        x, y = hw * (2*t - 1), -hw
                    elif edge == 1:
                        x, y = hw * (2*t - 1), hw
                    elif edge == 2:
                        x, y = -hw, hw * (2*t - 1)
                    else:
                        x, y = hw, hw * (2*t - 1)
                    z = height * math.sin(math.pi * t) * 0.5
                    make_box(bm,
                             (x - rib_w/2, y - rib_w/2, z - rib_d/2),
                             (x + rib_w/2, y + rib_w/2, z + rib_w/2))
