"""
Multi-story baroque facade generator — window rhythms, pilasters, cornices,
balconies, rustication. Aligned to UE5 PCG module grid.
"""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ..core.mesh_builder import make_box, extrude_profile_linear, revolve_profile
from ..core.profile_curves import ogee, cyma_recta, cavetto
from ..core import constants as C
from ..core.golden import golden_section


@register_generator
class FacadeGenerator(BaseGenerator):
    generator_id = "facade"
    generator_name = "Baroque Facade"
    category = "Architecture"
    description = "Multi-story baroque facade with windows, pilasters, cornices, balconies"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("facade_width", "Facade Width", ParamType.FLOAT,
                                2400, 400, 8000, "Total facade width cm", "Dimensions"),
            ParameterDefinition("story_height", "Story Height", ParamType.FLOAT,
                                C.FACADE_STORY_HEIGHT, 300, 1200, "Height per story cm", "Dimensions"),
            ParameterDefinition("story_count", "Story Count", ParamType.INT,
                                3, 1, 7, "Number of stories", "Dimensions"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                C.FACADE_WALL_THICKNESS, 10, 80, "Wall thickness cm", "Structure"),
            ParameterDefinition("window_width", "Window Width", ParamType.FLOAT,
                                C.WINDOW_WIDTH, 60, 300, "Window opening width cm", "Windows"),
            ParameterDefinition("window_height", "Window Height", ParamType.FLOAT,
                                C.WINDOW_HEIGHT, 100, 500, "Window opening height cm", "Windows"),
            ParameterDefinition("window_sill_height", "Sill Height", ParamType.FLOAT,
                                C.WINDOW_SILL_HEIGHT, 40, 300, "Window sill height from floor cm", "Windows"),
            ParameterDefinition("windows_per_bay", "Windows Per Bay", ParamType.INT,
                                1, 1, 3, "Windows per bay per story", "Windows"),
            ParameterDefinition("pilaster_width", "Pilaster Width", ParamType.FLOAT,
                                40, 15, 80, "Pilaster width cm", "Pilasters"),
            ParameterDefinition("pilaster_depth", "Pilaster Depth", ParamType.FLOAT,
                                15, 5, 40, "Pilaster projection cm", "Pilasters"),
            ParameterDefinition("pilaster_enabled", "Pilasters", ParamType.BOOL,
                                True, description="Add pilasters between bays", category="Pilasters"),
            ParameterDefinition("cornice_enabled", "Cornices", ParamType.BOOL,
                                True, description="Add story cornices", category="Cornices"),
            ParameterDefinition("cornice_projection", "Cornice Projection", ParamType.FLOAT,
                                25, 5, 60, "Cornice outward projection cm", "Cornices"),
            ParameterDefinition("balcony_enabled", "Balconies", ParamType.BOOL,
                                True, description="Add balconies to piano nobile", category="Balconies"),
            ParameterDefinition("balcony_depth", "Balcony Depth", ParamType.FLOAT,
                                80, 30, 200, "Balcony platform depth cm", "Balconies"),
            ParameterDefinition("rustication_enabled", "Rustication", ParamType.BOOL,
                                True, description="Ground floor rustication", category="Rustication"),
            ParameterDefinition("rustication_depth", "Rustication Depth", ParamType.FLOAT,
                                8, 2, 20, "Rustication block projection cm", "Rustication"),
            ParameterDefinition("parapet_enabled", "Parapet", ParamType.BOOL,
                                True, description="Top parapet with balustrade", category="Parapet"),
            ParameterDefinition("parapet_height", "Parapet Height", ParamType.FLOAT,
                                80, 30, 200, "Parapet height cm", "Parapet"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                12, 6, 32, "Curved element resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        fw = params.get('facade_width', 2400)
        sh = params.get('story_height', C.FACADE_STORY_HEIGHT)
        n_stories = params.get('story_count', 3)
        wall_t = params.get('wall_thickness', C.FACADE_WALL_THICKNESS)
        win_w = params.get('window_width', C.WINDOW_WIDTH)
        win_h = params.get('window_height', C.WINDOW_HEIGHT)
        sill_h = params.get('window_sill_height', C.WINDOW_SILL_HEIGHT)
        win_per_bay = params.get('windows_per_bay', 1)
        pil_w = params.get('pilaster_width', 40)
        pil_d = params.get('pilaster_depth', 15)
        has_pilasters = params.get('pilaster_enabled', True)
        has_cornices = params.get('cornice_enabled', True)
        cornice_proj = params.get('cornice_projection', 25)
        has_balcony = params.get('balcony_enabled', True)
        balcony_d = params.get('balcony_depth', 80)
        has_rustication = params.get('rustication_enabled', True)
        rust_depth = params.get('rustication_depth', 8)
        has_parapet = params.get('parapet_enabled', True)
        parapet_h = params.get('parapet_height', 80)
        segs = params.get('segments', 12)

        total_h = sh * n_stories

        # Bay width = column spacing module (400cm)
        bay_width = C.COLUMN_SPACING
        n_bays = max(int(fw / bay_width), 1)
        actual_bay_w = fw / n_bays

        # ---- MAIN WALL ----
        make_box(bm, (0, 0, 0), (fw, wall_t, total_h))

        # ---- RUSTICATION (ground floor) ----
        if has_rustication and n_stories >= 1:
            block_h = 40
            block_w = actual_bay_w / 3
            n_rows = int(sh / block_h)
            n_cols = int(fw / block_w)
            for r in range(n_rows):
                for c in range(n_cols):
                    bx = c * block_w
                    bz = r * block_h
                    # Alternate projection for rustication pattern
                    proj = rust_depth if (r + c) % 2 == 0 else rust_depth * 0.5
                    make_box(bm,
                             (bx, -proj, bz),
                             (bx + block_w - 2, 0, bz + block_h - 2))

        # ---- PILASTERS ----
        if has_pilasters:
            for i in range(n_bays + 1):
                px = i * actual_bay_w - pil_w / 2
                if px < -pil_w / 2:
                    px = 0
                # Full-height pilaster
                make_box(bm,
                         (px, -pil_d, 0),
                         (px + pil_w, 0, total_h))
                # Pilaster capital (simplified)
                cap_h = 30
                make_box(bm,
                         (px - 5, -pil_d - 5, total_h - cap_h),
                         (px + pil_w + 5, 5, total_h))

        # ---- STORY CORNICES ----
        if has_cornices:
            for s in range(1, n_stories + 1):
                cz = s * sh
                # Cornice as extruded ogee profile
                cornice_profile = ogee(radius=cornice_proj * 0.6,
                                       height=cornice_proj * 0.4,
                                       segments=segs)
                # Shift profile to cornice position
                shifted = [(x + wall_t, z + cz) for x, z in cornice_profile]
                extrude_profile_linear(bm, shifted, fw, direction='X',
                                       offset=(0, -cornice_proj, 0))

        # ---- WINDOWS ----
        piano_nobile = 1  # second story (index 1) for balconies
        for s in range(n_stories):
            floor_z = s * sh
            for b in range(n_bays):
                bay_cx = (b + 0.5) * actual_bay_w

                # Window placement
                if win_per_bay == 1:
                    offsets = [0]
                elif win_per_bay == 2:
                    offsets = [-actual_bay_w * 0.2, actual_bay_w * 0.2]
                else:
                    offsets = [-actual_bay_w * 0.3, 0, actual_bay_w * 0.3]

                for wo in offsets:
                    wx = bay_cx + wo - win_w / 2
                    wz = floor_z + sill_h

                    # Window recess (dark void)
                    make_box(bm,
                             (wx, -wall_t * 0.3, wz),
                             (wx + win_w, wall_t * 0.5, wz + win_h))

                    # Window surround (architrave frame)
                    frame_w = 8
                    frame_d = 5
                    # Left jamb
                    make_box(bm,
                             (wx - frame_w, -frame_d, wz),
                             (wx, 0, wz + win_h))
                    # Right jamb
                    make_box(bm,
                             (wx + win_w, -frame_d, wz),
                             (wx + win_w + frame_w, 0, wz + win_h))
                    # Lintel / arch
                    lintel_h = 20
                    make_box(bm,
                             (wx - frame_w, -frame_d, wz + win_h),
                             (wx + win_w + frame_w, 0, wz + win_h + lintel_h))

                    # Pediment (alternating triangular/segmental)
                    if s > 0:
                        ped_h = 25
                        ped_w = win_w + 2 * frame_w
                        if (b + s) % 2 == 0:
                            # Triangular pediment
                            ped_profile = [
                                (-ped_w / 2, 0), (0, ped_h),
                                (ped_w / 2, 0), (-ped_w / 2, 0)
                            ]
                            shifted_ped = [(x + bay_cx + wo, z + wz + win_h + lintel_h)
                                           for x, z in ped_profile]
                            extrude_profile_linear(bm, shifted_ped, frame_d,
                                                   direction='Y', offset=(0, -frame_d, 0))
                        else:
                            # Segmental (arched) pediment
                            arch_pts = []
                            for i in range(segs + 1):
                                t = i / segs
                                angle = math.pi * t
                                ax = ped_w / 2 * math.cos(angle)
                                az = ped_h * math.sin(angle)
                                arch_pts.append((ax + bay_cx + wo, az + wz + win_h + lintel_h))
                            # Simplified as box
                            make_box(bm,
                                     (wx - frame_w, -frame_d, wz + win_h + lintel_h),
                                     (wx + win_w + frame_w, 0, wz + win_h + lintel_h + ped_h * 0.6))

                # ---- BALCONIES (piano nobile) ----
                if has_balcony and s == piano_nobile:
                    balc_w = win_w + 40
                    balc_z = floor_z + sill_h - 10
                    # Platform
                    make_box(bm,
                             (bay_cx - balc_w / 2, -balcony_d, balc_z),
                             (bay_cx + balc_w / 2, 0, balc_z + 10))
                    # Balustrade (simplified as thin wall)
                    rail_h = 60
                    rail_t = 6
                    # Front rail
                    make_box(bm,
                             (bay_cx - balc_w / 2, -balcony_d, balc_z + 10),
                             (bay_cx + balc_w / 2, -balcony_d + rail_t, balc_z + 10 + rail_h))
                    # Side rails
                    for side in [-1, 1]:
                        sx = bay_cx + side * balc_w / 2
                        make_box(bm,
                                 (sx - rail_t * (1 if side == 1 else 0),
                                  -balcony_d, balc_z + 10),
                                 (sx + rail_t * (1 if side == -1 else 0),
                                  -balcony_d + rail_t, balc_z + 10 + rail_h))

        # ---- PARAPET ----
        if has_parapet:
            parapet_t = 15
            make_box(bm, (0, -parapet_t, total_h),
                     (fw, 0, total_h + parapet_h))
            # Parapet cap (cyma recta profile)
            cap_h = 12
            cap_d = parapet_t + 8
            make_box(bm, (0, -cap_d, total_h + parapet_h),
                     (fw, 0, total_h + parapet_h + cap_h))

            # Balustrade on top of parapet (simplified)
            bal_h = 50
            bal_r = 8
            n_bals = int(fw / (bal_r * 4))
            for i in range(n_bals):
                bx = (i + 0.5) * (fw / n_bals)
                bal_profile = [(bal_r, 0), (bal_r * 1.3, bal_h * 0.3),
                               (bal_r * 0.6, bal_h * 0.5),
                               (bal_r * 1.1, bal_h * 0.7),
                               (bal_r * 0.8, bal_h), (bal_r, 0)]
                revolve_profile(bm, bal_profile, segments=segs // 2,
                                offset=(bx, -cap_d / 2, total_h + parapet_h + cap_h))
