"""Finial generator — flame/acorn pinnacle ornament."""

import math
import bmesh
from ..base_generator import BaseGenerator, GeneratorContext, ParameterDefinition, ParamType, register_generator
from ...core.mesh_builder import revolve_profile
from ...core import constants as C


@register_generator
class FinialGenerator(BaseGenerator):
    generator_id = "ornament_finial"
    generator_name = "Finial"
    category = "Ornaments"
    description = "Flame or acorn pinnacle finial ornament"

    @classmethod
    def get_parameters(cls):
        return [
            ParameterDefinition("height", "Height", ParamType.FLOAT, 80, 20, 300, "cm", "Dimensions"),
            ParameterDefinition("radius", "Max Radius", ParamType.FLOAT, 15, 3, 50, "cm", "Dimensions"),
            ParameterDefinition("style", "Style", ParamType.ENUM, "flame",
                                enum_items=[("flame", "Flame", "Flame-shaped finial"),
                                            ("acorn", "Acorn", "Acorn-shaped finial"),
                                            ("ball", "Ball", "Spherical finial")],
                                category="Shape"),
            ParameterDefinition("segments", "Segments", ParamType.INT, 24, 8, 48, "Resolution", "Detail"),
        ]

    def _build(self, bm, params, ctx):
        height = params.get('height', 80)
        radius = params.get('radius', 15)
        style = params.get('style', 'flame')
        segs = params.get('segments', 24)

        n = 24
        profile = []

        if style == 'flame':
            # Flame shape: stacked lobes with pointed tip
            for i in range(n + 1):
                t = i / n
                z = height * t
                # Flame profile: bulge at 0.3, narrow at 0.6, bulge at 0.8, point at 1.0
                if t < 0.15:
                    r = radius * 0.4 * (t / 0.15)  # base
                elif t < 0.35:
                    lt = (t - 0.15) / 0.2
                    r = radius * (0.4 + 0.6 * math.sin(lt * math.pi * 0.5))
                elif t < 0.55:
                    lt = (t - 0.35) / 0.2
                    r = radius * (1.0 - 0.4 * lt)
                elif t < 0.75:
                    lt = (t - 0.55) / 0.2
                    r = radius * (0.6 + 0.3 * math.sin(lt * math.pi * 0.5))
                elif t < 0.9:
                    lt = (t - 0.75) / 0.15
                    r = radius * (0.9 - 0.6 * lt)
                else:
                    lt = (t - 0.9) / 0.1
                    r = radius * 0.3 * (1.0 - lt)
                profile.append((max(r, 0.1), z))

        elif style == 'acorn':
            # Acorn: bulb + cap + point
            for i in range(n + 1):
                t = i / n
                z = height * t
                if t < 0.6:
                    # Bulb
                    lt = t / 0.6
                    r = radius * math.sin(lt * math.pi * 0.5)
                elif t < 0.8:
                    # Cap
                    lt = (t - 0.6) / 0.2
                    r = radius * (1.0 + 0.15 * math.sin(lt * math.pi))
                else:
                    # Point
                    lt = (t - 0.8) / 0.2
                    r = radius * (1.0 - lt) * 0.8
                profile.append((max(r, 0.1), z))

        else:  # ball
            for i in range(n + 1):
                t = i / n
                z = height * t
                angle = math.pi * t
                r = radius * math.sin(angle)
                profile.append((max(r, 0.1), z))

        # Add base pedestal
        base_profile = [
            (0, 0),
            (radius * 0.5, 0),
            (radius * 0.5, height * 0.05),
            (radius * 0.35, height * 0.05),
        ]

        full_profile = base_profile + [(r + radius * 0.1, z + height * 0.05) for r, z in profile]
        revolve_profile(bm, full_profile, segments=segs)
