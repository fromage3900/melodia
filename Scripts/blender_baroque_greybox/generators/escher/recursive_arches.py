"""
Recursive arches generator — golden-ratio nested arches.

Creates a series of arches within arches, each scaled by 0.618x,
producing a baroque tunnel effect that appears to recede infinitely.
"""

import math
import bmesh
from mathutils import Vector
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import make_box, make_arch, extrude_profile_linear
from ....core.profile_curves import semicircular_arch, pointed_arch, basket_arch
from ....core import constants as C


@register_generator
class RecursiveArchesGenerator(BaseGenerator):
    generator_id = "recursive_arches"
    generator_name = "Recursive Arches"
    category = "Escher"
    description = "Golden-ratio nested arches creating infinite tunnel effect"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("arch_width", "Arch Width", ParamType.FLOAT,
                                600, 200, 2000, "Outermost arch width cm", "Dimensions"),
            ParameterDefinition("arch_height", "Arch Height", ParamType.FLOAT,
                                800, 300, 3000, "Outermost arch height cm", "Dimensions"),
            ParameterDefinition("arch_depth", "Arch Depth", ParamType.FLOAT,
                                40, 10, 120, "Each arch ring depth cm", "Dimensions"),
            ParameterDefinition("arch_thickness", "Arch Thickness", ParamType.FLOAT,
                                25, 8, 80, "Arch ring thickness cm", "Dimensions"),
            ParameterDefinition("depth_levels", "Depth Levels", ParamType.INT,
                                8, 3, 20, "Number of nested arch levels", "Recursion"),
            ParameterDefinition("scale_ratio", "Scale Ratio", ParamType.FLOAT,
                                C.GOLDEN_RATIO, 0.3, 0.9,
                                "Scale factor per level (0.618 = golden)", "Recursion"),
            ParameterDefinition("arch_type", "Arch Type", ParamType.ENUM,
                                "semicircular",
                                enum_items=[
                                    ("semicircular", "Semicircular", "Roman round arch"),
                                    ("pointed", "Pointed", "Gothic pointed arch"),
                                    ("basket", "Basket-handle", "Three-centered arch"),
                                ],
                                category="Style"),
            ParameterDefinition("twist_per_level", "Twist Per Level", ParamType.FLOAT,
                                0, 0, 15, "Rotation per level (degrees)", "Escher"),
            ParameterDefinition("floor_enabled", "Floor", ParamType.BOOL,
                                True, description="Add floor slab", category="Structure"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                20, 5, 60, "Side wall thickness cm", "Structure"),
            ParameterDefinition("total_depth", "Total Depth", ParamType.FLOAT,
                                0, 100, 4000, "Total tunnel depth (0 = auto)", "Dimensions"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                16, 6, 32, "Arch curve resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        arch_w = params.get('arch_width', 600)
        arch_h = params.get('arch_height', 800)
        arch_d = params.get('arch_depth', 40)
        arch_t = params.get('arch_thickness', 25)
        n_levels = params.get('depth_levels', 8)
        ratio = params.get('scale_ratio', C.GOLDEN_RATIO)
        arch_type = params.get('arch_type', 'semicircular')
        twist = params.get('twist_per_level', 0)
        has_floor = params.get('floor_enabled', True)
        wall_t = params.get('wall_thickness', 20)
        total_depth = params.get('total_depth', 0)
        segs = params.get('segments', 16)

        if total_depth <= 0:
            total_depth = n_levels * arch_d * 1.5

        twist_rad = math.radians(twist)
        current_w = arch_w
        current_h = arch_h
        z_offset = 0

        for level in range(n_levels):
            # Current arch dimensions
            w = current_w
            h = current_h
            cx = 0  # center X
            cy = z_offset  # position along tunnel

            # Build arch ring at this level
            self._build_arch_ring(bm, cx, cy, w, h, arch_d, arch_t,
                                  arch_type, segs, twist_rad * level)

            z_offset += arch_d * 1.2  # spacing between arch rings

            # Scale down for next level
            current_w *= ratio
            current_h *= ratio

        # ---- SIDE WALLS (connecting the arches) ----
        if n_levels > 1:
            # Left wall
            make_box(bm,
                     (-arch_w / 2 - wall_t, 0, 0),
                     (-arch_w / 2, z_offset, arch_h))
            # Right wall
            make_box(bm,
                     (arch_w / 2, 0, 0),
                     (arch_w / 2 + wall_t, z_offset, arch_h))

        # ---- FLOOR ----
        if has_floor:
            make_box(bm,
                     (-arch_w / 2 - wall_t, 0, -wall_t),
                     (arch_w / 2 + wall_t, z_offset, 0))

        # ---- IMPOSSIBLE: add closing arch that connects back ----
        if ctx.mode.name == 'IMPOSSIBLE' and twist > 0:
            # Add a final arch that's rotated to create a visual paradox
            final_w = current_w
            final_h = current_h
            self._build_arch_ring(bm, 0, z_offset, final_w, final_h,
                                  arch_d, arch_t, arch_type, segs,
                                  twist_rad * n_levels + math.pi)

    def _build_arch_ring(self, bm, cx, cy, width, height, depth,
                         thickness, arch_type, segments, rotation=0):
        """Build a single arch ring at the given position."""
        hw = width / 2

        # Build arch profile based on type
        if arch_type == 'pointed':
            profile = pointed_arch(radius=hw, height=height, segments=segments)
        elif arch_type == 'basket':
            profile = basket_arch(radius=hw, rise=height * 0.7, segments=segments)
        else:
            # Semicircular
            profile = []
            r = hw
            spring_h = height - r
            # Left side up
            profile.append((-hw - thickness, 0))
            profile.append((-hw - thickness, spring_h))
            # Outer arc
            for i in range(segments + 1):
                t = i / segments
                angle = math.pi * (1.0 - t)
                x = (hw + thickness) * math.cos(angle)
                z = spring_h + (hw + thickness) * math.sin(angle)
                profile.append((x, z))
            # Right side down
            profile.append((hw + thickness, spring_h))
            profile.append((hw + thickness, 0))
            # Inner path
            profile.append((hw, 0))
            profile.append((hw, spring_h))
            for i in range(segments + 1):
                t = i / segments
                angle = math.pi * t
                x = hw * math.cos(angle)
                z = spring_h + hw * math.sin(angle)
                profile.append((x, z))
            profile.append((-hw, spring_h))
            profile.append((-hw, 0))

        if arch_type != 'semicircular':
            # For pointed/basket, create thickness by offset
            outer = [(x * (1 + thickness / hw), z * (1 + thickness / height * 0.3))
                     for x, z in profile]
            inner = list(reversed(profile))
            profile = outer + inner + [outer[0]]

        # Apply rotation
        if abs(rotation) > 1e-6:
            cos_r = math.cos(rotation)
            sin_r = math.sin(rotation)
            rotated = []
            for x, z in profile:
                rx = x * cos_r - z * sin_r
                rz = x * sin_r + z * cos_r
                rotated.append((rx + cx, rz))
            profile = rotated
        else:
            profile = [(x + cx, z) for x, z in profile]

        # Extrude along Y (tunnel depth)
        extrude_profile_linear(bm, profile, depth, direction='Y',
                               offset=(0, cy, 0))
