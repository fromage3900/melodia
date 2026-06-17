"""UV mapping utilities."""

import bmesh
from mathutils import Vector


def auto_uv(bm: bmesh.types.BMesh, mapping: str = 'default') -> bmesh.types.BMesh:
    """Simple auto-UV based on projection type."""
    if not bm.faces:
        return bm
    bm.faces.ensure_lookup_table()
    uv_layer = bm.loops.layers.uv.verify()
    for face in bm.faces:
        for loop in face.loops:
            uv = loop[uv_layer].uv
            if mapping in ('column', 'stem_column'):
                uv[0] = loop.vert.co.xy.angle / 6.2832
                uv[1] = loop.vert.co.z / 600.0
            elif mapping in ('wall',):
                uv[0] = loop.vert.co.x / 400.0
                uv[1] = loop.vert.co.z / 600.0
            elif mapping in ('arch', 'doorway'):
                uv[0] = loop.vert.co.x / 200.0
                uv[1] = loop.vert.co.z / 350.0
            elif mapping in ('ornament',):
                uv[0] = loop.vert.co.x / 100.0
                uv[1] = loop.vert.co.z / 100.0
            elif mapping in ('vault',):
                uv[0] = loop.vert.co.x / 400.0
                uv[1] = loop.vert.co.y / 600.0
            elif mapping in ('floor', 'mosaic'):
                uv[0] = loop.vert.co.x / 400.0
                uv[1] = loop.vert.co.y / 400.0
            elif mapping in ('railing',):
                uv[0] = loop.vert.co.x / 400.0
                uv[1] = loop.vert.co.z / 100.0
            else:
                uv[0] = loop.vert.co.x
                uv[1] = loop.vert.co.y
    return bm
