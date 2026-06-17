"""
Gravity platform generator — tilted floor that appears navigable.

Creates architectural platforms with tilted floors, inverted ceilings,
and Escher-style gravity-defying rooms where "up" is ambiguous.
"""

import math
import bmesh
from mathutils import Vector, Matrix
from ...base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ....core.mesh_builder import make_box, revolve_profile, extrude_profile_linear
from ....core import constants as C


@register_generator
class GravityPlatformGenerator(BaseGenerator):
    generator_id = "gravity_platform"
    generator_name = "Gravity Platform"
    category = "Escher"
    description = "Tilted floor platform with gravity-defying architecture"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("platform_width", "Platform Width", ParamType.FLOAT,
                                800, 200, 3000, "Platform width cm", "Dimensions"),
            ParameterDefinition("platform_length", "Platform Length", ParamType.FLOAT,
                                1200, 300, 4000, "Platform length cm", "Dimensions"),
            ParameterDefinition("wall_height", "Wall Height", ParamType.FLOAT,
                                C.STORY_HEIGHT, 200, 1500, "Room wall height cm", "Dimensions"),
            ParameterDefinition("tilt_angle_x", "Tilt Angle X", ParamType.FLOAT,
                                15, -45, 45, "Floor tilt around X axis (degrees)", "Gravity"),
            ParameterDefinition("tilt_angle_y", "Tilt Angle Y", ParamType.FLOAT,
                                10, -45, 45, "Floor tilt around Y axis (degrees)", "Gravity"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                25, 10, 60, "Wall thickness cm", "Structure"),
            ParameterDefinition("floor_thickness", "Floor Thickness", ParamType.FLOAT,
                                20, 5, 50, "Floor slab thickness cm", "Structure"),
            ParameterDefinition("columns_enabled", "Columns", ParamType.BOOL,
                                True, description="Add corner columns", category="Structure"),
            ParameterDefinition("column_radius", "Column Radius", ParamType.FLOAT,
                                18, 6, 40, "Column radius cm", "Structure"),
            ParameterDefinition("architrave_enabled", "Architrave", ParamType.BOOL,
                                True, description="Add connecting architraves", category="Structure"),
            ParameterDefinition("multi_level", "Multi-Level", ParamType.BOOL,
                                False, description="Stack multiple tilted platforms",
                                category="Layout"),
            ParameterDefinition("level_count", "Level Count", ParamType.INT,
                                2, 2, 5, "Number of stacked levels", "Layout"),
            ParameterDefinition("level_offset", "Level Offset", ParamType.FLOAT,
                                400, 200, 800, "Vertical offset between levels cm", "Layout"),
            ParameterDefinition("inverted_ceiling", "Inverted Ceiling", ParamType.BOOL,
                                True, description="Add ceiling that mirrors floor tilt",
                                category="Escher"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                16, 6, 32, "Column radial resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        pw = params.get('platform_width', 800)
        pl = params.get('platform_length', 1200)
        wh = params.get('wall_height', C.STORY_HEIGHT)
        tilt_x = params.get('tilt_angle_x', 15)
        tilt_y = params.get('tilt_angle_y', 10)
        wall_t = params.get('wall_thickness', 25)
        floor_t = params.get('floor_thickness', 20)
        has_cols = params.get('columns_enabled', True)
        col_r = params.get('column_radius', 18)
        has_arch = params.get('architrave_enabled', True)
        multi = params.get('multi_level', False)
        n_levels = params.get('level_count', 2)
        level_off = params.get('level_offset', 400)
        inverted = params.get('inverted_ceiling', True)
        segs = params.get('segments', 16)

        levels = n_levels if multi else 1

        for lvl in range(levels):
            base_z = lvl * level_off

            # Build tilt rotation matrix
            rot_x = Matrix.Rotation(math.radians(tilt_x), 4, 'X')
            rot_y = Matrix.Rotation(math.radians(tilt_y), 4, 'Y')
            tilt_mat = rot_y @ rot_x

            # Center of platform
            cx = pl / 2
            cy = pw / 2
            center = Vector((cx, cy, base_z))

            # ---- FLOOR SLAB (tilted) ----
            self._build_tilted_slab(bm, center, pw, pl, floor_t, tilt_mat,
                                    offset_z=0)

            # ---- WALLS (tilted) ----
            wall_local_h = wh
            # Four walls as tilted boxes
            # Front wall (Y = 0)
            self._build_tilted_wall(bm, center, pl, wall_local_h, wall_t,
                                    tilt_mat, axis='x', pos=-pw / 2)
            # Back wall (Y = pw)
            self._build_tilted_wall(bm, center, pl, wall_local_h, wall_t,
                                    tilt_mat, axis='x', pos=pw / 2)
            # Left wall (X = 0)
            self._build_tilted_wall(bm, center, pw, wall_local_h, wall_t,
                                    tilt_mat, axis='y', pos=-pl / 2)
            # Right wall (X = pl)
            self._build_tilted_wall(bm, center, pw, wall_local_h, wall_t,
                                    tilt_mat, axis='y', pos=pl / 2)

            # ---- CEILING (tilted, possibly inverted) ----
            if inverted:
                # Ceiling tilts opposite to floor (Escher effect)
                inv_tilt = Matrix.Rotation(math.radians(-tilt_x), 4, 'X') @ \
                           Matrix.Rotation(math.radians(-tilt_y), 4, 'Y')
                self._build_tilted_slab(bm, center, pw, pl, floor_t, inv_tilt,
                                        offset_z=wall_local_h)
            else:
                self._build_tilted_slab(bm, center, pw, pl, floor_t, tilt_mat,
                                        offset_z=wall_local_h)

            # ---- CORNER COLUMNS ----
            if has_cols:
                for ix in [-1, 1]:
                    for iy in [-1, 1]:
                        px = cx + ix * (pl / 2 - col_r)
                        py = cy + iy * (pw / 2 - col_r)
                        col_center = Vector((px, py, base_z))
                        # Column follows tilt
                        col_profile = [(col_r, 0), (col_r, wall_local_h),
                                       (col_r * 0.9, wall_local_h), (col_r * 0.9, 0)]
                        revolve_profile(bm, col_profile, segments=segs,
                                        offset=(px, py, base_z))

            # ---- ARCHITRAVES ----
            if has_arch:
                arch_h = 25
                arch_d = wall_t + 10
                # Top connecting beams
                make_box(bm,
                         (0, -arch_d / 2, base_z + wall_local_h),
                         (pl, arch_d / 2, base_z + wall_local_h + arch_h))
                make_box(bm,
                         (-arch_d / 2, 0, base_z + wall_local_h),
                         (arch_d / 2, pw, base_z + wall_local_h + arch_h))

    def _build_tilted_slab(self, bm, center, width, length, thickness,
                           tilt_mat, offset_z=0):
        """Build a tilted floor/ceiling slab."""
        hw = width / 2
        hl = length / 2
        z_off = offset_z

        # Four corners of the slab
        corners = [
            Vector((-hl, -hw, z_off)),
            Vector((hl, -hw, z_off)),
            Vector((hl, hw, z_off)),
            Vector((-hl, hw, z_off)),
            # Top face
            Vector((-hl, -hw, z_off + thickness)),
            Vector((hl, -hw, z_off + thickness)),
            Vector((hl, hw, z_off + thickness)),
            Vector((-hl, hw, z_off + thickness)),
        ]

        # Apply tilt around center
        tilted = []
        for c in corners:
            local = c - center
            rotated = tilt_mat @ local
            tilted.append(rotated + center)

        # Create faces
        v = [bm.verts.new(p) for p in tilted]
        bm.verts.ensure_lookup_table()

        # Bottom face
        try:
            bm.faces.new([v[0], v[3], v[2], v[1]])
        except ValueError:
            pass
        # Top face
        try:
            bm.faces.new([v[4], v[5], v[6], v[7]])
        except ValueError:
            pass
        # Side faces
        sides = [(0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        for s in sides:
            try:
                bm.faces.new([v[s[0]], v[s[1]], v[s[2]], v[s[3]]])
            except ValueError:
                pass

    def _build_tilted_wall(self, bm, center, length, height, thickness,
                           tilt_mat, axis='x', pos=0):
        """Build a tilted wall."""
        if axis == 'x':
            # Wall runs along X, positioned at Y=pos
            corners = [
                Vector((-length / 2, pos, 0)),
                Vector((length / 2, pos, 0)),
                Vector((length / 2, pos, height)),
                Vector((-length / 2, pos, height)),
                Vector((-length / 2, pos + thickness, 0)),
                Vector((length / 2, pos + thickness, 0)),
                Vector((length / 2, pos + thickness, height)),
                Vector((-length / 2, pos + thickness, height)),
            ]
        else:
            # Wall runs along Y, positioned at X=pos
            corners = [
                Vector((pos, -length / 2, 0)),
                Vector((pos, length / 2, 0)),
                Vector((pos, length / 2, height)),
                Vector((pos, -length / 2, height)),
                Vector((pos + thickness, -length / 2, 0)),
                Vector((pos + thickness, length / 2, 0)),
                Vector((pos + thickness, length / 2, height)),
                Vector((pos + thickness, -length / 2, height)),
            ]

        # Apply tilt
        tilted = []
        for c in corners:
            local = c - center
            rotated = tilt_mat @ local
            tilted.append(rotated + center)

        v = [bm.verts.new(p) for p in tilted]
        bm.verts.ensure_lookup_table()

        faces = [
            (v[0], v[3], v[2], v[1]),  # inner
            (v[4], v[5], v[6], v[7]),  # outer
            (v[0], v[1], v[5], v[4]),  # bottom
            (v[2], v[3], v[7], v[6]),  # top
            (v[0], v[4], v[7], v[3]),  # left
            (v[1], v[2], v[6], v[5]),  # right
        ]
        for f in faces:
            try:
                bm.faces.new(f)
            except ValueError:
                pass
