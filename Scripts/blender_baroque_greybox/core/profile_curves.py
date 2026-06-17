"""
2D profile curves for baroque architectural elements.

All profiles return lists of (x, z) tuples suitable for revolution (lathe)
or extrusion along a path. Coordinates in centimeters.
"""

import math
from .math_utils import lerp, smoothstep


def _normalize_profile(pts, height=1.0):
    """Normalize profile to given height range."""
    if not pts:
        return pts
    min_z = min(z for _, z in pts)
    max_z = max(z for _, z in pts)
    rng = max_z - min_z
    if rng < 1e-12:
        return pts
    return [(x, (z - min_z) / rng * height) for x, z in pts]


# ---------------------------------------------------------------------------
# Classical molding profiles
# ---------------------------------------------------------------------------

def ogee(radius=10.0, height=20.0, segments=16):
    """Ogee (S-curve) profile — convex over concave."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        # S-curve: two arcs
        if t < 0.5:
            angle = math.pi * (1.0 - t * 2.0)
            x = radius * (1.0 - math.cos(angle))
            z = height * 0.5 * (1.0 - math.sin(angle))
        else:
            angle = math.pi * ((t - 0.5) * 2.0)
            x = radius * math.cos(angle)
            z = height * 0.5 + height * 0.5 * math.sin(angle)
        pts.append((x, z))
    return pts


def cavetto(radius=10.0, height=15.0, segments=12):
    """Cavetto — concave quarter-round profile."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * 0.5 * t
        x = radius * math.cos(angle)
        z = height * (1.0 - math.sin(angle))
        pts.append((x, z))
    return pts


def torus_profile(major_r=15.0, minor_r=5.0, segments=16):
    """Torus cross-section (half) for column bases."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * t  # half circle
        x = major_r - minor_r * math.cos(angle)
        z = minor_r * math.sin(angle)
        pts.append((x, z))
    return pts


def scotia(major_r=12.0, minor_r=6.0, segments=12):
    """Scotia — concave molding profile."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * 0.5 * t
        x = major_r - minor_r * (1.0 - math.cos(angle))
        z = -minor_r * math.sin(angle)
        pts.append((x, z))
    return pts


def fillet(length=5.0):
    """Simple straight fillet (small flat)."""
    return [(0.0, 0.0), (length, 0.0), (length, length), (0.0, length)]


def cyma_recta(width=12.0, height=20.0, segments=16):
    """Cyma recta — S-curve starting vertical, ending horizontal."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        if t < 0.5:
            angle = math.pi * 0.5 * (t * 2.0)
            x = width * 0.5 * (1.0 - math.cos(angle))
            z = height * 0.5 * math.sin(angle)
        else:
            angle = math.pi * 0.5 * ((t - 0.5) * 2.0)
            x = width * 0.5 + width * 0.5 * math.sin(angle)
            z = height * 0.5 + height * 0.5 * (1.0 - math.cos(angle))
        pts.append((x, z))
    return pts


def cyma_reversa(width=12.0, height=20.0, segments=16):
    """Cyma reversa — S-curve starting horizontal, ending vertical."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        if t < 0.5:
            angle = math.pi * 0.5 * (t * 2.0)
            x = width * 0.5 * math.sin(angle)
            z = height * 0.5 * (1.0 - math.cos(angle))
        else:
            angle = math.pi * 0.5 * ((t - 0.5) * 2.0)
            x = width * 0.5 + width * 0.5 * (1.0 - math.cos(angle))
            z = height * 0.5 + height * 0.5 * math.sin(angle)
        pts.append((x, z))
    return pts


# ---------------------------------------------------------------------------
# Ornamental patterns
# ---------------------------------------------------------------------------

def dentil_pattern(tooth_width=8.0, gap_width=6.0, tooth_height=12.0,
                   count=10):
    """Dentil molding pattern — repeating rectangular teeth."""
    pts = []
    for i in range(count):
        x_start = i * (tooth_width + gap_width)
        # Bottom of gap
        pts.append((x_start, 0.0))
        # Rise of tooth
        pts.append((x_start, tooth_height))
        # Top of tooth
        pts.append((x_start + tooth_width, tooth_height))
        # Fall of tooth
        pts.append((x_start + tooth_width, 0.0))
    # Close
    total_w = count * (tooth_width + gap_width) - gap_width
    pts.append((total_w, 0.0))
    return pts


def egg_and_dart(egg_width=12.0, dart_width=6.0, height=20.0, count=8,
                 segments=8):
    """Egg-and-dart repeating pattern."""
    pts = []
    period = egg_width + dart_width
    for i in range(count):
        x_base = i * period
        # Egg (half-ellipse)
        for j in range(segments + 1):
            t = j / segments
            angle = math.pi * t
            x = x_base + egg_width * 0.5 * (1.0 - math.cos(angle))
            z = height * math.sin(angle)
            pts.append((x, z))
        # Dart (V-notch)
        x_dart = x_base + egg_width
        pts.append((x_dart + dart_width * 0.5, 0.0))
        pts.append((x_dart + dart_width, height * 0.3))
    return pts


