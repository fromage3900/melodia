"""Coffered ceiling generator — recessed square/octagonal panel pattern."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import make_box
from ...core import constants as C


@register_generator
class CofferedGenerator(BaseGenerator):
    generator_id = "vault_coffered"
    generator_name = "Coffered Ceiling"
    category = "Vaults"
    description = "Recessed square or octagonal coffer pattern"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("width", "Total Width", ParamType.FLOAT, 600, 200, 2000, "cm", "Dimensions"),
            ParameterDefinition("depth", "Total Depth", ParamType.FLOAT, 600, 200, 2000, "cm", "Dimensions"),
            ParameterDefinition("grid_x", "Grid Columns", ParamType.INT, 4, 2, 12, "Coffered grid X", "Layout"),
            ParameterDefinition("grid_y", "Grid Rows", ParamType.INT, 4, 2, 12, "Coffered grid Y", "Layout"),
            ParameterDefinition("coffer_depth", "Coffer Recess Depth", ParamType.FLOAT, 20, 5, 60, "How deep each coffer is", "Detail"),
            ParameterDefinition("frame_width", "Frame Width", ParamType.FLOAT, 10, 3, 30, "Width of frame between coffers", "Detail"),
            ParameterDefinition("shape", "Coffer Shape", ParamType.ENUM, "square",
                                enum_items=[("square", "Square", "Square coffers"),
                                            ("octagon", "Octagonal", "Octagonal coffers")],
                                category="Shape"),
        ]

    def _build(self, bm, params, ctx):
        width = params.get('width', 600)
        depth = params.get('depth', 600)
        grid_x = params.get('grid_x', 4)
        grid_y = params.get('grid_y', 4)
        coffer_d = params.get('coffer_depth', 20)
        frame_w = params.get('frame_width', 10)
        shape = params.get('shape', 'square')

        cell_w = width / grid_x
        cell_d = depth / grid_y
        base_thickness = 10  # ceiling slab thickness

        # Base slab
        make_box(bm, (0, 0, 0), (width, depth, base_thickness))

        # Coffer recesses
        for ix in range(grid_x):
            for iy in range(grid_y):
                x0 = ix * cell_w + frame_w
                y0 = iy * cell_d + frame_w
                x1 = (ix + 1) * cell_w - frame_w
                y1 = (iy + 1) * cell_d - frame_w

                if shape == 'square':
                    # Square recess
                    make_box(bm,
                             (x0, y0, base_thickness - coffer_d),
                             (x1, y1, base_thickness + 1))
                else:
                    # Octagonal: square with chamfered corners
                    chamfer = min(cell_w, cell_d) * 0.1
                    # Center square
                    make_box(bm,
                             (x0 + chamfer, y0, base_thickness - coffer_d),
                             (x1 - chamfer, y1, base_thickness + 1))
                    make_box(bm,
                             (x0, y0 + chamfer, base_thickness - coffer_d),
                             (x1, y1 - chamfer, base_thickness + 1))
                    # Corner cuts (small boxes to remove)
                    for cx, cy in [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]:
                        make_box(bm,
                                 (min(cx, x0 + chamfer), min(cy, y0 + chamfer),
                                  base_thickness - coffer_d),
                                 (max(cx, x1 - chamfer), max(cy, y1 - chamfer),
                                  base_thickness + 1))
