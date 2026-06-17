"""
Abstract base generator class and generator registry.

All architectural generators inherit from BaseGenerator and register
themselves in the central registry for UI discovery.
"""

import bpy
import bmesh
from abc import ABC, abstractmethod
from enum import Enum
from dataclasses import dataclass, field
from typing import List, Any, Optional
from ..core.seed_manager import create_rng
from ..core.geometry_modes import GeometryMode, apply_mode
from ..core.mesh_builder import create_bmesh, create_object_from_bmesh
from ..core.naming import make_generator_name


# ---------------------------------------------------------------------------
# Parameter definition
# ---------------------------------------------------------------------------

class ParamType(Enum):
    FLOAT = "float"
    INT = "int"
    ENUM = "enum"
    BOOL = "bool"
    FLOAT_VECTOR = "float_vector"


@dataclass
class ParameterDefinition:
    """Describes one user-tunable generator parameter."""
    name: str
    display_name: str
    param_type: ParamType
    default: Any
    min_val: Optional[float] = None
    max_val: Optional[float] = None
    description: str = ""
    category: str = "General"
    enum_items: list = field(default_factory=list)  # for ENUM type


# ---------------------------------------------------------------------------
# Generator context
# ---------------------------------------------------------------------------

@dataclass
class GeneratorContext:
    """Context passed to every generator."""
    scene: Any = None  # bpy.types.Scene
    collection: Any = None  # bpy.types.Collection
    mode: GeometryMode = GeometryMode.VALID
    seed: int = 42
    rng: Any = None  # random.Random

    def __post_init__(self):
        if self.rng is None:
            self.rng = create_rng(self.seed)


# ---------------------------------------------------------------------------
# Generator registry
# ---------------------------------------------------------------------------

_REGISTRY = {}


def register_generator(cls):
    """Register a generator class in the global registry."""
    _REGISTRY[cls.generator_id] = cls
    return cls


def get_generator(generator_id: str):
    """Get a generator class by ID."""
    return _REGISTRY.get(generator_id)


def list_generators() -> dict:
    """Return the full generator registry."""
    return dict(_REGISTRY)


def list_generators_by_category() -> dict:
    """Return generators grouped by category."""
    cats = {}
    for gid, cls in _REGISTRY.items():
        cat = cls.category
        if cat not in cats:
            cats[cat] = []
        cats[cat].append(cls)
    return cats


# ---------------------------------------------------------------------------
# Base generator
# ---------------------------------------------------------------------------

class BaseGenerator(ABC):
    """
    Abstract base class for all architectural generators.

    Subclasses must implement:
        - generator_id: str (unique identifier)
        - generator_name: str (display name)
        - category: str (UI grouping)
        - get_parameters() -> list[ParameterDefinition]
        - _build(bm, params, ctx) -> None (geometry construction)
    """

    generator_id: str = "base"
    generator_name: str = "Base Generator"
    category: str = "General"
    description: str = ""

    @classmethod
    def get_parameters(cls) -> List[ParameterDefinition]:
        """Return list of tunable parameters for this generator."""
        return []

    @abstractmethod
    def _build(self, bm: bmesh.types.BMesh, params: dict,
               ctx: GeneratorContext) -> None:
        """Build geometry into the BMesh. Override in subclasses."""
        pass

    def generate(self, params: dict, ctx: GeneratorContext) -> bpy.types.Object:
        """
        Full generation pipeline:
        1. Create BMesh
        2. Call _build()
        3. Apply geometry mode
        4. Create Blender object
        """
        bm = create_bmesh()

        try:
            # Build geometry
            self._build(bm, params, ctx)

            # Apply geometry mode (VALID cleanup or IMPOSSIBLE permissive)
            apply_mode(bm, ctx.mode)

            # Create object
            obj_name = make_generator_name(
                self.generator_id,
                params.get('variant', ''),
                ctx.seed
            )
            obj = create_object_from_bmesh(bm, obj_name, ctx.collection)

            return obj
        finally:
            bm.free()

    def preview(self, params: dict, ctx: GeneratorContext) -> Optional[bpy.types.Object]:
        """Quick low-poly preview (default: same as generate)."""
        return self.generate(params, ctx)
