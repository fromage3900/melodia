"""
Mesh construction utilities for Melodia Art Nouveau Greybox.

BMesh-based operations: revolve, extrude, sweep, loft, primitives.
Includes sweep_variable_section for thick-to-thin stem geometry.
"""

import math
import bmesh
from mathutils import Vector, Matrix


def create_bmesh() -> bmesh.types.BMesh:
    return bmesh.new()


# ---------------------------------------------------------------------------
# Profile operations
# ---------------------------------------------------------------------------

def revolve_profile(bm: bmesh.types.BMesh, profile: list[tuple[float, float]],
                    segments: int = 32, angle: float = math.pi * 2) -> bmesh.types.BMesh:
    """Revolve a 2D profile (r, z) around Z axis."""
    if len(profile) < 2:
        return bm
    # Create initial vertical edge loop
    verts = []
    for r, z in profile:
        if r < 0:
            r = 0
        v = bm.verts.new((r, 0.0, z))
        verts.append(v)
    bm.edges.ensure_lookup_table()
    for i in range(len(verts) - 1):
        bm.edges.new([verts[i], verts[i + 1]])
    # Spin (revolve) around Z
    geom = bm.verts[:] + bm.edges[:]
    bmesh.ops.spin(bm, geom=geom, cent=(0, 0, 0), axis=(0, 0, 1),
                   angle=angle, steps=segments, use_duplicate=False)
    return bm


def extrude_profile_linear(bm: bmesh.types.BMesh, profile: list[tuple[float, float]],
                           depth: float, segments: int = 8) -> bmesh.types.BMesh:
    """Extrude a 2D profile (x, z) along Y axis."""
    if len(profile) < 2:
        return bm
    # Create face from profile
    verts_2d = [bm.verts.new((x, 0.0, z)) for x, z in profile]
    bm.edges.ensure_lookup_table()
    for i in range(len(verts_2d)):
        bm.edges.new([verts_2d[i], verts_2d[(i + 1) % len(verts_2d)]])
    if len(verts_2d) >= 3:
        bm.faces.new(verts_2d)
    # Extrude along Y
    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    ret = bmesh.ops.extrude_face_region(bm, geom=geom)
    # Move extruded vertices
    for v in ret['geom']:
        if isinstance(v, bmesh.types.BMVert):
            v.co.y += depth
    return bm


def sweep_along_curve(bm: bmesh.types.BMesh, cross_section: list[tuple[float, float]],
                      path: list[Vector], scale_variation: float = 0.0) -> bmesh.types.BMesh:
    """
    Sweep a cross-section along a 3D path.

    Args:
        cross_section: 2D profile in local XY plane
        path: 3D path points
        scale_variation: 0-1, how much cross-section scales along path
    """
    if len(cross_section) < 3 or len(path) < 2:
        return bm

    num_segments = len(path) - 1
    verts_grid = []

    for i, pt in enumerate(path):
        # Compute orientation from path tangent
        if i < num_segments:
            direction = (path[i + 1] - path[i]).normalized()
        else:
            direction = (path[i] - path[i - 1]).normalized()
        up = Vector((0, 0, 1))
        if abs(direction.dot(up)) > 0.99:
            up = Vector((0, 1, 0))
        right = direction.cross(up).normalized()
        forward = direction
        local_up = forward.cross(right).normalized()

        # Scale variation
        t = i / max(num_segments, 1)
        scale = 1.0 - scale_variation * t

        # Place cross-section
        ring = []
        for cx, cy in cross_section:
            world_x = pt + right * (cx * scale) + local_up * (cy * scale)
            ring.append(bm.verts.new(world_x))
        verts_grid.append(ring)

    # Create faces between rings
    bm.verts.ensure_lookup_table()
    for i in range(num_segments):
        for j in range(len(cross_section)):
            j_next = (j + 1) % len(cross_section)
            v0 = verts_grid[i][j]
            v1 = verts_grid[i][j_next]
            v2 = verts_grid[i + 1][j_next]
            v3 = verts_grid[i + 1][j]
            try:
                bm.faces.new([v0, v1, v2, v3])
            except ValueError:
                pass  # Degenerate face
    return bm


def sweep_variable_section(bm: bmesh.types.BMesh,
                           cross_section_func,
                           path: list[Vector],
                           num_sections: int = 24) -> bmesh.types.BMesh:
    """
    Sweep with interpolated cross-section shapes along path.

    Args:
        cross_section_func: Callable(t) -> list[(x,y)] where t is 0-1 along path
        path: 3D path points
        num_sections: Number of cross-section rings
    """
    if len(path) < 2 or num_sections < 2:
        return bm

    verts_grid = []
    for i in range(num_sections):
        t = i / (num_sections - 1)
        # Interpolate position along path
        total_len = sum((path[j + 1] - path[j]).length for j in range(len(path) - 1))
        target_len = t * total_len
        accum = 0
        for j in range(len(path) - 1):
            seg_len = (path[j + 1] - path[j]).length
            if accum + seg_len >= target_len:
                local_t = (target_len - accum) / seg_len if seg_len > 0 else 0
                pt = path[j].lerp(path[j + 1], local_t)
                direction = (path[j + 1] - path[j]).normalized()
                break
            accum += seg_len
        else:
            pt = path[-1]
            direction = (path[-1] - path[-2]).normalized()

        # Frame
        up = Vector((0, 0, 1))
        if abs(direction.dot(up)) > 0.99:
            up = Vector((0, 1, 0))
        right = direction.cross(up).normalized()
        local_up = direction.cross(right).normalized()

        # Cross-section at this t
        section = cross_section_func(t)
        ring = []
        for cx, cy in section:
            world_x = pt + right * cx + local_up * cy
            ring.append(bm.verts.new(world_x))
        verts_grid.append(ring)

    # Create faces
    for i in range(num_sections - 1):
        ring_a = verts_grid[i]
        ring_b = verts_grid[i + 1]
        n = len(ring_a)
        for j in range(n):
            j_next = (j + 1) % n
            if j_next >= len(ring_b):
                continue
            try:
                bm.faces.new([ring_a[j], ring_a[j_next], ring_b[j_next], ring_b[j]])
            except ValueError:
                pass
    return bm


