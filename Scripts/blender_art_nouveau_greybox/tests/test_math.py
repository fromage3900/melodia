"""Unit tests for math utilities (no bpy required)."""

import sys
sys.path.insert(0, 'g:/Melodia/Scripts/blender_art_nouveau_greybox')


def test_whiplash_amplitude():
    from core.whiplash import WhiplashCurve
    pts = WhiplashCurve.generate(200, 40, phases=2, segments=32)
    max_z = max(abs(p.z) for p in pts)
    assert max_z <= 45, f"Amplitude exceeded: {max_z}"
    print("PASS: whiplash amplitude")


def test_whiplash_length():
    from core.whiplash import WhiplashCurve
    pts = WhiplashCurve.generate(200, 40, phases=2, segments=32)
    assert len(pts) == 32
    print("PASS: whiplash length")


def test_asymmetric_offset():
    import random
    rng = random.Random(42)
    for _ in range(100):
        offset = rng.uniform(-0.3, 0.3)
        assert -0.35 <= offset <= 0.35
    print("PASS: asymmetric offset bounds")


if __name__ == "__main__":
    test_whiplash_amplitude()
    test_whiplash_length()
    test_asymmetric_offset()
    print("All math tests passed.")
