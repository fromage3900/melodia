"""
Base generator with self-registration pattern.

All generators inherit BaseGenerator, define their parameters,
and self-register via @register_generator decorator.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import Any


class ParamType(Enum):
    FLOAT = 'FLOAT'
    INT = 'INT'
    ENUM = 'ENUM'
    BOOL = 'BOOL'
    FLOAT_VECTOR = 'FLOAT_VECTOR'


@dataclass
class ParameterDefinition:
    name: str
    param_type: ParamType
    default: Any = None
    min_val: float = 0.0
    max_val: float = 1000.0
    description: str = ''
    enum_items: list[tuple] = field(default_factory=list)


@dataclass
class GeneratorContext:
    mode: Any  # GeometryMode
    seed: int
    rng: Any  # random.Random
    collection: str = ''
    export_path: str = ''


_REGISTRY: dict[str, type['BaseGenerator']] = {}


def register_generator(cls: type['BaseGenerator']) -> type['BaseGenerator']:
    _REGISTRY[cls.generator_id] = cls
    return cls


def get_generator(gen_id: str) -> type['BaseGenerator'] | None:
    return _REGISTRY.get(gen_id)


def list_generators() -> list[type['BaseGenerator']]:
    return list(_REGISTRY.values())


def list_generators_by_category() -> dict[str, list[type['BaseGenerator']]]:
    result: dict[str, list] = {}
    for gen in _REGISTRY.values():
        result.setdefault(gen.category, []).append(gen)
    return result


class BaseGenerator(ABC):
    generator_id: str = ''
    generator_name: str = ''
    category: str = ''
    description: str = ''

    @classmethod
    def get_parameters(cls) -> list[ParameterDefinition]:
        return []

    @abstractmethod
    def _build(self, ctx: GeneratorContext, params: dict[str, Any]) -> Any:
        """Build and return a BMesh."""
        ...

    def generate(self, ctx: GeneratorContext, params: dict[str, Any]) -> Any:
        """Full generation pipeline."""
        bm = self._build(ctx, params)
        # Apply geometry mode
        from ..core.geometry_modes import apply_mode
        apply_mode(bm, ctx.mode)
        return bm

    def preview(self, ctx: GeneratorContext, params: dict[str, Any]):
        """Generate and create a Blender object for preview."""
        import bpy
        import bmesh
        from ..core.naming import make_generator_name
        bm = self.generate(ctx, params)
        mesh = bpy.data.meshes.new(make_generator_name(self.generator_id, ctx.seed))
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new(mesh.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        return obj
