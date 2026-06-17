"""Rose window generator — radial tracery with cusped arches."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class RoseWindowGenerator(BaseGenerator):
    generator_id = "ornament_rose_window"
    generator_name = "Rose Window"
    category = "Ornaments"
    description = "Radial tracery rose window with cusped arches"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT, 150, 30, 500, "Window radius cm", "Dimensions"),
            ParameterDefinition("depth", "Depth", ParamType.FLOAT, 20, 5, 60, "Frame depth cm", "Dimensions"),
            ParameterDefinition("petals", "Petals/Spokes", ParamType.INT, 12, 4, 24, "Radial symmetry count", "Detail"),
            ParameterDefinition("inner_radius", "Inner Ring Radius", ParamType.FLOAT, 40, 10, 200, "Central circle radius", "Shape"),
            ParameterDefinition("rings", "Tracery Rings", ParamType.INT, 2, 1, 4, "Number of concentric rings", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 32, 8, 64, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 150)
        depth = params.get('depth', 20)
        petals = params.get('petals', 12)
        inner_r = params.get('inner_radius', 40)
        rings = params.get('rings', 2)
        segs = params.get('segments', 32)

        # Outer ring (frame)
        frame_thickness = radius * 0.08
        outer_profile = [
            (radius - frame_thickness, 0),
            (radius, 0),
            (radius, depth),
            (radius - frame_thickness, depth),
            (radius - frame_thickness, 0),
        ]
        revolve_profile(bm, outer_profile, segments=segs)

        # Inner circle (oculus)
        oculus_profile = [
            (0, 0),
            (inner_r, 0),
            (inner_r, depth * 0.5),
            (inner_r * 0.8, depth * 0.5),
            (inner_r * 0.8, depth * 0.3),
            (0, depth * 0.3),
        ]
        revolve_profile(bm, oculus_profile, segments=segs)

        # Radial spokes (mullions)
        for i in range(petals):
            angle = (i / petals) * math.pi * 2.0
            cos_a = math.cos(angle)
            sin_a = math.sin(angle)

            # Spoke from inner to outer
            spoke_width = radius * 0.03
            spoke_profile = []
            n_pts = 8
            for j in range(n_pts + 1):
                t = j / n_pts
                r = inner_r + (radius - frame_thickness - inner_r) * t
                # Slight taper
                w = spoke_width * (1.0 - 0.3 * t)
                spoke_profile.append((r, depth * 0.15 - w))
                spoke_profile.append((r, depth * 0.15 + w))

            # Cusped arch between spokes
            if i < petals:
                arch_r = (radius - frame_thickness - inner_r) * 0.3
                mid_r = inner_r + (radius - frame_thickness - inner_r) * 0.5
                arch_profile = [
                    (mid_r - arch_r * 0.5, depth * 0.4),
                    (mid_r, depth * 0.4 + arch_r),
                    (mid_r + arch_r * 0.5, depth * 0.4),
                ]
                revolve_profile(bm, arch_profile, segments=max(segs // 4, 6))

        # Concentric tracery rings
        for r_idx in range(1, rings + 1):
            ring_r = inner_r + (radius - frame_thickness - inner_r) * (r_idx / (rings + 1))
            ring_thickness = radius * 0.02
            ring_profile = [
                (ring_r - ring_thickness, depth * 0.2),
                (ring_r + ring_thickness, depth * 0.2),
                (ring_r + ring_thickness, depth * 0.6),
                (ring_r - ring_thickness, depth * 0.6),
                (ring_r - ring_thickness, depth * 0.2),
            ]
            revolve_profile(bm, ring_profile, segments=segs)
