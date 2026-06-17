"""
Organic profile curves for Melodia Art Nouveau Greybox.

Plant stems, petals, tendrils, lily pads, leaves, whiplash moldings.
All return list[(x, z)] tuples in the XZ plane for revolve/extrude operations.
"""

import math


def stem_profile(r_bottom: float, r_top: float, height: float,
                 nodes: int = 3, node_bulge: float = 0.15,
                 segments: int = 24) -> list[tuple[float, float]]:
    """
    Plant stem profile with node swellings.

    Args:
        r_bottom: Base radius
        r_top: Top radius
        height: Total shaft height
        nodes: Number of node swellings along shaft
        node_bulge: Extra radius at nodes (fraction of base radius)
        segments: Profile resolution
    """
    pts = []
    bulge = r_bottom * node_bulge
    for i in range(segments + 1):
        t = i / segments
        z = t * height
        # Linear taper
        r = r_bottom + (r_top - r_bottom) * t
        # Node swellings
        for n in range(nodes):
            node_t = (n + 1) / (nodes + 1)
            dist = abs(t - node_t)
            if dist < 0.08:
                r += bulge * (1.0 - dist / 0.08) ** 2
        pts.append((r, z))
    return pts


def petal_profile(width: float, length: float, curl: float = 0.4,
                  segments: int = 16) -> list[tuple[float, float]]:
    """
    Iris/lily petal cross-section.

    Args:
        width: Petal width at base
        length: Petal length
        curl: How much the petal curls back (0-1)
        segments: Resolution
    """
    pts = []
    for i in range(segments + 1):
        t = i / segments
        # Petal outline: starts narrow, widens, narrows again
        r = width * math.sin(math.pi * t)
        # Curl at tip
        z = t * length - curl * length * t * t * (1 - t)
        pts.append((r, z))
    return pts


def tendril_cross_section(radius: float, segments: int = 12) -> list[tuple[float, float]]:
    """Small circular profile for tendril sweep."""
    pts = []
    for i in range(segments + 1):
        angle = (i / segments) * 2 * math.pi
        pts.append((radius * math.cos(angle), radius * math.sin(angle)))
    return pts


def lily_pad_profile(radius: float, notch_angle: float = 30,
                     segments: int = 24) -> list[tuple[float, float]]:
    """
    Circular profile with V-notch (like a lily pad).
    Returns points in XZ plane.
    """
    notch_rad = math.radians(notch_angle)
    pts = []
    for i in range(segments + 1):
        angle = (i / segments) * 2 * math.pi
        # Skip the notch region
        if -notch_rad / 2 < angle < notch_rad / 2:
            continue
        r = radius
        x = r * math.cos(angle)
        z = r * math.sin(angle)
        pts.append((x, z))
    # Close the profile at the notch
    pts.append((radius * math.cos(notch_rad / 2), radius * math.sin(notch_rad / 2)))
    pts.append((radius * math.cos(-notch_rad / 2), radius * math.sin(-notch_rad / 2)))
    return pts


def leaf_profile(length: float, width: float, curl: float = 0.2,
                 segments: int = 16) -> list[tuple[float, float]]:
    """
    General leaf shape profile.

    Args:
        length: Leaf length
        width: Maximum leaf width
        curl: Edge curl amount
        segments: Resolution
    """
    pts = []
    for i in range(segments + 1):
        t = i / segments
        # Leaf outline: pointed at both ends, widest in middle
        r = width * math.sin(math.pi * t) * (1.0 - 0.3 * abs(t - 0.5))
        # Slight curl along edges
        z = t * length + curl * length * math.sin(2 * math.pi * t)
        pts.append((r, z))
    return pts


def whiplash_molding(depth: float, width: float, segments: int = 24) -> list[tuple[float, float]]:
    """
    S-curve molding profile (replaces classical ogee).

    Args:
        depth: Profile depth (Z extent)
        width: Profile width (X extent)
        segments: Resolution
    """
    pts = []
    for i in range(segments + 1):
        t = i / segments
        x = t * width
        # S-curve: concave then convex
        z = depth * (2 * t - 1) * t * (1 - t) * 4 + depth * t
        pts.append((x, z))
    return pts


def organic_column_profile(r_bot: float, r_top: float, height: float,
                           entasis: float = 0.05, segments: int = 24) -> list[tuple[float, float]]:
    """
    Subtle organic swelling (not classical entasis).
    Slight bulge in the middle, like a growing stem.
    """
    pts = []
    bulge = (r_bot + r_top) / 2 * entasis
    for i in range(segments + 1):
        t = i / segments
        z = t * height
        # Linear taper
        r = r_bot + (r_top - r_bot) * t
        # Organic bulge in middle
        r += bulge * math.sin(math.pi * t)
        pts.append((r, z))
    return pts


def acorn_profile(radius: float, height: float, segments: int = 16) -> list[tuple[float, float]]:
    """Acorn/seed pod shape for finials and decorative caps."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        z = t * height
        # Acorn: narrow base, bulging middle, pointed tip
        r = radius * math.sin(math.pi * t) * (1.0 + 0.3 * (1 - t))
        pts.append((r, z))
    return pts
