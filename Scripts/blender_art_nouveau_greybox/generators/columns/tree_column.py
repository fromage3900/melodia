"""Gaudi-style tree column: recursive L-system branching."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.bezier import CubicBezier
from ...core.mesh_builder import sweep_variable_section
from ...core.math_utils import asymmetric_offset


class LSystemBranch:
    """Represents one branch in the recursive structure."""
    def __init__(self, start: Vector, direction: Vector, radius: float, length: float, depth: int):
        self.start = start
        self.direction = direction
        self.radius = radius
        self.length = length
        self.depth = depth


@register_generator
class TreeColumnGenerator(BaseGenerator):
    generator_id = 'tree_column'
    generator_name = 'Tree Column'
    category = 'Columns'
    description = 'Gaudi-style tree column with recursive branching'

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition('trunk_height', ParamType.FLOAT, default=600, min_val=200, max_val=2000),
            ParameterDefinition('trunk_radius_bottom', ParamType.FLOAT, default=35, min_val=10, max_val=100),
            ParameterDefinition('trunk_radius_top', ParamType.FLOAT, default=12, min_val=5, max_val=50),
            ParameterDefinition('branching_depth', ParamType.INT, default=2, min_val=1, max_val=4),
            ParameterDefinition('branches_per_split', ParamType.INT, default=3, min_val=2, max_val=6),
            ParameterDefinition('spread_angle', ParamType.FLOAT, default=45, min_val=10, max_val=90),
            ParameterDefinition('asymmetry', ParamType.FLOAT, default=0.2, min_val=0, max_val=1),
            ParameterDefinition('segments', ParamType.INT, default=16, min_val=8, max_val=48),
        ]

    def _build(self, ctx: GeneratorContext, params: dict):
        bm = bmesh.new()
        trunk_h = params['trunk_height']
        r_bot = params['trunk_radius_bottom']
        r_top = params['trunk_radius_top']
        max_depth = params['branching_depth']
        n_branches = params['branches_per_split']
        spread = math.radians(params['spread_angle'])
        asym = params['asymmetry']
        segments = params['segments']

        # Trunk
        from ...core.profile_curves import organic_column_profile
        from ...core.mesh_builder import revolve_profile
        profile = organic_column_profile(r_bot, r_top, trunk_h, entasis=0.08, segments=segments)
        revolve_profile(bm, profile, segments)

        # Recursive branching
        top_pos = Vector((0, 0, trunk_h))
        branches = [LSystemBranch(top_pos, Vector((0, 0, 1)), r_top, trunk_h * 0.3, max_depth)]
        self._generate_branches(bm, branches, n_branches, spread, asym, ctx.rng, segments)
        return bm

    def _generate_branches(self, bm, branches: list[LSystemBranch], n_branches: int,
                           spread: float, asym: float, rng, segments: int):
        new_branches = []
        for branch in branches:
            if branch.depth <= 0 or branch.radius < 1:
                continue
            for i in range(n_branches):
                angle = (i / n_branches) * math.pi * 2
                if rng:
                    angle += asymmetric_offset(rng, asym * 0.5)
                # New direction
                dx = math.cos(angle) * math.sin(spread)
                dy = math.sin(angle) * math.sin(spread)
                dz = math.cos(spread)
                new_dir = Vector((dx, dy, dz)).normalized()
                new_len = branch.length * 0.7
                new_r = branch.radius * 0.65
                new_start = branch.start + branch.direction * branch.length
                new_branch = LSystemBranch(new_start, new_dir, new_r, new_len, branch.depth - 1)
                new_branches.append(new_branch)
                # Create geometry
                end = new_start + new_dir * new_len
                mid = new_start + new_dir * new_len * 0.5 + Vector((0, 0, new_len * 0.1))
                curve = CubicBezier(new_start, mid, end * 0.7 + new_start * 0.3, end)
                path = curve.sample(16)

                def section(t, r=new_r):
                    pts = []
                    for j in range(10):
                        a = (j / 10) * math.pi * 2
                        pts.append((math.cos(a) * r * (1 - t * 0.5), math.sin(a) * r * (1 - t * 0.5)))
                    return pts
                sweep_variable_section(bm, section, path, 12)
        if new_branches:
            self._generate_branches(bm, new_branches, n_branches, spread, asym, rng, segments)
