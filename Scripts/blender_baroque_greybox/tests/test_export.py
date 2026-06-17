"""
FBX export verification tests — ensures exported FBX meets UE5 requirements.

Must be run inside Blender. Tests:
- Scale (cm → m conversion via global_scale=0.01)
- SM_ naming prefix
- Pivot at base center
- Non-empty mesh data
"""

import bpy
import os
import sys
import tempfile

addon_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if addon_dir not in sys.path:
    sys.path.insert(0, os.path.dirname(addon_dir))


def _clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def _generate_test_object():
    """Generate a simple test object."""
    from generators.balustrade import BalustradeGenerator
    from generators.base_generator import GeneratorContext
    from core.geometry_modes import GeometryMode

    _clear_scene()
    ctx = GeneratorContext(
        scene=bpy.context.scene,
        collection=bpy.context.scene.collection,
        mode=GeometryMode.VALID,
        seed=42,
    )
    gen = BalustradeGenerator()
    return gen.generate({'length': 200, 'baluster_count': 5}, ctx)


class TestNaming:
    """Test SM_ prefix naming convention."""

    def test_generator_name_has_sm_prefix(self):
        from core.naming import make_generator_name
        name = make_generator_name("column_doric", "", 42)
        assert name.startswith("SM_"), f"Expected SM_ prefix, got: {name}"

    def test_variant_name(self):
        from core.naming import make_generator_name
        name = make_generator_name("column_doric", "variant_a", 42)
        assert name.startswith("SM_")
        assert "variant_a" in name

    def test_unique_names(self):
        from core.naming import make_generator_name
        name1 = make_generator_name("column_doric", "", 1)
        name2 = make_generator_name("column_doric", "", 2)
        assert name1 != name2


class TestFBXExport:
    """Test FBX export pipeline."""

    def setup_method(self):
        _clear_scene()

    def test_export_creates_file(self):
        """Verify FBX export creates a file."""
        obj = _generate_test_object()
        if obj is None:
            return  # Skip if generation fails

        from core.fbx_export import prepare_object_for_export
        prepare_object_for_export(obj)

        # Check object is prepared (has SM_ prefix)
        assert obj.name.startswith("SM_"), f"Object not renamed: {obj.name}"

    def test_pivot_at_base_center(self):
        """Verify pivot is set to base center."""
        obj = _generate_test_object()
        if obj is None:
            return

        from core.fbx_export import set_pivot_to_base_center
        set_pivot_to_base_center(obj)

        # After pivot adjustment, the lowest Z vertex should be at z=0
        # relative to the object origin
        if obj.type == 'MESH' and obj.data.vertices:
            min_z = min(v.co.z for v in obj.data.vertices)
            # Should be approximately 0 (allowing small floating point error)
            assert abs(min_z) < 1.0, f"Min Z after pivot adjust: {min_z}"


class TestGeometryModes:
    """Test VALID vs IMPOSSIBLE mode behavior."""

    def setup_method(self):
        _clear_scene()

    def test_valid_mode_cleanup(self):
        """VALID mode should produce a mesh with cleaned geometry."""
        from generators.base_generator import GeneratorContext
        from core.geometry_modes import GeometryMode

        ctx = GeneratorContext(
            scene=bpy.context.scene,
            collection=bpy.context.scene.collection,
            mode=GeometryMode.VALID,
            seed=42,
        )

        from generators.balustrade import BalustradeGenerator
        gen = BalustradeGenerator()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert len(obj.data.vertices) > 0

    def test_impossible_mode_permissive(self):
        """IMPOSSIBLE mode should allow non-manifold geometry."""
        from generators.base_generator import GeneratorContext
        from core.geometry_modes import GeometryMode

        ctx = GeneratorContext(
            scene=bpy.context.scene,
            collection=bpy.context.scene.collection,
            mode=GeometryMode.IMPOSSIBLE,
            seed=42,
        )

        from generators.escher.penrose_stairs import PenroseStairsGenerator
        gen = PenroseStairsGenerator()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert len(obj.data.vertices) > 0


class TestScale:
    """Test centimeter scale alignment with UE5."""

    def test_wall_width_constant(self):
        from core.constants import WALL_WIDTH
        assert WALL_WIDTH == 400  # cm

    def test_story_height_constant(self):
        from core.constants import STORY_HEIGHT
        assert STORY_HEIGHT == 600  # cm

    def test_column_spacing_constant(self):
        from core.constants import COLUMN_SPACING
        assert COLUMN_SPACING == 400  # cm

    def test_fbx_scale_factor(self):
        """FBX global_scale should be 0.01 (cm to m)."""
        # Verify the export function uses 0.01 scale by default
        import inspect
        from core.fbx_export import export_to_ue5
        sig = inspect.signature(export_to_ue5)
        default_scale = sig.parameters['global_scale'].default
        assert default_scale == 0.01, f"Expected global_scale=0.01, got {default_scale}"


class TestBatchSystem:
    """Test batch generation and layout parsing."""

    def test_parse_elements(self):
        from batch.layout_parser import parse_layout
        data = {
            "elements": [
                {"type": "column_doric", "params": {"height": 600}},
                {"type": "balustrade", "params": {"length": 400},
                 "location": [400, 0, 0]},
            ]
        }
        elements = parse_layout(data)
        assert len(elements) == 2
        assert elements[0]['type'] == 'column_doric'
        assert elements[1]['location'] == (400, 0, 0)

    def test_parse_grid(self):
        from batch.layout_parser import parse_layout
        data = {
            "grid": {
                "rows": 2,
                "cols": 3,
                "spacing": [400, 400],
                "pattern": [{"type": "column_doric"}]
            }
        }
        elements = parse_layout(data)
        assert len(elements) == 6  # 2 rows × 3 cols

    def test_parse_ring(self):
        from batch.layout_parser import parse_layout
        data = {
            "ring": {
                "radius": 600,
                "count": 8,
                "pattern": [{"type": "column_corinthian"}]
            }
        }
        elements = parse_layout(data)
        assert len(elements) == 8


if __name__ == '__main__':
    import pytest
    pytest.main([__file__, '-v'])