def bead_and_reel(bead_r=5.0, reel_h=8.0, count=6, segments=8):
    """Bead-and-reel alternating pattern."""
    pts = []
    for i in range(count):
        x_base = i * (bead_r * 2 + reel_h)
        # Bead (half-circle)
        for j in range(segments + 1):
            t = j / segments
            angle = math.pi * t
            x = x_base + bead_r * (1.0 - math.cos(angle))
            z = bead_r * math.sin(angle)
            pts.append((x, z))
        # Reel (rectangle)
        x_reel = x_base + bead_r * 2
        pts.append((x_reel, 0.0))
        pts.append((x_reel, bead_r))
        pts.append((x_reel + reel_h, bead_r))
        pts.append((x_reel + reel_h, 0.0))
    return pts


# ---------------------------------------------------------------------------
# Column shaft profiles
# ---------------------------------------------------------------------------

def column_shaft_profile(radius_bottom=30.0, radius_top=25.0,
                         height=600.0, entasis=0.02, segments=32):
    """Column shaft profile with optional entasis (subtle swelling)."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        z = height * t
        # Linear taper
        r = lerp(radius_bottom, radius_top, t)
        # Entasis: subtle convex swelling at ~1/3 height
        if entasis > 0:
            swell = entasis * radius_bottom * math.sin(math.pi * t)
            r += swell
        pts.append((r, z))
    return pts


def baluster_turned(height=80.0, radius_max=8.0, radius_min=4.0,
                    segments=32):
    """Baroque turned baluster profile (stacked bulges)."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        z = height * t
        # Base and cap: narrow
        # Middle sections: bulging
        if t < 0.1:
            r = lerp(radius_min * 1.2, radius_max * 0.6, t / 0.1)
        elif t < 0.3:
            lt = (t - 0.1) / 0.2
            r = lerp(radius_max * 0.6, radius_max, math.sin(lt * math.pi * 0.5))
        elif t < 0.5:
            lt = (t - 0.3) / 0.2
            r = lerp(radius_max, radius_min * 0.8, math.sin(lt * math.pi * 0.5))
        elif t < 0.7:
            lt = (t - 0.5) / 0.2
            r = lerp(radius_min * 0.8, radius_max * 0.9, math.sin(lt * math.pi * 0.5))
        elif t < 0.9:
            lt = (t - 0.7) / 0.2
            r = lerp(radius_max * 0.9, radius_max * 0.5, math.sin(lt * math.pi * 0.5))
        else:
            lt = (t - 0.9) / 0.1
            r = lerp(radius_max * 0.5, radius_min * 1.2, lt)
        pts.append((r, z))
    return pts


def baluster_simple(height=80.0, radius_bottom=6.0, radius_top=5.0,
                    segments=16):
    """Simple tapered baluster."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        z = height * t
        r = lerp(radius_bottom, radius_top, t)
        pts.append((r, z))
    return pts


def baluster_twisted(height=80.0, radius=6.0, twist=2.0,
                     segments=48):
    """Twisted/helical baluster profile (returns 3D points)."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        z = height * t
        angle = t * math.pi * 2.0 * twist
        # Modulate radius for spiral effect
        r = radius * (0.7 + 0.3 * math.sin(angle))
        pts.append((r, z))
    return pts


# ---------------------------------------------------------------------------
# Arch profiles
# ---------------------------------------------------------------------------

def semicircular_arch(radius=100.0, segments=24):
    """Semicircular arch profile."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * t
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        pts.append((x, z))
    return pts


def pointed_arch(radius=100.0, height=140.0, segments=24):
    """Gothic pointed arch profile."""
    pts = []
    half = segments // 2
    # Left side
    for i in range(half + 1):
        t = i / half
        angle = math.pi * 0.5 * t
        x = radius * (1.0 - math.cos(angle))
        z = height * math.sin(angle)
        pts.append((x - radius * 0.5, z))
    # Right side (mirror)
    for i in range(half + 1):
        t = i / half
        angle = math.pi * 0.5 * (1.0 - t)
        x = radius * (1.0 - math.cos(angle))
        z = height * math.sin(angle)
        pts.append((radius * 0.5 - x, z))
    return pts


def basket_arch(radius=100.0, rise=60.0, segments=24):
    """Basket-handle (three-centered) arch profile."""
    pts = []
    for i in range(segments + 1):
        t = i / segments
        angle = math.pi * t
        x = radius * math.cos(angle)
        z = rise * math.sin(angle)
        pts.append((x, z))
    return pts
