"""Surface shared utilities."""

import bmesh
from mathutils import Vector
from ...core.mesh_builder import make_box


def build_frame(bm: bmesh.types.BMesh, width: float, height: float, depth: float,
                frame_width: float, segments: int = 8) -> bmesh.types.BMesh:
    """Rectangular frame for panels."""
    hw, hh = width / 2, height / 2
    fw = frame_width / 2
    # Top
    make_box(bm, width, frame_width, depth, Vector((0, hh - fw, 0)))
    # Bottom
    make_box(bm, width, frame_width, depth, Vector((0, -hh + fw, 0)))
    # Left
    make_box(bm, frame_width, height - frame_width * 2, depth, Vector((-hw + fw, 0, 0)))
    # Right
    make_box(bm, frame_width, height - frame_width * 2, depth, Vector((hw - fw, 0, 0)))
    return bm
