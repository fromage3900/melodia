"""
Impossible bridge generator — self-connecting bridge that loops into itself.

Creates an architectural bridge whose two ends appear to connect to each
other at different heights, forming an impossible loop.
"""

import math
import bmesh
from mathutils import Vector
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import make_box, sweep_along_curve, revolve_profile
from ....core.bezier import CubicBezier
from ....core import constants as C


@register_generator
class ImpossibleBridgeGenerator(BaseGenerator):
    generator_id = "impossible_bridge"
    generator_name = "Impossible Bridge"
    category = "Escher"
    description = "Self-connecting bridge loop with impossible height transition"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("span", "Span", ParamType.FLOAT,
                                1200, 300, 4000, "Bridge span distance cm", "Dimensions"),
            ParameterDefinition("width", "Bridge Width", ParamType.FLOAT,
                                200, 80, 600, "Bridge deck width cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT,
                                600, 200, 2000, "Bridge height above ground cm", "Dimensions"),
            ParameterDefinition("deck_thickness", "Deck Thickness", ParamType.FLOAT,
                                25, 10, 60, "Deck slab thickness cm", "Dimensions"),
            ParameterDefinition("arch_rise", "Arch Rise", ParamType.FLOAT,
                                200, 50, 800, "Arch upward curve cm", "Shape"),
            ParameterDefinition("arch_count", "Arch Count", ParamType.INT,
                                3, 1, 8, "Number of support arches", "Structure"),
            ParameterDefinition("pier_width", "Pier Width", ParamType.FLOAT,
                                50, 20, 120, "Support pier width cm", "Structure"),
            ParameterDefinition("pier_depth", "Pier Depth", ParamType.FLOAT,
                                40, 15, 100, "Support pier depth cm", "Structure"),
            ParameterDefinition("parapet_enabled", "Parapet", ParamType.BOOL,
                                True, description="Add bridge parapet walls", category="Parapet"),
            ParameterDefinition("parapet_height", "Parapet Height", ParamType.FLOAT,
                                80, 30, 200, "Parapet wall height cm", "Parapet"),
            ParameterDefinition("impossible_loop", "Impossible Loop", ParamType.BOOL,
                                True, description="Create self-connecting height paradox",
                                category="Escher"),
            ParameterDefinition("samples", "Samples", ParamType.INT,
                                32, 8, 64, "Arch curve resolution", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                12, 4, 24, "Cross-section resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        span = params.get('span', 1200)
        width = params.get('width', 200)
        height = params.get('height', 600)
        deck_t = params.get('deck_thickness', 25)
        arch_rise = params.get('arch_rise', 200)
        n_arches = params.get('arch_count', 3)
        pier_w = params.get('pier_width', 50)
        pier_d = params.get('pier_depth', 40)
        has_parapet = params.get('parapet_enabled', True)
        parapet_h = params.get('parapet_height', 80)
        impossible = params.get('impossible_loop', True)
        n_samples = params.get('samples', 32)
        n_segs = params.get('segments', 12)

        hw = width / 2

        # ---- BRIDGE DECK (slightly arched) ----
        deck_points = []
        deck_tangents = []
        for i in range(n_samples + 1):
            t = i / n_samples
            x = span * t
            # Parabolic arch
            z = height + arch_rise * 4 * t * (1 - t)
            deck_points.append(Vector((x, 0, z)))
            # Tangent
            dz = arch_rise * 4 * (1 - 2 * t)
            deck_tangents.append(Vector((1, 0, dz / span)).normalized())

        # Deck cross-section
        deck_cs = [
            (-hw, -deck_t / 2), (-hw, deck_t / 2),
            (hw, deck_t / 2), (hw, -deck_t / 2),
            (-hw, -deck_t / 2),
        ]
        sweep_along_curve(bm, deck_points, deck_tangents, deck_cs)

        # ---- PARAPETS ----
        if has_parapet:
            for side in [-1, 1]:
                sy = side * hw
                parapet_points = []
                parapet_tangents = []
                for i in range(n_samples + 1):
                    t = i / n_samples
                    x = span * t
                    z = height + arch_rise * 4 * t * (1 - t) + deck_t / 2
                    parapet_points.append(Vector((x, sy, z)))
                    dz = arch_rise * 4 * (1 - 2 * t)
                    parapet_tangents.append(Vector((1, 0, dz / span)).normalized())

                parapet_cs = [
                    (0, 0), (0, parapet_h),
                    (side * 10, parapet_h), (side * 10, 0),
                    (0, 0),
                ]
                sweep_along_curve(bm, parapet_points, parapet_tangents, parapet_cs)

        # ---- SUPPORT ARCHES ----
        arch_spacing = span / (n_arches + 1)
        for a in range(n_arches):
            ax = arch_spacing * (a + 1)
            az = height + arch_rise * 4 * ((a + 1) / (n_arches + 1)) * \
                 (1 - (a + 1) / (n_arches + 1))

            # Pier
            make_box(bm,
                     (ax - pier_w / 2, -pier_d / 2, 0),
                     (ax + pier_w / 2, pier_d / 2, az - deck_t / 2))

            # Arch ribs (semicircular under the deck)
            arch_r = pier_d
            for i in range(n_segs + 1):
                t = i / n_segs
                angle = math.pi * t
                rx = ax + arch_r * math.cos(angle) * 0.3
                ry = -pier_d / 2 + pier_d * t
                rz = az - deck_t / 2 - arch_r * math.sin(angle) * 0.5
                if i < n_segs:
                    angle2 = math.pi * (i + 1) / n_segs
                    rx2 = ax + arch_r * math.cos(angle2) * 0.3
                    ry2 = -pier_d / 2 + pier_d * (i + 1) / n_segs
                    rz2 = az - deck_t / 2 - arch_r * math.sin(angle2) * 0.5
                    # Arch segment as small box
                    make_box(bm,
                             (min(rx, rx2) - 5, min(ry, ry2) - 5, min(rz, rz2) - 5),
                             (max(rx, rx2) + 5, max(ry, ry2) + 5, max(rz, rz2) + 5))

        # ---- IMPOSSIBLE LOOP ----
        if impossible and ctx.mode.name == 'IMPOSSIBLE':
            # Create a descending ramp that connects back under the bridge
            # This creates the visual paradox of the bridge looping into itself
            ramp_points = []
            ramp_tangents = []
            n_ramp = n_samples
            for i in range(n_ramp + 1):
                t = i / n_ramp
                # Ramp goes from right end, under the bridge, to left end
                x = span - span * t
                y = -hw - 100 * math.sin(t * math.pi)  # curves outward
                z = height * (1 - t) - 50  # descends
                ramp_points.append(Vector((x, y, z)))
                dx = -span
                dy = -100 * math.pi * math.cos(t * math.pi)
                dz = -height
                ramp_tangents.append(Vector((dx, dy, dz)).normalized())

            ramp_cs = [
                (-hw * 0.6, -deck_t / 2), (-hw * 0.6, deck_t / 2),
                (hw * 0.6, deck_t / 2), (hw * 0.6, -deck_t / 2),
                (-hw * 0.6, -deck_t / 2),
            ]
            sweep_along_curve(bm, ramp_points, ramp_tangents, ramp_cs)

            # Connection pillars at the paradox points
            for px in [0, span]:
                make_box(bm,
                         (px - pier_w / 2, -pier_d / 2, 0),
                         (px + pier_w / 2, pier_d / 2, height + arch_rise))