def loft(bm: bmesh.types.BMesh, rings: list[list[Vector]]) -> bmesh.types.BMesh:
    """Loft between multiple ring loops."""
    if len(rings) < 2:
        return bm
    verts_grid = []
    for ring in rings:
        verts_grid.append([bm.verts.new(v) for v in ring])
    for i in range(len(verts_grid) - 1):
        n = len(verts_grid[i])
        for j in range(n):
            j_next = (j + 1) % n
            if j_next >= len(verts_grid[i + 1]):
                continue
            try:
                bm.faces.new([verts_grid[i][j], verts_grid[i][j_next],
                              verts_grid[i + 1][j_next], verts_grid[i + 1][j]])
            except ValueError:
                pass
    return bm


# ---------------------------------------------------------------------------
# Primitives
# ---------------------------------------------------------------------------

def make_box(bm: bmesh.types.BMesh, width: float, height: float, depth: float,
             center: Vector = None) -> bmesh.types.BMesh:
    if center is None:
        center = Vector()
    hw, hh, hd = width / 2, height / 2, depth / 2
    corners = [
        Vector((-hw, -hh, -hd)), Vector((hw, -hh, -hd)),
        Vector((hw, hh, -hd)), Vector((-hw, hh, -hd)),
        Vector((-hw, -hh, hd)), Vector((hw, -hh, hd)),
        Vector((hw, hh, hd)), Vector((-hw, hh, hd)),
    ]
    verts = [bm.verts.new(center + c) for c in corners]
    faces = [
        [verts[0], verts[1], verts[2], verts[3]],  # bottom
        [verts[4], verts[7], verts[6], verts[5]],  # top
        [verts[0], verts[4], verts[5], verts[1]],  # front
        [verts[2], verts[6], verts[7], verts[3]],  # back
        [verts[0], verts[3], verts[7], verts[4]],  # left
        [verts[1], verts[5], verts[6], verts[2]],  # right
    ]
    for face in faces:
        try:
            bm.faces.new(face)
        except ValueError:
            pass
    return bm


def make_cylinder(bm: bmesh.types.BMesh, radius: float, height: float,
                  segments: int = 24) -> bmesh.types.BMesh:
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False,
                          vertices=segments, radius1=radius, radius2=radius,
                          depth=height)
    return bm


def make_arch(bm: bmesh.types.BMesh, width: float, height: float, depth: float,
              thickness: float, segments: int = 16) -> bmesh.types.BMesh:
    """Semicircular arch."""
    inner_r = width / 2
    outer_r = inner_r + thickness
    pts = []
    # Outer arc
    for i in range(segments + 1):
        a = math.pi * (1 - i / segments)
        pts.append((outer_r * math.cos(a), outer_r * math.sin(a) + height - outer_r))
    # Inner arc (reverse)
    for i in range(segments + 1):
        a = math.pi * i / segments
        pts.append((inner_r * math.cos(a), inner_r * math.sin(a) + height - inner_r))
    extrude_profile_linear(bm, pts, depth)
    return bm


# ---------------------------------------------------------------------------
# Branch joint geometry
# ---------------------------------------------------------------------------

def create_branch_joint(bm: bmesh.types.BMesh, center: Vector, radius: float,
                        branch_dirs: list[Vector], segments: int = 12) -> bmesh.types.BMesh:
    """Create geometry for a branching point (3+ tubes merging)."""
    # Main sphere at junction
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False,
                          vertices=segments, radius1=radius * 1.3, radius2=radius * 1.3,
                          depth=radius * 2)
    # Move to center
    offset = center - Vector((0, 0, 0))
    for v in bm.verts:
        v.co += offset
    return bm


# ---------------------------------------------------------------------------
# Surface displacement
# ---------------------------------------------------------------------------

def displace_surface(bm: bmesh.types.BMesh, displacements: list[tuple[float, float, float]],
                     threshold: float = 0.5) -> bmesh.types.BMesh:
    """Apply displacement to vertices matching position within threshold."""
    for dx, dy, dz in displacements:
        target = Vector((dx, dy, dz))
        for v in bm.verts:
            dist = (v.co - target).length
            if dist < threshold:
                v.co += Vector((0, 0, dz * 0.5))
    return bm
