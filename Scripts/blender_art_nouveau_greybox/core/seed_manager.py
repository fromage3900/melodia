"""Seed management for deterministic generation."""

import random


def create_rng(seed: int = 42) -> random.Random:
    """Create a seeded RNG instance."""
    return random.Random(seed)


def hash_seed(*args) -> int:
    """Create a deterministic seed from multiple values."""
    h = 0
    for a in args:
        h = (h * 31 + hash(a)) & 0xFFFFFFFF
    return h
