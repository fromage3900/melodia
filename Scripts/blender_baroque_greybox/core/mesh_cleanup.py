"""
Mesh cleanup utilities for VALID mode enforcement.
"""

import bmesh
from mathutils import Vector


def remove_degenerate_faces(bm, min_area=1e-6):
    """Remove faces with area below threshold."""
    faces_to_remove = []
    for f in bm.faces:
        if f.calc_area() < min_area:
            faces_to_remove.append(f)
    for f in faces_to_remove:
        bm.faces.remove(f)
    bm.flush()


def recalc_normals(bm, outside=True):
    """Recalculate all face normals to point outward."""
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])


def merge_close_verts(bm, threshold=0.01):
    """Merge vertices closer than threshold distance."""
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=threshold)


def triangulate_ngons(bm):
    """Triangulate faces with more than 4 vertices for safety."""
    ngons = [f for f in bm.faces if len(f.verts) > 4]
    if ngons:
        bmesh.ops.triangulate(bm, faces=ngons, quad_method='FIXED',
                              ngon_method='BEAUTY')


def quads_only(bm):
    """Convert all faces to quads or tris (no ngons)."""
    ngons = [f for f in bm.faces if len(f.verts) > 4]
    if ngons:
        bmesh.ops.triangulate(bm, faces=ngons, quad_method='FIXED',
                              ngon_method='BEAUTY')
        # Then try to re-join tris into quads where possible
        bmesh.ops.join_triangles(bm, faces=bm.faces[:],
                                 angle_face_threshold=0.1,
                                 angle_shape_threshold=0.1)


def check_manifold(bm):
    """
    Check if the mesh is manifold (watertight).
    Returns (is_manifold: bool, problem_edges: list).
    """
    problem_edges = []
    for e in bm.edges:
        n_faces = len(e.link_faces)
        if n_faces != 2:
            problem_edges.append(e)
    return (len(problem_edges) == 0, problem_edges)


def check_self_intersections(bm):
    """
    Basic self-intersection check using BVH tree.
    Returns True if likely self-intersecting.
    """
    import mathutils
    if len(bm.faces) < 2:
        return False

    # Create temporary mesh for BVH
    temp_mesh = bmesh.new()
    temp_mesh.from_bmesh(bm)

    tree = mathutils.bvhtree.BVHTree.FromBMesh(temp_mesh)
    overlaps = tree.overlap(tree)

    temp_mesh.free()
    return len(overlaps) > 0


def full_cleanup(bm):
    """
    Full cleanup pipeline for VALID mode.
    Returns cleanup report dict.
    """
    report = {}

    # Merge close vertices
    before = len(bm.verts)
    merge_close_verts(bm)
    report['merged_verts'] = before - len(bm.verts)

    # Remove degenerate faces
    before = len(bm.faces)
    remove_degenerate_faces(bm)
    report['removed_faces'] = before - len(bm.faces)

    # Recalculate normals
    recalc_normals(bm)

    # Check manifold
    is_manifold, problems = check_manifold(bm)
    report['is_manifold'] = is_manifold
    report['problem_edges'] = len(problems)

    return report
