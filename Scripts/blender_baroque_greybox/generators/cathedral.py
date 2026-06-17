"""
Cathedral generator — nave, aisles, transept, apse, flying buttresses.
Shape-grammar decomposition: Foundation + Walls + Roof.
Golden ratio proportions by default.
"""

import math
import bmesh
from .base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ..core.mesh_builder import make_box, extrude_profile_linear, revolve_profile
from ..core import constants as C
from ..core.golden import golden_section


@register_generator
class CathedralGenerator(BaseGenerator):
    generator_id = "cathedral"
    generator_name = "Cathedral"
    category = "Architecture"
    description = "Full baroque cathedral: nave, aisles, transept, apse, buttresses"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("nave_length", "Nave Length", ParamType.FLOAT, 2400, 400, 8000, "cm", "Nave"),
            ParameterDefinition("nave_width", "Nave Width", ParamType.FLOAT, 800, 200, 3000, "cm", "Nave"),
            ParameterDefinition("nave_height", "Nave Height", ParamType.FLOAT, 1800, 400, 5000, "cm", "Nave"),
            ParameterDefinition("aisle_count", "Aisles Per Side", ParamType.INT, 1, 0, 3, "Number of aisles each side", "Aisles"),
            ParameterDefinition("aisle_width", "Aisle Width", ParamType.FLOAT, 400, 100, 1000, "cm", "Aisles"),
            ParameterDefinition("aisle_height", "Aisle Height", ParamType.FLOAT, 1000, 300, 3000, "cm", "Aisles"),
            ParameterDefinition("transept_enabled", "Transept", ParamType.BOOL, True, "Add transept wing", "Transept"),
            ParameterDefinition("transept_width", "Transept Width", ParamType.FLOAT, 400, 100, 1200, "cm", "Transept"),
            ParameterDefinition("apse_enabled", "Apse", ParamType.BOOL, True, "Add semicircular apse", "Apse"),
            ParameterDefinition("apse_radius", "Apse Radius", ParamType.FLOAT, 400, 100, 1200, "cm", "Apse"),
            ParameterDefinition("buttresses_enabled", "Flying Buttresses", ParamType.BOOL, True, "Add flying buttresses", "Buttresses"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT, 30, 10, 80, "cm", "Structure"),
            ParameterDefinition("golden_proportions", "Golden Proportions", ParamType.BOOL, True, "Use golden ratio for proportions", "Proportions"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Curved element resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        nave_l = params.get('nave_length', 2400)
        nave_w = params.get('nave_width', 800)
        nave_h = params.get('nave_height', 1800)
        aisle_count = params.get('aisle_count', 1)
        aisle_w = params.get('aisle_width', 400)
        aisle_h = params.get('aisle_height', 1000)
        has_transept = params.get('transept_enabled', True)
        transept_w = params.get('transept_width', 400)
        has_apse = params.get('apse_enabled', True)
        apse_r = params.get('apse_radius', 400)
        has_buttresses = params.get('buttresses_enabled', True)
        wall_t = params.get('wall_thickness', 30)
        golden = params.get('golden_proportions', True)
        segs = params.get('segments', 24)

        if golden:
            # Adjust nave_h to golden ratio of nave_w
            nave_h = nave_w * C.PHI * 2.0

        # ---- FOUNDATION ----
        total_w = nave_w + 2 * aisle_count * aisle_w
        make_box(bm, (0, 0, -50), (nave_l, total_w, 0))

        # ---- NAVE WALLS ----
        # Left wall
        make_box(bm, (0, 0, 0), (nave_l, wall_t, nave_h))
        # Right wall
        make_box(bm, (0, total_w - wall_t, 0), (nave_l, total_w, nave_h))
        # Back wall (west front)
        make_box(bm, (0, 0, 0), (wall_t, total_w, nave_h))

        # ---- NAVE COLUMNS (interior colonnade) ----
        col_spacing = C.COLUMN_SPACING
        n_cols = int(nave_l / col_spacing)
        col_r = 20
        for i in range(1, n_cols):
            x = i * col_spacing
            for side in [0, 1]:
                z_base = side * (nave_w + aisle_count * aisle_w)
                # Column as cylinder approximation
                col_profile = [(col_r, 0), (col_r, nave_h * 0.85),
                               (col_r * 0.9, nave_h * 0.85),
                               (col_r * 0.9, 0)]
                revolve_profile(bm, col_profile, segments=16,
                                offset=(x, z_base + (wall_t if side == 0 else -wall_t), 0))

        # ---- AISLES ----
        for side in [0, 1]:
            for a in range(aisle_count):
                if side == 0:
                    ay = wall_t + a * aisle_w
                else:
                    ay = nave_w + wall_t + a * aisle_w - wall_t
                # Aisle wall
                make_box(bm, (0, ay, 0), (nave_l, ay + wall_t, aisle_h))
                # Aisle floor
                make_box(bm, (0, ay, 0), (nave_l, ay + aisle_w, 5))

        # ---- TRANSEPT ----
        if has_transept:
            tx = nave_l * 0.65
            # Left transept arm
            make_box(bm, (tx, 0, 0), (tx + transept_w, -transept_w, nave_h * 0.9))
            # Right transept arm
            make_box(bm, (tx, total_w, 0), (tx + transept_w, total_w + transept_w, nave_h * 0.9))
            # Crossing tower base
            make_box(bm, (tx, wall_t, 0), (tx + transept_w, nave_w + wall_t, nave_h * 1.2))

        # ---- APSE ----
        if has_apse:
            apse_cx = nave_l
            apse_cy = total_w / 2
            # Semicircular apse as revolved profile
            apse_profile = [
                (apse_r, 0), (apse_r, aisle_h),
                (apse_r - wall_t, aisle_h), (apse_r - wall_t, 0),
                (apse_r, 0),
            ]
            revolve_profile(bm, apse_profile, segments=segs,
                            angle=math.pi,
                            offset=(apse_cx, apse_cy, 0))

        # ---- FLYING BUTTRESSES ----
        if has_buttresses:
            n_buttresses = max(int(nave_l / (col_spacing * 2)), 2)
            for i in range(1, n_buttresses):
                bx = i * col_spacing * 2
                for side in [0, 1]:
                    if side == 0:
                        by = -aisle_w
                        bz = aisle_h
                        btz = nave_h * 0.7
                    else:
                        by = total_w + aisle_w
                        bz = aisle_h
                        btz = nave_h * 0.7
                    # Buttress pier
                    make_box(bm,
                             (bx - 15, by - 15, 0),
                             (bx + 15, by + 15, bz))
                    # Flying arch (simplified as angled box)
                    make_box(bm,
                             (bx - 10, by - 10, bz),
                             (bx + 10, by + (10 if side == 0 else -10), btz))
