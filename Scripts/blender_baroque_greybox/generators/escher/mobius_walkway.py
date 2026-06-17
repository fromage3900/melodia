"""
Mobius walkway generator — half-twist continuous surface.

Creates a Mobius strip architectural walkway with baroque ornamentation.
The surface has only one side and one edge — an impossible architectural space.
"""

import math
import bmesh
from mathutils import Vector
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import sweep_along_curve, make_box
from ....core import constants as C


@register_generator
class MobiusWalkwayGenerator(BaseGenerator):
    generator_id = "mobius_walkway"
    generator_name = "Mobius Walkway"
    category = "Escher"
    description = "Half-twist continuous walkway (Mobius strip)"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT,
                                800, 200, 3000, "Center-line radius cm", "Dimensions"),
            ParameterDefinition("width", "Walkway Width", ParamType.FLOAT,
                                200, 60, 600, "Walkway surface width cm", "Dimensions"),
            ParameterDefinition("thickness", "Thickness", ParamType.FLOAT,
                                20, 5, 60, "Walkway slab thickness cm", "Dimensions"),
            ParameterDefinition("twist", "Twist", ParamType.FLOAT,
                                1.0, 0.5, 3.5, "Number of half-twists (odd = Mobius)",
                                category="Escher"),
            ParameterDefinition("railing_enabled", "Railing", ParamType.BOOL,
                                True, description="Add railings on edges", category="Railings"),
            ParameterDefinition("railing_height", "Railing Height", ParamType.FLOAT,
                                100, 40, 200, "Railing height cm", "Railings"),
            ParameterDefinition("railing_thickness", "Railing Thickness", ParamType.FLOAT,
                                8, 3, 20, "Railing wall thickness cm", "Railings"),
            ParameterDefinition("columns_enabled", "Support Columns", ParamType.BOOL,
                                True, description="Add support columns around ring",
                                category="Structure"),
            ParameterDefinition("column_count", "Column Count", ParamType.INT,
                                8, 4, 24, "Number of support columns", "Structure"),
            ParameterDefinition("column_radius", "Column Radius", ParamType.FLOAT,
                                15, 6, 40, "Column radius cm", "Structure"),
            ParameterDefinition("samples", "Samples", ParamType.INT,
                                64, 16, 128, "Ring sampling resolution", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                8, 4, 24, "Cross-section resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 800)
        width = params.get('width', 200)
        thickness = params.get('thickness', 20)
        n_twists = params.get('twist', 1.0)
        has_railing = params.get('railing_enabled', True)
        rail_h = params.get('railing_height', 100)
        rail_t = params.get('railing_thickness', 8)
        has_cols = params.get('columns_enabled', True)
        n_cols = params.get('column_count', 8)
        col_r = params.get('column_radius', 15)
        n_samples = params.get('samples', 64)
        n_segs = params.get('segments', 8)

        # ---- CENTER-LINE PATH (circle in XY plane) ----
        path_points = []
        path_tangents = []
        for i in range(n_samples):
            t = i / n_samples
            angle = t * math.pi * 2
            x = radius * math.cos(angle)
            y = radius * math.sin(angle)
            z = 0
            path_points.append(Vector((x, y, z)))
            # Tangent
            tx = -radius * math.sin(angle)
            ty = radius * math.cos(angle)
            path_tangents.append(Vector((tx, ty, 0)).normalized())

        # ---- CROSS-SECTION (flat slab with optional railings) ----
        hw = width / 2
        cross_section = [
            (-hw, -thickness / 2),
            (-hw, thickness / 2),
            (hw, thickness / 2),
            (hw, -thickness / 2),
            (-hw, -thickness / 2),
        ]

        # Total twist = n_twists * pi (half-twists)
        total_twist = n_twists * math.pi

        # ---- SWEEP THE WALKWAY ----
        sweep_along_curve(bm, path_points, path_tangents, cross_section,
                          twist=total_twist)

        # ---- RAILINGS ----
        if has_railing:
            # Left edge railing
            left_cs = [
                (-hw, thickness / 2),
                (-hw, thickness / 2 + rail_h),
                (-hw + rail_t, thickness / 2 + rail_h),
                (-hw + rail_t, thickness / 2),
                (-hw, thickness / 2),
            ]
            sweep_along_curve(bm, path_points, path_tangents, left_cs,
                              twist=total_twist)

            # Right edge railing
            right_cs = [
                (hw - rail_t, thickness / 2),
                (hw - rail_t, thickness / 2 + rail_h),
                (hw, thickness / 2 + rail_h),
                (hw, thickness / 2),
                (hw - rail_t, thickness / 2),
            ]
            sweep_along_curve(bm, path_points, path_tangents, right_cs,
                              twist=total_twist)

        # ---- SUPPORT COLUMNS ----
        if has_cols:
            from ....core.mesh_builder import revolve_profile
            for i in range(n_cols):
                angle = (i / n_cols) * math.pi * 2
                cx = radius * math.cos(angle)
                cy = radius * math.sin(angle)
                # Column from ground to walkway
                col_profile = [(col_r, 0), (col_r, -radius * 0.3),
                               (col_r * 0.9, -radius * 0.3), (col_r * 0.9, 0)]
                revolve_profile(bm, col_profile, segments=n_segs,
                                offset=(cx, cy, -thickness))
