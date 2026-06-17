"""Acanthus leaf cluster generator — procedural leaf cluster for Corinthian capitals."""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class AcanthusGenerator(BaseGenerator):
    generator_id = "ornament_acanthus"
    generator_name = "Acanthus Leaf Cluster"
    category = "Ornaments"
    description = "Stylized acanthus leaf cluster for Corinthian capitals"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("radius", "Radius", ParamType.FLOAT, 30, 5, 100, "Base radius cm", "Dimensions"),
            ParameterDefinition("height", "Height", ParamType.FLOAT, 60, 10, 200, "Total height cm", "Dimensions"),
            ParameterDefinition("leaf_count", "Leaf Count", ParamType.INT, 8, 4, 24, "Number of leaves", "Detail"),
            ParameterDefinition("curl", "Curl Amount", ParamType.FLOAT, 0.5, 0, 1.5, "How much leaves curl outward", "Shape"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        radius = params.get('radius', 30)
        height = params.get('height', 60)
        leaf_count = params.get('leaf_count', 8)
        curl = params.get('curl', 0.5)
        segs = params.get('segments', 24)

        # Build a central stem
        stem_profile = [
            (radius * 0.15, 0),
            (radius * 0.12, height * 0.3),
            (radius * 0.08, height * 0.7),
            (radius * 0.05, height),
        ]
        revolve_profile(bm, stem_profile, segments=segs)

        # Build leaves as revolved profiles around the stem
        for i in range(leaf_count):
            angle = (i / leaf_count) * math.pi * 2.0
            cos_a = math.cos(angle)
            sin_a = math.sin(angle)

            # Each leaf: a curved protrusion from the stem
            n_pts = 12
            leaf_pts = []
            for j in range(n_pts + 1):
                t = j / n_pts
                # Leaf shape: width peaks at 0.4, length goes to height
                leaf_width = radius * 0.5 * math.sin(math.pi * t) * (1.0 + curl * t)
                leaf_z = height * 0.1 + height * 0.8 * t
                leaf_r = radius * 0.2 + leaf_width
                leaf_pts.append((leaf_r, leaf_z))

            # Add tip
            leaf_pts.append((radius * 0.15, height * 0.95))

            # Revolve this leaf
            revolve_profile(bm, leaf_pts, segments=max(segs // 2, 8))
