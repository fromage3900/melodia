"""Gaudi vault: catenary/hyperbolic paraboloid organic ceiling."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box


@register_generator
class GaudiVaultGenerator(BaseGenerator):
    generator_id = 'gaudi_vault'
    generator_name = 'Gaudi Vault'
    category = 'Vaults'
    description = 'Catenary or hyperbolic paraboloid organic ceiling'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('width', ParamType.FLOAT, default=400, min_val=100, max_val=2000),
            ParameterDefinition('length', ParamType.FLOAT, default=600, min_val=100, max_val=3000),
            ParameterDefinition('rise', ParamType.FLOAT, default=200, min_val=50, max_val=1000),
            ParameterDefinition('vault_type', ParamType.ENUM, default='catenary',
                                enum_items=[('catenary', 'Catenary', 'Inverted catenary curve'), ('hyperbolic_paraboloid', 'Hyperbolic Paraboloid', 'Bilinear warped surface'), ('warped_plane', 'Warped Plane', 'Simple curved plane')]),
            ParameterDefinition('thickness', ParamType.FLOAT, default=12, min_val=3, max_val=50),
            ParameterDefinition('ribs_enabled', ParamType.BOOL, default=True),
            ParameterDefinition('rib_width', ParamType.FLOAT, default=12, min_val=4, max_val=30),
            ParameterDefinition('rib_count', ParamType.INT, default=4, min_val=2, max_val=12),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.1, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=32, min_val=8, max_val=64),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        w = params['width']
        l = params['length']
        rise = params['rise']
        thick = params['thickness']
        seg = params['segments']
        vault_type = params['vault_type']

        # Create vertex grid for vault surface
        verts = []
        for i in range(seg + 1):
            for j in range(seg + 1):
                u = i / seg
                v = j / seg
                x = (u - 0.5) * w
                y = (v - 0.5) * l
                if vault_type == 'catenary':
                    # Catenary: z = a * cosh(x/a) - a
                    a = w / (2 * math.log(rise / w + 1)) if rise > w else w
                    z = rise - a * (math.cosh((x / a)) - 1) * math.sin(math.pi * v)
                elif vault_type == 'hyperbolic_paraboloid':
                    z = (4 * rise / (w * l)) * x * y * (1 - abs(u - 0.5) * 2) * (1 - abs(v - 0.5) * 2)
                else:  # warped_plane
                    z = rise * math.sin(math.pi * u) * math.sin(math.pi * v)
                if params['asymmetry'] > 0:
                    z += ctx.rng.uniform(-params['asymmetry'] * rise * 0.1, params['asymmetry'] * rise * 0.1)
                verts.append(bm.verts.new((x, y, max(z, 0))))

        # Create faces from grid
        bm.verts.ensure_lookup_table()
        for i in range(seg):
            for j in range(seg):
                idx = i * (seg + 1) + j
                try:
                    bm.faces.new([verts[idx], verts[idx + 1], verts[idx + seg + 2], verts[idx + seg + 1]])
                except ValueError:
                    pass

        # Ribs
        if params['ribs_enabled']:
            rib_w = params['rib_width']
            for i in range(params['rib_count']):
                t = (i + 1) / (params['rib_count'] + 1)
                y = (t - 0.5) * l
                make_box(bm, rib_w, l * 0.02, thick * 2, Vector((0, y, rise * 0.5)))

        return bm
