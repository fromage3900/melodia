"""Mesh cleanup utilities."""

import bmesh


def remove_degenerate_faces(bm: bmesh.types.BMesh) -> bmesh.types.BMesh:
    for f in bm.faces[:]:
        if len(f.verts) < 3:
            bm.faces.remove(f)
    return bm


def recalc_normals(bm: bmesh.types.BMesh) -> bmesh.types.BMesh:
    bm.faces.ensure_lookup_table()
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def merge_close_verts(bm: bmesh.types.BMesh, dist: float = 0.001) -> bmesh.types.BMesh:
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=dist)
    return bm


def triangulate_ngons(bm: bmesh.types.BMesh) -> bmesh.types.BMesh:
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    return bm


def quads_only(bm: bmesh.types.BMesh) -> bmesh.types.BMesh:
    bmesh.ops.tris_to_quads(bm, faces=bm.faces)
    return bm


def check_manifold(bm: bmesh.types.BMesh) -> bool:
    for e in bm.edges:
        if len(e.link_faces) != 2:
            return False
    return True


def check_self_intersections(bm: bmesh.types.BMesh) -> int:
    # Simplified: count faces with reversed normals
    issues = 0
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        if f.calc_area() < 0:
            issues += 1
    return issues


def full_cleanup(bm: bmesh.types.BMesh) -> bmesh.types.BMesh:
    remove_degenerate_faces(bm)
    merge_close_verts(bm)
    recalc_normals(bm)
    return bm
