"""
Bezier curve-driven freeform architecture generator.

Define architectural elements (walls, arches, corridors, towers) as
bezier control-point splines and sweep cross-sections along them.
"""

import math
import bmesh
from mathutils import Vector
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ..core.mesh_builder import sweep_along_curve, make_box, loft, revolve_profile
from ..core.bezier import CubicBezier
from ..core import constants as C
from ..core.golden import golden_spiral_points


@register_generator
class CurveArchitectureGenerator(BaseGenerator):
    generator_id = "curve_architecture"
    generator_name = "Curve Architecture"
    category = "Architecture"
    description = "Bezier curve-driven freeform architectural elements"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("curve_type", "Curve Type", ParamType.ENUM,
                                "freeform",
                                enum_items=[
                                    ("freeform", "Freeform Spline", "User-defined control points"),
                                    ("spiral", "Golden Spiral", "Golden ratio spiral tower"),
                                    ("helix", "Helix Ramp", "Helical walkway/tower"),
                                    ("wave", "Undulating Wall", "Sinusoidal wall curve"),
                                    ("catenary", "Catenary Arch", "Natural arch curve"),
                                ],
                                category="Curve"),
            ParameterDefinition("length", "Length", ParamType.FLOAT,
                                1600, 200, 8000, "Total curve length cm", "Dimensions"),
            ParameterDefinition("wall_height", "Wall Height", ParamType.FLOAT,
                                C.STORY_HEIGHT, 200, 2000, "Wall/floor height cm", "Dimensions"),
            ParameterDefinition("wall_thickness", "Wall Thickness", ParamType.FLOAT,
                                30, 10, 80, "Wall thickness cm", "Dimensions"),
            ParameterDefinition("floor_enabled", "Floor", ParamType.BOOL,
                                True, description="Add floor slab", category="Structure"),
            ParameterDefinition("floor_thickness", "Floor Thickness", ParamType.FLOAT,
                                15, 5, 40, "Floor slab thickness cm", "Structure"),
            ParameterDefinition("ceiling_enabled", "Ceiling", ParamType.BOOL,
                                True, description="Add ceiling slab", category="Structure"),
            ParameterDefinition("cross_section", "Cross Section", ParamType.ENUM,
                                "rectangular",
                                enum_items=[
                                    ("rectangular", "Rectangular", "Rectangular corridor/wall"),
                                    ("arched", "Arched", "Semicircular arch cross-section"),
                                    ("octagonal", "Octagonal", "Octagonal tower"),
                                    ("circular", "Circular", "Circular tower/tube"),
                                ],
                                category="Cross Section"),
            ParameterDefinition("twist", "Twist", ParamType.FLOAT,
                                0, 0, 720, "Total twist degrees along path", "Curve"),
            ParameterDefinition("taper_ratio", "Taper Ratio", ParamType.FLOAT,
                                1.0, 0.2, 2.0, "End/start scale ratio", "Curve"),
            ParameterDefinition("curve_amplitude", "Amplitude", ParamType.FLOAT,
                                400, 50, 2000, "Curve deviation amplitude cm", "Curve"),
            ParameterDefinition("curve_frequency", "Frequency", ParamType.FLOAT,
                                2.0, 0.5, 8.0, "Number of wave periods", "Curve"),
            ParameterDefinition("helix_radius", "Helix Radius", ParamType.FLOAT,
                                600, 100, 3000, "Helix/spiral radius cm", "Curve"),
            ParameterDefinition("helix_turns", "Helix Turns", ParamType.FLOAT,
                                2.0, 0.5, 8.0, "Number of helix turns", "Curve"),
            ParameterDefinition("samples", "Samples", ParamType.INT,
                                48, 8, 128, "Curve sampling resolution", "Detail"),
            ParameterDefinition("segments", "Segments", ParamType.INT,
                                12, 4, 32, "Cross-section resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        curve_type = params.get('curve_type', 'freeform')
        length = params.get('length', 1600)
        wall_h = params.get('wall_height', C.STORY_HEIGHT)
        wall_t = params.get('wall_thickness', 30)
        has_floor = params.get('floor_enabled', True)
        floor_t = params.get('floor_thickness', 15)
        has_ceiling = params.get('ceiling_enabled', True)
        cs_type = params.get('cross_section', 'rectangular')
        twist_deg = params.get('twist', 0)
        taper = params.get('taper_ratio', 1.0)
        amplitude = params.get('curve_amplitude', 400)
        frequency = params.get('curve_frequency', 2.0)
        helix_r = params.get('helix_radius', 600)
        helix_turns = params.get('helix_turns', 2.0)
        n_samples = params.get('samples', 48)
        n_segs = params.get('segments', 12)

        # ---- GENERATE CURVE PATH ----
        control_points = self._generate_control_points(
            curve_type, length, amplitude, frequency, helix_r, helix_turns, wall_h, ctx
        )

        # Fit bezier segments through control points
        bezier_segments = CubicBezier.fit_from_points(control_points)

        # Sample full path
        path_points = []
        path_tangents = []
        total_segs = len(bezier_segments)
        samples_per_seg = max(n_samples // max(total_segs, 1), 4)

        for seg in bezier_segments:
            pts_tans = seg.sample_with_tangents(samples_per_seg)
            for pt, tan in pts_tans:
                # Avoid duplicating junction points
                if path_points and (pt - path_points[-1]).length < 1e-6:
                    continue
                path_points.append(pt)
                path_tangents.append(tan)

        if len(path_points) < 2:
            return

        # ---- BUILD CROSS-SECTION ----
        cross_section = self._build_cross_section(cs_type, wall_h, wall_t, n_segs)

        # ---- SCALE VARIATION (taper) ----
        scale_variation = None
        if abs(taper - 1.0) > 0.01:
            n_pts = len(path_points)
            scale_variation = []
            for i in range(n_pts):
                t = i / max(n_pts - 1, 1)
                s = 1.0 + (taper - 1.0) * t
                scale_variation.append(s)

        # ---- SWEEP ----
        twist_rad = math.radians(twist_deg)
        sweep_along_curve(bm, path_points, path_tangents, cross_section,
                          twist=twist_rad, scale_variation=scale_variation)

        # ---- FLOOR SLAB ----
        if has_floor:
            floor_cs = self._build_floor_cross_section(wall_h, floor_t, n_segs)
            sweep_along_curve(bm, path_points, path_tangents, floor_cs,
                              twist=0.0, scale_variation=scale_variation)

        # ---- CEILING SLAB ----
        if has_ceiling:
            ceil_cs = self._build_ceiling_cross_section(wall_h, floor_t, n_segs)
            sweep_along_curve(bm, path_points, path_tangents, ceil_cs,
                              twist=0.0, scale_variation=scale_variation)

    def _generate_control_points(self, curve_type, length, amplitude,
                                 frequency, helix_r, helix_turns, wall_h, ctx):
        """Generate control points based on curve type."""
        rng = ctx.rng
        n_ctrl = max(int(frequency * 4), 4)

        if curve_type == 'spiral':
            # Golden spiral in XY, rising in Z
            spiral_pts = golden_spiral_points(turns=helix_turns, scale=helix_r * 0.3,
                                              segments_per_turn=12)
            points = []
            n = len(spiral_pts)
            for i, (sx, sy) in enumerate(spiral_pts):
                z = wall_h * (i / max(n - 1, 1))
                points.append(Vector((sx + helix_r, sy, z)))
            return points

        elif curve_type == 'helix':
            points = []
            total_angle = helix_turns * math.pi * 2
            n_pts = max(int(helix_turns * 16), 16)
            for i in range(n_pts + 1):
                t = i / n_pts
                angle = total_angle * t
                x = helix_r * math.cos(angle)
                y = helix_r * math.sin(angle)
                z = wall_h * t * helix_turns
                points.append(Vector((x, y, z)))
            return points

        elif curve_type == 'wave':
            points = []
            for i in range(n_ctrl + 1):
                t = i / n_ctrl
                x = length * t
                y = amplitude * math.sin(t * frequency * math.pi * 2)
                z = 0
                points.append(Vector((x, y, z)))
            return points

        elif curve_type == 'catenary':
            # Catenary arch: y = a * cosh(x/a)
            points = []
            a = amplitude * 0.5
            half_l = length * 0.5
            for i in range(n_ctrl + 1):
                t = i / n_ctrl
                x = -half_l + length * t
                z = a * (math.cosh(x / a) - 1.0) if abs(x / a) < 50 else a * 50
                points.append(Vector((x, 0, z)))
            return points

        else:  # freeform
            points = [Vector((0, 0, 0))]
            for i in range(1, n_ctrl):
                x = length * (i / n_ctrl)
                y = rng.uniform(-amplitude, amplitude)
                z = rng.uniform(0, wall_h * 0.5)
                points.append(Vector((x, y, z)))
            points.append(Vector((length, 0, 0)))
            return points

    def _build_cross_section(self, cs_type, wall_h, wall_t, segments):
        """Build 2D cross-section profile for sweep."""
        if cs_type == 'arched':
            # Rectangular base + semicircular top
            profile = []
            # Floor
            profile.append((-wall_h / 2, 0))
            # Left wall up
            profile.append((-wall_h / 2, wall_h))
            # Arch
            r = wall_h / 2
            for i in range(segments + 1):
                t = i / segments
                angle = math.pi * t
                ax = -r * math.cos(angle)
                az = wall_h + r * math.sin(angle)
                profile.append((ax, az))
            # Right wall down
            profile.append((wall_h / 2, wall_h))
            profile.append((wall_h / 2, 0))
            return profile

        elif cs_type == 'octagonal':
            profile = []
            r = wall_h * 0.4
            for i in range(8):
                angle = math.pi * 2 * i / 8
                x = r * math.cos(angle)
                z = r * math.sin(angle) + r
                profile.append((x, z))
            # Close
            profile.append(profile[0])
            return profile

        elif cs_type == 'circular':
            profile = []
            r = wall_h * 0.35
            for i in range(segments + 1):
                angle = math.pi * 2 * i / segments
                x = r * math.cos(angle)
                z = r * math.sin(angle) + r
                profile.append((x, z))
            return profile

        else:  # rectangular
            hw = wall_h / 2
            return [
                (-hw, 0), (-hw, wall_h),
                (hw, wall_h), (hw, 0),
                (-hw, 0),
            ]

    def _build_floor_cross_section(self, wall_h, floor_t, segments):
        """Thin floor slab cross-section."""
        hw = wall_h / 2
        return [
            (-hw, -floor_t), (hw, -floor_t),
            (hw, 0), (-hw, 0), (-hw, -floor_t),
        ]

    def _build_ceiling_cross_section(self, wall_h, floor_t, segments):
        """Thin ceiling slab cross-section."""
        hw = wall_h / 2
        return [
            (-hw, wall_h), (hw, wall_h),
            (hw, wall_h + floor_t), (-hw, wall_h + floor_t),
            (-hw, wall_h),
        ]
