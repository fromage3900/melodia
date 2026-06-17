"""
Balustrade generator — parametric balustrade with turned/twisted/simple balusters,
rails, and newel posts. Supports linear and path-following modes.
"""

import math
import bmesh
from mathutils import Vector
from .base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ..core.mesh_builder import revolve_profile, extrude_profile_linear, make_box
from ..core.profile_curves import baluster_turned, baluster_simple, baluster_twisted
from ..core import constants as C


@register_generator
class BalustradeGenerator(BaseGenerator):
    generator_id = "balustrade"
    generator_name = "Balustrade"
    category = "Architecture"
    description = "Parametric balustrade with turned/twisted/simple balusters"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("length", "Length", ParamType.FLOAT,
                                400, 50, 4000, "Total run length cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT,
                                C.BALUSTRADE_HEIGHT, 30, 200, "Total height cm", "Dimensions"),
            ParameterDefinition("baluster_count", "Baluster Count", ParamType.INT,
                                10, 2, 50, "Number of balusters", "Layout"),
            ParameterDefinition("baluster_profile", "Baluster Profile", ParamType.ENUM,
                                "baroque_turned",
                                enum_items=[
                                    ("baroque_turned", "Baroque Turned", "Ornate stacked bulges"),
                                    ("twisted", "Twisted", "Helical spiral"),
                                    ("simple", "Simple", "Tapered cylinder"),
                                ],
                                category="Style"),
            ParameterDefinition("rail_height", "Rail Height", ParamType.FLOAT,
                                C.RAIL_HEIGHT, 3, 25, "Top/bottom rail height", "Rails"),
            ParameterDefinition("rail_depth", "Rail Depth", ParamType.FLOAT,
                                C.RAIL_DEPTH, 5, 30, "Rail depth", "Rails"),
            ParameterDefinition("newel_enabled", "Newel Posts", ParamType.BOOL,
                                True, description="Add newel posts at ends", category="Newels"),
            ParameterDefinition("newel_width", "Newel Width", ParamType.FLOAT,
                                C.NEWEL_WIDTH, 10, 50, "Newel post width", "Newels"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                16, 6, 32, "Baluster radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        length = params.get('length', 400)
        height = params.get('height', C.BALUSTRADE_HEIGHT)
        count = params.get('baluster_count', 10)
        profile_type = params.get('baluster_profile', 'baroque_turned')
        rail_h = params.get('rail_height', C.RAIL_HEIGHT)
        rail_d = params.get('rail_depth', C.RAIL_DEPTH)
        has_newel = params.get('newel_enabled', True)
        newel_w = params.get('newel_width', C.NEWEL_WIDTH)
        segs = params.get('segments', 16)

        baluster_h = height - 2 * rail_h
        spacing = length / max(count + 1, 2)

        # Bottom rail
        rail_profile = [
            (0, 0), (rail_d, 0), (rail_d, rail_h), (0, rail_h), (0, 0)
        ]
        extrude_profile_linear(bm, rail_profile, length, direction='X',
                               offset=(0, -rail_d / 2, 0))

        # Top rail
        top_rail_profile = [
            (0, height - rail_h), (rail_d, height - rail_h),
            (rail_d, height), (0, height), (0, height - rail_h)
        ]
        extrude_profile_linear(bm, top_rail_profile, length, direction='X',
                               offset=(0, -rail_d / 2, 0))

        # Balusters
        if profile_type == 'baroque_turned':
            bal_profile = baluster_turned(baluster_h, radius_max=rail_d * 0.4,
                                          radius_min=rail_d * 0.2, segments=segs * 2)
        elif profile_type == 'twisted':
            bal_profile = baluster_twisted(baluster_h, radius=rail_d * 0.3,
                                           segments=segs * 2)
        else:
            bal_profile = baluster_simple(baluster_h, radius_bottom=rail_d * 0.35,
                                          radius_top=rail_d * 0.25, segments=segs)

        for i in range(count):
            x = spacing * (i + 1)
            # Revolve baluster profile at position
            revolve_profile(bm, bal_profile, segments=segs,
                            offset=(x, 0, rail_h))

        # Newel posts
        if has_newel:
            for x_pos in [0, length]:
                make_box(bm,
                         (x_pos - newel_w / 2, -newel_w / 2, 0),
                         (x_pos + newel_w / 2, newel_w / 2, height + rail_h))
                # Newel cap
                make_box(bm,
                         (x_pos - newel_w * 0.7, -newel_w * 0.7, height + rail_h),
                         (x_pos + newel_w * 0.7, newel_w * 0.7, height + rail_h + rail_h * 0.5))
