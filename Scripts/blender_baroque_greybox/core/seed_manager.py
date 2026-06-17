"""Deterministic seeded RNG factory."""

import random


def create_rng(seed: int) -> random.Random:
    """Create a seeded random.Random instance for deterministic generation."""
    rng = random.Random()
    rng.seed(seed)
    return rng


def hash_seed(*args) -> int:
    """Combine multiple values into a single deterministic seed."""
    h = 0
    for a in args:
        h = (h * 31 + hash(a)) & 0x7FFFFFFF
    return h
