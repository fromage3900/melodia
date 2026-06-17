"""
Geometry mode switch: VALID vs IMPOSSIBLE.

VALID mode: watertight manifold mesh, cleanup, exportable.
IMPOSSIBLE mode: non-manifold allowed, self-intersections, Escher-style.
"""

import enum
import bmesh
from . import mesh_cleanup


class GeometryMode(enum.Enum):
    VALID = "valid"
    IMPOSSIBLE = "impossible"


def apply_mode(bm, mode: GeometryMode):
    """
    Apply geometry mode post-processing to a BMesh.

    VALID: full cleanup pipeline (merge verts, remove degenerate,
           recalc normals, check manifold).
    IMPOSSIBLE: minimal processing — allow non-manifold edges,
                self-intersections, overlapping geometry.
    """
    if mode == GeometryMode.VALID:
        report = mesh_cleanup.full_cleanup(bm)
        return report
    else:
        # IMPOSSIBLE mode: minimal cleanup only
        # Merge exactly overlapping verts (distance < 0.001)
        bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=0.001)
        # Remove zero-area faces
        faces_to_remove = [f for f in bm.faces if f.calc_area() < 1e-10]
        for f in faces_to_remove:
            bm.faces.remove(f)
        bm.flush()
        return {'mode': 'IMPOSSIBLE', 'is_manifold': False}


def is_valid_mode(mode) -> bool:
    """Check if mode is VALID."""
    if isinstance(mode, str):
        return mode.upper() == 'VALID'
    return mode == GeometryMode.VALID
