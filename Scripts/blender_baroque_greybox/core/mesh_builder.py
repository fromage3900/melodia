"""
BMesh-based mesh construction utilities.

Provides high-level operations for architectural geometry:
extrude_profile, revolve_profile, sweep_along_curve, loft, fillet_edge.
All geometry is built via bmesh for robust topology.
"""

import math
import bmesh
from mathutils import Vector, Matrix


def create_bmesh():
    """Create a new empty BMesh."""
    return bmesh.new()


def bmesh_to_mesh(bm, name="GeneratedMesh"):
    """Convert BMesh to a Blender Mesh data-block."""
    import bpy
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    mesh.update()
    return mesh


def create_object_from_bmesh(bm, name="GeneratedMesh", collection=None):
    """Create a Blender Object from a BMesh."""
    import bpy
    mesh = bmesh_to_mesh(bm, name)
    obj = bpy.data.objects.new(name, mesh)
    if collection is None:
        collection = bpy.context.scene.collection
    collection.objects.link(obj)
    return obj


# ---------------------------------------------------------------------------
# Profile revolution (lathe)
# ---------------------------------------------------------------------------

def revolve_profile(bm, profile_2d, segments=32, angle=math.pi * 2.0,
                    axis='Y', offset=(0, 0, 0)):
    """
    Revolve a 2D profile (list of (x, z) tuples) around the Y axis.
    Creates a lathe mesh. Profile x = radius, z = height.

    Args:
        bm: BMesh to write into
        profile_2d: list of (radius, height) tuples
        segments: number of revolution segments
        angle: total revolution angle (default full circle)
        axis: rotation axis ('Y' for lathe, 'Z' for turntable)
        offset: (x, y, z) offset for the center
    """
    if not profile_2d or len(profile_2d) < 2:
        return

    n_profile = len(profile_2d)
    verts_grid = []

    for i in range(segments + 1):
        t = i / segments
        a = angle * t
        cos_a = math.cos(a)
        sin_a = math.sin(a)
        ring = []
        for (r, h) in profile_2d:
            if axis == 'Y':
                x = r * cos_a + offset[0]
                y = h + offset[1]
                z = r * sin_a + offset[2]
            else:  # Z axis
                x = r * cos_a + offset[0]
                y = r * sin_a + offset[1]
                z = h + offset[2]
            v = bm.verts.new((x, y, z))
            ring.append(v)
        verts_grid.append(ring)

    bm.verts.ensure_lookup_table()

    # Create faces between rings
    for i in range(len(verts_grid) - 1):
        for j in range(n_profile - 1):
            v0 = verts_grid[i][j]
            v1 = verts_grid[i][j + 1]
            v2 = verts_grid[i + 1][j + 1]
            v3 = verts_grid[i + 1][j]
            try:
                bm.faces.new([v0, v1, v2, v3])
            except ValueError:
                pass  # skip degenerate faces

    # Cap ends if not full revolution
    if angle < math.pi * 2.0 - 1e-6:
        # Start cap
        start_ring = verts_grid[0]
        if len(start_ring) >= 3:
            try:
                bm.faces.new(start_ring)
            except ValueError:
                pass
        # End cap
        end_ring = verts_grid[-1]
        if len(end_ring) >= 3:
            try:
                bm.faces.new(list(reversed(end_ring)))
            except ValueError:
                pass


# ---------------------------------------------------------------------------
# Linear extrusion
# ---------------------------------------------------------------------------

