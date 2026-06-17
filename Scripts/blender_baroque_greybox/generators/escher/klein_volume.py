"""
Klein volume generator — figure-8 architectural space.

Creates a Klein-bottle-inspired architectural volume where interior
and exterior surfaces merge. The path follows a figure-8 rotation
creating a non-orientable architectural space.
"""

import math
import bmesh
from mathutils import Vector
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import sweep_along_curve, make_box, revolve_profile
from ....core import constants as C


@register_generator
class KleinVolumeGenerator(BaseGenerator):
    generator_id = "klein_volume"
    generator_name = "Klein Volume"
    category = "Escher"
    description = "Figure-8 non-orientable architectural volume"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("major_radius", "Major Radius", ParamType.FLOAT,
                                1000, 300, 3000, "Figure-8 major radius cm", "Dimensions"),
            ParameterDefinition("minor_radius", "Minor Radius", ParamType.FLOAT,
                                300, 100, 1000, "Cross-section radius cm", "Dimensions"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                20, 5, 60, "Shell wall thickness cm", "Dimensions"),
            ParameterDefinition("figure8_ratio", "Figure-8 Ratio", ParamType.FLOAT,
                                0.5, 0.2, 1.0, "Lobe size ratio (0.5 = symmetric)",
                                category="Shape"),
            ParameterDefinition("vertical_extent", "Vertical Extent", ParamType.FLOAT,
                                400, 100, 1200, "Vertical figure-8 extent cm",
                                category="Shape"),
            ParameterDefinition("floor_enabled", "Floor Ribbons", ParamType.BOOL,
                                True, description="Add floor surface inside tube",
                                category="Structure"),
            ParameterDefinition("railing_enabled", "Railing", ParamType.BOOL,
                                True, description="Add interior railing",
                                category="Structure"),
            ParameterDefinition("railing_height", "Railing Height", ParamType.FLOAT,
                                90, 40, 200, "Railing height cm", "Structure"),
            ParameterDefinition("columns_enabled", "Support Columns", ParamType.BOOL,
                                True, description="Add support columns at crossings",
                                category="Structure"),
            ParameterDefinition("column_radius", "Column Radius", ParamType.FLOAT,
                                15, 6, 40, "Column radius cm", "Structure"),
            ParameterDefinition("samples", "Samples", ParamType.INT,
                                96, 24, 192, "Path sampling resolution", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                12, 6, 24, "Cross-section resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        major_r = params.get('major_radius', 1000)
        minor_r = params.get('minor_radius', 300)
        wall_t = params.get('wall_thickness', 20)
        fig8_ratio = params.get('figure8_ratio', 0.5)
        vert_ext = params.get('vertical_extent', 400)
        has_floor = params.get('floor_enabled', True)
        has_railing = params.get('railing_enabled', True)
        rail_h = params.get('railing_height', 90)
        has_cols = params.get('columns_enabled', True)
        col_r = params.get('column_radius', 15)
        n_samples = params.get('samples', 96)
        n_segs = params.get('segments', 12)

        # ---- FIGURE-8 PATH ----
        # Lemniscate of Bernoulli in XZ, with vertical extent
        path_points = []
        path_tangents = []

        for i in range(n_samples):
            t = i / n_samples
            angle = t * math.pi * 2

            # Lemniscate parametric form
            # x = a * cos(t) / (1 + sin^2(t))
            # z = a * sin(t) * cos(t) / (1 + sin^2(t))
            denom = 1.0 + math.sin(angle) ** 2
            x = major_r * math.cos(angle) / denom
            z_path = major_r * math.sin(angle) * math.cos(angle) / denom

            # Add vertical figure-8 motion
            y = vert_ext * math.sin(angle * 2) * fig8_ratio

            path_points.append(Vector((x, z_path, y)))

            # Numerical tangent
            dt = 0.001
            t2 = t + dt
            angle2 = t2 * math.pi * 2
            denom2 = 1.0 + math.sin(angle2) ** 2
            x2 = major_r * math.cos(angle2) / denom2
            z2 = major_r * math.sin(angle2) * math.cos(angle2) / denom2
            y2 = vert_ext * math.sin(angle2 * 2) * fig8_ratio
            tangent = Vector((x2 - x, z2 - z_path, y2 - y)).normalized()
            path_tangents.append(tangent)

        if len(path_points) < 2:
            return

        # ---- OUTER SHELL ----
        # Circular cross-section
        outer_cs = []
        for i in range(n_segs + 1):
            angle = math.pi * 2 * i / n_segs
            x = minor_r * math.cos(angle)
            z = minor_r * math.sin(angle)
            outer_cs.append((x, z))

        # Full twist = pi (half-twist for Klein bottle effect)
        sweep_along_curve(bm, path_points, path_tangents, outer_cs,
                          twist=math.pi)

        # ---- INNER HOLLOW (wall thickness) ----
        inner_r = minor_r - wall_t
        if inner_r > 5:
            inner_cs = []
            for i in range(n_segs + 1):
                angle = math.pi * 2 * i / n_segs
                x = inner_r * math.cos(angle)
                z = inner_r * math.sin(angle)
                inner_cs.append((x, z))

            sweep_along_curve(bm, path_points, path_tangents, inner_cs,
                              twist=math.pi)

            # Connect inner and outer at intervals (structural ribs)
            rib_spacing = max(n_samples // 12, 4)
            for i in range(0, n_samples, rib_spacing):
                if i < len(path_points):
                    pos = path_points[i]
                    tan = path_tangents[min(i, len(path_tangents) - 1)]
                    # Build a connecting ring
                    for j in range(n_segs):
                        angle = math.pi * 2 * j / n_segs
                        # Outer point
                        ox = minor_r * math.cos(angle)
                        oz = minor_r * math.sin(angle)
                        # Inner point
                        ix = inner_r * math.cos(angle)
                        iz = inner_r * math.sin(angle)
                        # Simple box rib
                        make_box(bm,
                                 (pos.x + ix - 2, pos.y + iz - 2, pos.z - 2),
                                 (pos.x + ox + 2, pos.y + oz + 2, pos.z + 2))

        # ---- FLOOR RIBBON ----
        if has_floor:
            # Flat ribbon through the center of the tube
            floor_hw = minor_r * 0.6
            floor_cs = [
                (-floor_hw, -minor_r * 0.3),
                (floor_hw, -minor_r * 0.3),
                (floor_hw, -minor_r * 0.3 + 5),
                (-floor_hw, -minor_r * 0.3 + 5),
                (-floor_hw, -minor_r * 0.3),
            ]
            sweep_along_curve(bm, path_points, path_tangents, floor_cs,
                              twist=math.pi)

        # ---- RAILING ----
        if has_railing:
            rail_hw = minor_r * 0.55
            for side in [-1, 1]:
                rail_cs = [
                    (side * rail_hw, -minor_r * 0.3 + 5),
                    (side * rail_hw, -minor_r * 0.3 + 5 + rail_h),
                    (side * (rail_hw - 5), -minor_r * 0.3 + 5 + rail_h),
                    (side * (rail_hw - 5), -minor_r * 0.3 + 5),
                    (side * rail_hw, -minor_r * 0.3 + 5),
                ]
                sweep_along_curve(bm, path_points, path_tangents, rail_cs,
                                  twist=math.pi)

        # ---- SUPPORT COLUMNS at crossing ----
        if has_cols:
            # Find the crossing point (center of figure-8)
            # At t=0.5 the lemniscate passes through origin
            mid_idx = n_samples // 2
            if mid_idx < len(path_points):
                crossing = path_points[mid_idx]
                # Column down to ground
                col_profile = [(col_r, 0), (col_r, crossing.z + minor_r),
                               (col_r * 0.9, crossing.z + minor_r),
                               (col_r * 0.9, 0)]
                revolve_profile(bm, col_profile, segments=n_segs,
                                offset=(crossing.x, crossing.y, 0))

            # Additional columns at the widest points
            for frac in [0.25, 0.75]:
                idx = int(n_samples * frac)
                if idx < len(path_points):
                    pt = path_points[idx]
                    col_profile = [(col_r, 0), (col_r, pt.z + minor_r),
                                   (col_r * 0.9, pt.z + minor_r),
                                   (col_r * 0.9, 0)]
                    revolve_profile(bm, col_profile, segments=n_segs,
                                    offset=(pt.x, pt.y, 0))
