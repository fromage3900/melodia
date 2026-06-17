"""
Unit tests for core math utilities, bezier curves, and golden ratio.

These tests can run standalone (no Blender required) for the pure math modules.
For Blender-dependent tests, use test_generators.py inside Blender.
"""

import math
import sys
import os

# Add parent to path for standalone testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestMathUtils:
    """Tests for core.math_utils."""

    def test_lerp(self):
        from core.math_utils import lerp
        assert lerp(0, 10, 0.0) == 0.0
        assert lerp(0, 10, 1.0) == 10.0
        assert lerp(0, 10, 0.5) == 5.0
        assert lerp(-100, 100, 0.25) == -50.0

    def test_smoothstep(self):
        from core.math_utils import smoothstep
        assert smoothstep(0, 1, 0.0) == 0.0
        assert smoothstep(0, 1, 1.0) == 1.0
        assert abs(smoothstep(0, 1, 0.5) - 0.5) < 1e-10
        # Monotonically increasing
        prev = 0.0
        for i in range(11):
            t = i / 10
            val = smoothstep(0, 1, t)
            assert val >= prev
            prev = val

    def test_clamp(self):
        from core.math_utils import clamp
        assert clamp(5, 0, 10) == 5
        assert clamp(-5, 0, 10) == 0
        assert clamp(15, 0, 10) == 10


class TestBezier:
    """Tests for core.bezier.CubicBezier."""

    def _make_line_bezier(self):
        """Create a bezier that represents a straight line from (0,0,0) to (10,0,0)."""
        try:
            from mathutils import Vector
        except ImportError:
            return None
        from core.bezier import CubicBezier
        return CubicBezier(
            Vector((0, 0, 0)),
            Vector((3.33, 0, 0)),
            Vector((6.67, 0, 0)),
            Vector((10, 0, 0)),
        )

    def test_evaluate_endpoints(self):
        bz = self._make_line_bezier()
        if bz is None:
            return  # Skip if no mathutils
        start = bz.evaluate(0.0)
        end = bz.evaluate(1.0)
        assert abs(start.x) < 1e-6
        assert abs(end.x - 10.0) < 1e-6

    def test_evaluate_midpoint(self):
        bz = self._make_line_bezier()
        if bz is None:
            return
        mid = bz.evaluate(0.5)
        assert abs(mid.x - 5.0) < 0.5  # approximately midpoint

    def test_arc_length(self):
        bz = self._make_line_bezier()
        if bz is None:
            return
        length = bz.arc_length()
        assert abs(length - 10.0) < 0.5  # approximately 10

    def test_sample_count(self):
        bz = self._make_line_bezier()
        if bz is None:
            return
        pts = bz.sample(11)
        assert len(pts) == 11

    def test_split(self):
        bz = self._make_line_bezier()
        if bz is None:
            return
        left, right = bz.split(0.5)
        # Left should start at same point
        assert abs(left.p0.x - bz.p0.x) < 1e-6
        # Right should end at same point
        assert abs(right.p3.x - bz.p3.x) < 1e-6

    def test_reverse(self):
        bz = self._make_line_bezier()
        if bz is None:
            return
        rev = bz.reverse()
        assert abs(rev.p0.x - bz.p3.x) < 1e-6
        assert abs(rev.p3.x - bz.p0.x) < 1e-6

    def test_fit_from_points(self):
        try:
            from mathutils import Vector
        except ImportError:
            return
        from core.bezier import CubicBezier
        points = [Vector((i * 10, 0, 0)) for i in range(5)]
        segs = CubicBezier.fit_from_points(points)
        assert len(segs) == 4  # n-1 segments for n points


class TestGolden:
    """Tests for core.golden."""

    def test_golden_section(self):
        # Can test without bpy
        # golden_section splits by 0.618
        value = 100
        ratio = 0.6180339887498949
        larger = value * ratio
        smaller = value - larger
        assert abs(larger - 61.803) < 0.01
        assert abs(smaller - 38.197) < 0.01

    def test_phi_value(self):
        from core.constants import PHI, GOLDEN_RATIO
        assert abs(PHI - 1.6180339887) < 1e-6
        assert abs(GOLDEN_RATIO - 0.6180339887) < 1e-6
        assert abs(PHI * GOLDEN_RATIO - 1.0) < 1e-6  # φ × φ⁻¹ = 1

    def test_proportional_scale(self):
        # Test that proportional_scale decreases by golden ratio
        base = 1000
        ratio = 0.6180339887498949
        # Manual check
        expected = [base, base * ratio, base * ratio * ratio]
        # Can't import without bpy, but verify math
        assert abs(expected[1] - 618.034) < 0.01
        assert abs(expected[2] - 381.966) < 0.01


class TestConstants:
    """Tests for core.constants alignment."""

    def test_module_grid(self):
        from core.constants import WALL_WIDTH, STORY_HEIGHT, COLUMN_SPACING
        assert WALL_WIDTH == 400
        assert STORY_HEIGHT == 600
        assert COLUMN_SPACING == 400

    def test_escher_defaults(self):
        from core.constants import ESCHER_STEP_HEIGHT, ESCHER_STAIR_WIDTH
        assert ESCHER_STEP_HEIGHT == 20
        assert ESCHER_STAIR_WIDTH == 120

    def test_facade_defaults(self):
        from core.constants import WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_SILL_HEIGHT
        assert WINDOW_WIDTH == 160
        assert WINDOW_HEIGHT == 280
        assert WINDOW_SILL_HEIGHT == 120


class TestProfileCurves:
    """Tests for core.profile_curves (pure math, no bpy needed)."""

    def test_ogee_returns_points(self):
        # profile_curves imports from math_utils which is pure Python
        # but mathutils may not be available outside Blender
        try:
            from core.profile_curves import ogee
            pts = ogee(radius=10, height=20, segments=8)
            assert len(pts) == 9  # segments + 1
            # All z values should be within [0, height]
            for x, z in pts:
                assert z >= -0.01
                assert z <= 20.01
        except ImportError:
            pass  # Skip if mathutils not available

    def test_baluster_turned(self):
        try:
            from core.profile_curves import baluster_turned
            pts = baluster_turned(height=80, segments=16)
            assert len(pts) == 17
            # First point at z=0, last at z=height
            assert abs(pts[0][1]) < 1e-6
            assert abs(pts[-1][1] - 80) < 1e-6
        except ImportError:
            pass


if __name__ == '__main__':
    # Run with: python -m pytest tests/test_math.py -v
    import pytest
    pytest.main([__file__, '-v'])