def extrude_profile_linear(bm, profile_2d, length, direction='Y',
                           offset=(0, 0, 0)):
    """
    Extrude a 2D profile linearly along an axis.

    Args:
        bm: BMesh to write into
        profile_2d: list of (x, z) tuples defining the cross-section
        length: extrusion distance
        direction: extrusion axis ('X', 'Y', or 'Z')
        offset: base offset (x, y, z)
    """
    if not profile_2d or len(profile_2d) < 2:
        return

    n = len(profile_2d)
    dir_vec = {'X': Vector((1, 0, 0)), 'Y': Vector((0, 1, 0)),
               'Z': Vector((0, 0, 1))}[direction]

    # Bottom ring
    bottom = []
    for (x, z) in profile_2d:
        if direction == 'Y':
            v = bm.verts.new((x + offset[0], offset[1], z + offset[2]))
        elif direction == 'X':
            v = bm.verts.new((offset[0], x + offset[1], z + offset[2]))
        else:
            v = bm.verts.new((x + offset[0], z + offset[1], offset[2]))
        bottom.append(v)

    # Top ring
    top = []
    disp = dir_vec * length
    for (x, z) in profile_2d:
        if direction == 'Y':
            v = bm.verts.new((x + offset[0] + disp.x,
                              offset[1] + disp.y,
                              z + offset[2] + disp.z))
        elif direction == 'X':
            v = bm.verts.new((offset[0] + disp.x,
                              x + offset[1] + disp.y,
                              z + offset[2] + disp.z))
        else:
            v = bm.verts.new((x + offset[0] + disp.x,
                              z + offset[1] + disp.y,
                              offset[2] + disp.z))
        top.append(v)

    bm.verts.ensure_lookup_table()

    # Side faces
    for i in range(n - 1):
        try:
            bm.faces.new([bottom[i], bottom[i + 1], top[i + 1], top[i]])
        except ValueError:
            pass

    # Cap faces
    if n >= 3:
        try:
            bm.faces.new(list(reversed(bottom)))
        except ValueError:
            pass
        try:
            bm.faces.new(top)
        except ValueError:
            pass


# ---------------------------------------------------------------------------
# Sweep along curve
# ---------------------------------------------------------------------------

def sweep_along_curve(bm, curve_points, curve_tangents, cross_section,
                      twist=0.0, scale_variation=None):
    """
    Sweep a 2D cross-section along a 3D curve.

    Args:
        bm: BMesh to write into
        curve_points: list of Vector positions along the curve
        curve_tangents: list of Vector tangents (normalized)
        cross_section: list of (x, z) tuples (profile in local frame)
        twist: total twist angle in radians over the full path
        scale_variation: optional list of scale factors per curve point
    """
    if len(curve_points) < 2 or not cross_section:
        return

    n_cs = len(cross_section)
    n_path = len(curve_points)
    verts_grid = []

    up = Vector((0, 1, 0))

    for i in range(n_path):
        pos = curve_points[i]
        tan = curve_tangents[i] if i < len(curve_tangents) else Vector((0, 1, 0))

        # Build rotation matrix from tangent
        # Use tangent as the local Y (forward) axis
        if abs(tan.dot(up)) > 0.99:
            right = tan.cross(Vector((1, 0, 0)))
        else:
            right = tan.cross(up)
        if right.length < 1e-12:
            right = Vector((1, 0, 0))
        right.normalize()
        local_up = right.cross(tan)
        local_up.normalize()

        # Apply twist
        twist_angle = twist * (i / max(n_path - 1, 1))
        cos_t = math.cos(twist_angle)
        sin_t = math.sin(twist_angle)

        # Apply scale variation
        sc = 1.0
        if scale_variation and i < len(scale_variation):
            sc = scale_variation[i]

        ring = []
        for (cx, cz) in cross_section:
            # Rotate cross-section by twist
            rx = cx * cos_t - cz * sin_t
            rz = cx * sin_t + cz * cos_t
            # Transform to world space
            world_pos = pos + right * (rx * sc) + local_up * (rz * sc)
            v = bm.verts.new(world_pos)
            ring.append(v)
        verts_grid.append(ring)

    bm.verts.ensure_lookup_table()

    # Connect rings with faces
    for i in range(n_path - 1):
        for j in range(n_cs - 1):
            v0 = verts_grid[i][j]
            v1 = verts_grid[i][j + 1]
            v2 = verts_grid[i + 1][j + 1]
            v3 = verts_grid[i + 1][j]
            try:
                bm.faces.new([v0, v1, v2, v3])
            except ValueError:
                pass

    # Cap ends
    if n_cs >= 3:
        try:
            bm.faces.new(list(reversed(verts_grid[0])))
        except ValueError:
            pass
        try:
            bm.faces.new(verts_grid[-1])
        except ValueError:
            pass


# ---------------------------------------------------------------------------
# Loft between profiles
# ---------------------------------------------------------------------------

