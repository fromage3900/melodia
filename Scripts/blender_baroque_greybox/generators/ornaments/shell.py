"""Shell/conch motif generator — radiating ribs from a central point."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class ShellGenerator(BaseGenerator):
    generator_id = "ornament_shell"
    generator_name = "Shell Motif"
    category = "Ornaments"
    description = "Radiating rib conch/shell ornament"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT, 40, 10, 150, "cm", "Dimensions"),
            ParameterDefinition("depth", "Depth", ParamType.FLOAT, 15, 3, 50, "Projection depth cm", "Dimensions"),
            ParameterDefinition("ribs", "Rib Count", ParamType.INT, 12, 4, 32, "Number of radiating ribs", "Detail"),
            ParameterDefinition("fan_angle", "Fan Angle", ParamType.FLOAT, 180, 60, 360, "Degrees of fan spread", "Shape"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 40)
        depth = params.get('depth', 15)
        ribs = params.get('ribs', 12)
        fan_angle_deg = params.get('fan_angle', 180)
        segs = params.get('segments', 24)

        fan_rad = math.radians(fan_angle_deg)
        start_angle = (math.pi * 2.0 - fan_rad) / 2.0

        # Shell base (fan shape)
        profile = [(0, 0)]
        n = segs
        for i in range(n + 1):
            t = i / n
            angle = start_angle + fan_rad * t
            x = radius * math.cos(angle)
            z = radius * math.sin(angle) * 0.3  # Flatten
            profile.append((x, z))
        profile.append((0, 0))

        # Create base fan
        revolve_profile(bm, profile, segments=max(segs // 4, 8))

        # Add radiating ribs
        rib_width = radius * 0.04
        for i in range(ribs):
            t = (i + 0.5) / ribs
            angle = start_angle + fan_rad * t
            cos_a = math.cos(angle)
            sin_a = math.sin(angle)

            # Each rib is a small protruding bar
            rib_profile = []
            rib_n = 8
            for j in range(rib_n + 1):
                rt = j / rib_n
                r = radius * 0.2 + radius * 0.75 * rt
                w = rib_width * (1.0 - 0.5 * rt)
                d = depth * 0.3 * math.sin(rt * math.pi)
                rib_profile.append((r, d - w))
                rib_profile.append((r, d + w))

            if rib_profile:
                revolve_profile(bm, rib_profile, segments=max(segs // 6, 4))
