"""Geometry modes: VALID (manifold) vs IMPOSSIBLE (non-manifold allowed)."""

from enum import Enum


class GeometryMode(Enum):
    VALID = 'VALID'
    IMPOSSIBLE = 'IMPOSSIBLE'


def apply_mode(bm, mode: GeometryMode):
    """Apply geometry mode post-processing."""
    from . import mesh_cleanup
    if mode == GeometryMode.VALID:
        mesh_cleanup.full_cleanup(bm)
    # IMPOSSIBLE mode: skip cleanup, allow self-intersections


def is_valid_mode(mode: GeometryMode) -> bool:
    return mode == GeometryMode.VALID