def loft(bm, profile_rings, cap_start=True, cap_end=True):
    """
    Loft between multiple profile rings (each a list of Vector).
    Creates a smooth surface connecting the rings.

    Args:
        bm: BMesh to write into
        profile_rings: list of lists of Vector (each ring same vertex count)
        cap_start: close the first ring
        cap_end: close the last ring
    """
    if len(profile_rings) < 2:
        return

    n = len(profile_rings[0])
    vert_grid = []

    for ring in profile_rings:
        verts = []
        for pt in ring:
            v = bm.verts.new(pt)
            verts.append(v)
        vert_grid.append(verts)

    bm.verts.ensure_lookup_table()

    # Side faces
    for i in range(len(vert_grid) - 1):
        for j in range(n - 1):
            try:
                bm.faces.new([
                    vert_grid[i][j],
                    vert_grid[i][j + 1],
                    vert_grid[i + 1][j + 1],
                    vert_grid[i + 1][j],
                ])
            except ValueError:
                pass

    # Caps
    if n >= 3:
        if cap_start:
            try:
                bm.faces.new(list(reversed(vert_grid[0])))
            except ValueError:
                pass
        if cap_end:
            try:
                bm.faces.new(vert_grid[-1])
            except ValueError:
                pass


# ---------------------------------------------------------------------------
# Box / primitive helpers
# ---------------------------------------------------------------------------

def make_box(bm, min_corner, max_corner):
    """Create an axis-aligned box in the BMesh."""
    x0, y0, z0 = min_corner
    x1, y1, z1 = max_corner

    v = [
        bm.verts.new((x0, y0, z0)),
        bm.verts.new((x1, y0, z0)),
        bm.verts.new((x1, y1, z0)),
        bm.verts.new((x0, y1, z0)),
        bm.verts.new((x0, y0, z1)),
        bm.verts.new((x1, y0, z1)),
        bm.verts.new((x1, y1, z1)),
        bm.verts.new((x0, y1, z1)),
    ]
    bm.verts.ensure_lookup_table()

    faces = [
        (v[0], v[3], v[2], v[1]),  # bottom
        (v[4], v[5], v[6], v[7]),  # top
        (v[0], v[1], v[5], v[4]),  # front
        (v[2], v[3], v[7], v[6]),  # back
        (v[0], v[4], v[7], v[3]),  # left
        (v[1], v[2], v[6], v[5]),  # right
    ]
    for f in faces:
        try:
            bm.faces.new(f)
        except ValueError:
            pass
    return v


def make_cylinder(bm, radius, height, segments=24, center=True):
    """Create a cylinder. If center=True, pivot at base center."""
    z_offset = -height / 2 if center else 0
    profile = [(radius, z_offset), (radius, z_offset + height)]
    revolve_profile(bm, profile, segments=segments)
    return


# ---------------------------------------------------------------------------
# Arch generation
# ---------------------------------------------------------------------------

def make_arch(bm, width, height, depth, thickness=20.0, segments=16):
    """
    Create a baroque arch (semicircular top + rectangular sides).

    Args:
        bm: BMesh
        width: arch opening width
        height: total height (including semicircle)
        depth: arch depth (extrusion)
        thickness: arch frame thickness
        segments: arc resolution
    """
    # Outer arch profile (2D cross-section in XZ)
    arch_radius = width / 2.0
    spring_height = height - arch_radius

    # Build arch profile as a closed loop
    profile = []

    # Bottom-left
    profile.append((-width / 2.0 - thickness, 0))
    # Left side up to spring
    profile.append((-width / 2.0 - thickness, spring_height))
    # Outer arc
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * (1.0 - t)
        x = (width / 2.0 + thickness) * math.cos(angle)
        z = spring_height + arch_radius * math.sin(angle) + thickness
        profile.append((x, z))
    # Right side down
    profile.append((width / 2.0 + thickness, spring_height))
    profile.append((width / 2.0 + thickness, 0))
    # Bottom right to inner
    profile.append((width / 2.0, 0))
    # Inner right up to spring
    profile.append((width / 2.0, spring_height))
    # Inner arc
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * t
        x = arch_radius * math.cos(angle)
        z = spring_height + arch_radius * math.sin(angle)
        profile.append((x, z))
    # Inner left down
    profile.append((-width / 2.0, spring_height))
    profile.append((-width / 2.0, 0))

    # Extrude along Y (depth)
    extrude_profile_linear(bm, profile, depth, direction='Y')
