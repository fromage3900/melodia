"""
Penrose stairs generator — 4-flight impossible staircase loop.

Creates the classic MC Escher impossible staircase where each flight
appears to ascend yet returns to the same level. In IMPOSSIBLE mode
the geometry intentionally self-intersects.
"""

import math
import bmesh
from mathutils import Vector, Matrix
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import make_box, extrude_profile_linear
from ....core import constants as C


@register_generator
class PenroseStairsGenerator(BaseGenerator):
    generator_id = "penrose_stairs"
    generator_name = "Penrose Stairs"
    category = "Escher"
    description = "4-flight impossible staircase loop (MC Escher)"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("stair_width", "Stair Width", ParamType.FLOAT,
                                C.ESCHER_STAIR_WIDTH, 60, 400, "Staircase width cm", "Dimensions"),
            ParameterDefinition("steps_per_flight", "Steps Per Flight", ParamType.INT,
                                C.ESCHER_STEPS_PER_FLIGHT, 4, 30, "Steps in each flight", "Steps"),
            ParameterDefinition("step_height", "Step Height", ParamType.FLOAT,
                                C.ESCHER_STEP_HEIGHT, 10, 40, "Riser height cm", "Steps"),
            ParameterDefinition("step_depth", "Step Depth", ParamType.FLOAT,
                                C.ESCHER_STAIR_DEPTH, 15, 60, "Tread depth cm", "Steps"),
            ParameterDefinition("landing_size", "Landing Size", ParamType.FLOAT,
                                200, 80, 600, "Corner landing size cm", "Layout"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                20, 5, 60, "Side wall thickness cm", "Structure"),
            ParameterDefinition("column_enabled", "Columns", ParamType.BOOL,
                                True, description="Add corner columns", category="Structure"),
            ParameterDefinition("column_radius", "Column Radius", ParamType.FLOAT,
                                20, 8, 50, "Corner column radius cm", "Structure"),
            ParameterDefinition("impossible_overlap", "Impossible Overlap", ParamType.BOOL,
                                True, description="Create self-intersecting overlap (IMPOSSIBLE mode)",
                                category="Escher"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                16, 6, 32, "Column radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        sw = params.get('stair_width', C.ESCHER_STAIR_WIDTH)
        n_steps = params.get('steps_per_flight', C.ESCHER_STEPS_PER_FLIGHT)
        step_h = params.get('step_height', C.ESCHER_STEP_HEIGHT)
        step_d = params.get('step_depth', C.ESCHER_STAIR_DEPTH)
        landing = params.get('landing_size', 200)
        wall_t = params.get('wall_thickness', 20)
        has_cols = params.get('column_enabled', True)
        col_r = params.get('column_radius', 20)
        overlap = params.get('impossible_overlap', True)
        segs = params.get('segments', 16)

        flight_run = n_steps * step_d
        total_h = n_steps * step_h  # height gained per flight

        # Four corners of the square layout
        # Each flight goes from one corner to the next, ascending
        # The impossible trick: flight 4 appears to connect back to flight 1's start
        corners = [
            Vector((0, 0, 0)),                                    # SW
            Vector((flight_run + landing, 0, 0)),                 # SE
            Vector((flight_run + landing, flight_run + landing, 0)),  # NE
            Vector((0, flight_run + landing, 0)),                 # NW
        ]

        # Build each flight with progressive height
        for flight_idx in range(4):
            c0 = corners[flight_idx]
            c1 = corners[(flight_idx + 1) % 4]

            # Direction of this flight
            direction = (c1 - c0).normalized()
            # Perpendicular for width
            perp = Vector((0, 0, 1)).cross(direction)
            perp.normalize()

            base_z = flight_idx * total_h

            # Landing at start
            make_box(bm,
                     (c0.x - sw / 2, c0.y - sw / 2, base_z),
                     (c0.x + landing, c0.y + landing, base_z + wall_t))

            # Steps
            for s in range(n_steps):
                sz = base_z + s * step_h
                # Step position along direction
                sx = c0.x + direction.x * (landing + s * step_d)
                sy = c0.y + direction.y * (landing + s * step_d)

                # Tread (horizontal step)
                if abs(direction.x) > 0.5:
                    # X-dominant flight
                    make_box(bm,
                             (sx, -sw / 2, sz),
                             (sx + step_d, sw / 2, sz + step_h))
                else:
                    # Y-dominant flight
                    make_box(bm,
                             (-sw / 2, sy, sz),
                             (sw / 2, sy + step_d, sz + step_h))

            # Side walls along flight
            wall_h = total_h + step_h * 3
            if abs(direction.x) > 0.5:
                # X-dominant: walls on ±Y sides
                for side in [-1, 1]:
                    wy = side * (sw / 2 + wall_t / 2)
                    make_box(bm,
                             (c0.x + landing, wy - wall_t / 2, base_z),
                             (c1.x, wy + wall_t / 2, base_z + wall_h))
            else:
                # Y-dominant: walls on ±X sides
                for side in [-1, 1]:
                    wx = side * (sw / 2 + wall_t / 2)
                    make_box(bm,
                             (wx - wall_t / 2, c0.y + landing, base_z),
                             (wx + wall_t / 2, c1.y, base_z + wall_h))

        # Corner columns
        if has_cols:
            total_height = total_h * 4
            for i, c in enumerate(corners):
                col_z = i * total_h
                from ....core.mesh_builder import revolve_profile
                col_profile = [(col_r, 0), (col_r, total_height),
                               (col_r * 0.9, total_height), (col_r * 0.9, 0)]
                revolve_profile(bm, col_profile, segments=segs,
                                offset=(c.x, c.y, col_z))

        # Impossible overlap: in IMPOSSIBLE mode, add the connecting geometry
        # that makes the loop appear to work visually
        if overlap and ctx.mode.name == 'IMPOSSIBLE':
            # Add overlapping step sections at the closure point
            # This creates the visual illusion of the Penrose triangle
            closure_z = 3 * total_h
            overlap_len = landing * 0.6
            make_box(bm,
                     (-sw / 2, -sw / 2, closure_z),
                     (overlap_len, sw / 2, closure_z + total_h + wall_t))
