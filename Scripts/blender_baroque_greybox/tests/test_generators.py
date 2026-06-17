"""
Smoke tests for all generators — verifies each generator can produce
a non-empty mesh without errors. Must be run inside Blender.

Usage (from Blender Python or command line):
    blender --background --python -c "import pytest; pytest.main(['tests/', '-v'])"
"""

import bpy
import bmesh
import sys
import os

# Ensure addon is importable
addon_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if addon_dir not in sys.path:
    sys.path.insert(0, os.path.dirname(addon_dir))


def _make_context():
    """Create a test GeneratorContext."""
    from generators.base_generator import GeneratorContext
    from core.geometry_modes import GeometryMode
    return GeneratorContext(
        scene=bpy.context.scene,
        collection=bpy.context.scene.collection,
        mode=GeometryMode.VALID,
        seed=42,
    )


def _make_impossible_context():
    """Create an IMPOSSIBLE mode GeneratorContext."""
    from generators.base_generator import GeneratorContext
    from core.geometry_modes import GeometryMode
    return GeneratorContext(
        scene=bpy.context.scene,
        collection=bpy.context.scene.collection,
        mode=GeometryMode.IMPOSSIBLE,
        seed=42,
    )


def _clear_scene():
    """Remove all mesh objects from scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def _count_verts(obj):
    """Count vertices in an object."""
    if obj and obj.type == 'MESH':
        return len(obj.data.vertices)
    return 0


# ---------------------------------------------------------------------------
# Generator smoke tests
# ---------------------------------------------------------------------------

class TestColumnGenerators:
    """Smoke test all 5 column orders."""

    def setup_method(self):
        _clear_scene()

    def test_doric(self):
        from generators.columns.doric import DoricColumnGenerator
        gen = DoricColumnGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_ionic(self):
        from generators.columns.ionic import IonicColumnGenerator
        gen = IonicColumnGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_corinthian(self):
        from generators.columns.corinthian import CorinthianColumnGenerator
        gen = CorinthianColumnGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_composite(self):
        from generators.columns.composite import CompositeColumnGenerator
        gen = CompositeColumnGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_solomonic(self):
        from generators.columns.solomonic import SolomonicColumnGenerator
        gen = SolomonicColumnGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0


class TestMoldingGenerators:
    """Smoke test all 4 molding generators."""

    def setup_method(self):
        _clear_scene()

    def test_architrave(self):
        from generators.moldings.architrave import ArchitraveGenerator
        gen = ArchitraveGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_frieze(self):
        from generators.moldings.frieze import FriezeGenerator
        gen = FriezeGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_cornice(self):
        from generators.moldings.cornice import CorniceGenerator
        gen = CorniceGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_base_molding(self):
        from generators.moldings.base_molding import BaseMoldingGenerator
        gen = BaseMoldingGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0


class TestOrnamentGenerators:
    """Smoke test all 6 ornament generators."""

    def setup_method(self):
        _clear_scene()

    def _test_ornament(self, module_path, class_name):
        import importlib
        mod = importlib.import_module(module_path)
        cls = getattr(mod, class_name)
        gen = cls()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_volute(self):
        self._test_ornament('generators.ornaments.volute', 'VoluteGenerator')

    def test_acanthus(self):
        self._test_ornament('generators.ornaments.acanthus', 'AcanthusGenerator')

    def test_rose_window(self):
        self._test_ornament('generators.ornaments.rose_window', 'RoseWindowGenerator')

    def test_cartouche(self):
        self._test_ornament('generators.ornaments.cartouche', 'CartoucheGenerator')

    def test_finial(self):
        self._test_ornament('generators.ornaments.finial', 'FinialGenerator')

    def test_shell(self):
        self._test_ornament('generators.ornaments.shell', 'ShellGenerator')


class TestArchitectureGenerators:
    """Smoke test balustrade, cathedral, facade, curve architecture."""

    def setup_method(self):
        _clear_scene()

    def test_balustrade(self):
        from generators.balustrade import BalustradeGenerator
        gen = BalustradeGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_cathedral(self):
        from generators.cathedral import CathedralGenerator
        gen = CathedralGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_facade(self):
        from generators.facade import FacadeGenerator
        gen = FacadeGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_curve_architecture(self):
        from generators.curve_architecture import CurveArchitectureGenerator
        gen = CurveArchitectureGenerator()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0


class TestVaultGenerators:
    """Smoke test all 5 vault generators."""

    def setup_method(self):
        _clear_scene()

    def _test_vault(self, module_path, class_name):
        import importlib
        mod = importlib.import_module(module_path)
        cls = getattr(mod, class_name)
        gen = cls()
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

    def test_barrel_vault(self):
        self._test_vault('generators.vaults.barrel_vault', 'BarrelVaultGenerator')

    def test_groin_vault(self):
        self._test_vault('generators.vaults.groin_vault', 'GroinVaultGenerator')

    def test_ribbed_vault(self):
        self._test_vault('generators.vaults.ribbed_vault', 'RibbedVaultGenerator')

    def test_dome(self):
        self._test_vault('generators.vaults.dome', 'DomeGenerator')

    def test_coffered(self):
        self._test_vault('generators.vaults.coffered', 'CofferedVaultGenerator')


class TestEscherGenerators:
    """Smoke test all 6 Escher generators in both modes."""

    def setup_method(self):
        _clear_scene()

    def _test_escher(self, module_path, class_name):
        import importlib
        mod = importlib.import_module(module_path)
        cls = getattr(mod, class_name)
        gen = cls()

        # Test in VALID mode
        ctx = _make_context()
        obj = gen.generate({}, ctx)
        assert obj is not None
        assert _count_verts(obj) > 0

        # Test in IMPOSSIBLE mode
        _clear_scene()
        ctx = _make_impossible_context()
        obj2 = gen.generate({}, ctx)
        assert obj2 is not None
        assert _count_verts(obj2) > 0

    def test_penrose_stairs(self):
        self._test_escher('generators.escher.penrose_stairs', 'PenroseStairsGenerator')

    def test_mobius_walkway(self):
        self._test_escher('generators.escher.mobius_walkway', 'MobiusWalkwayGenerator')

    def test_impossible_bridge(self):
        self._test_escher('generators.escher.impossible_bridge', 'ImpossibleBridgeGenerator')

    def test_gravity_platform(self):
        self._test_escher('generators.escher.gravity_platform', 'GravityPlatformGenerator')

    def test_recursive_arches(self):
        self._test_escher('generators.escher.recursive_arches', 'RecursiveArchesGenerator')

    def test_klein_volume(self):
        self._test_escher('generators.escher.klein_volume', 'KleinVolumeGenerator')


class TestGeneratorRegistry:
    """Test the generator registry system."""

    def test_all_generators_registered(self):
        from generators.base_generator import list_generators
        # Import all generators to trigger registration
        import generators.columns.doric
        import generators.columns.ionic
        import generators.columns.corinthian
        import generators.columns.composite
        import generators.columns.solomonic
        import generators.moldings.architrave
        import generators.moldings.frieze
        import generators.moldings.cornice
        import generators.moldings.base_molding
        import generators.ornaments.volute
        import generators.ornaments.acanthus
        import generators.ornaments.rose_window
        import generators.ornaments.cartouche
        import generators.ornaments.finial
        import generators.ornaments.shell
        import generators.balustrade
        import generators.cathedral
        import generators.facade
        import generators.curve_architecture
        import generators.vaults.barrel_vault
        import generators.vaults.groin_vault
        import generators.vaults.ribbed_vault
        import generators.vaults.dome
        import generators.vaults.coffered
        import generators.escher.penrose_stairs
        import generators.escher.mobius_walkway
        import generators.escher.impossible_bridge
        import generators.escher.gravity_platform
        import generators.escher.recursive_arches
        import generators.escher.klein_volume

        registry = list_generators()
        # Should have at least 25 generators
        assert len(registry) >= 25, f"Expected >=25 generators, got {len(registry)}"

    def test_categories(self):
        from generators.base_generator import list_generators_by_category
        cats = list_generators_by_category()
        # Should have multiple categories
        assert len(cats) >= 3, f"Expected >=3 categories, got {len(cats)}"


if __name__ == '__main__':
    import pytest
    pytest.main([__file__, '-v'])
