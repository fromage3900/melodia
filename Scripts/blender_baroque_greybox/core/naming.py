"""
Naming utilities for UE5 static mesh convention.
"""

import re


def enforce_sm_prefix(name: str) -> str:
    """Ensure name starts with SM_ prefix (UE5 static mesh convention)."""
    # Strip existing prefix variants
    clean = re.sub(r'^(SM_)?', '', name)
    # Remove forbidden characters
    clean = re.sub(r'[^a-zA-Z0-9_]', '', clean)
    # Remove leading/trailing underscores
    clean = clean.strip('_')
    return f"SM_{clean}"


def unique_name(base_name: str, existing_names: set) -> str:
    """Generate a unique name by appending a counter if needed."""
    if base_name not in existing_names:
        return base_name
    counter = 1
    while f"{base_name}_{counter:03d}" in existing_names:
        counter += 1
    return f"{base_name}_{counter:03d}"


def sanitize_name(name: str) -> str:
    """Remove forbidden characters from asset name."""
    # No spaces, no diacritics, no _BS/_BSS suffix
    clean = re.sub(r'[^a-zA-Z0-9_]', '', name)
    clean = re.sub(r'_BS(S)?$', '', clean)
    clean = re.sub(r'_+$', '', clean)
    return clean


def make_generator_name(generator_type: str, variant: str,
                        seed: int = 0) -> str:
    """Create a standardized mesh name from generator info."""
    parts = [generator_type]
    if variant:
        parts.append(variant)
    if seed:
        parts.append(f"s{seed}")
    base = "_".join(parts)
    return enforce_sm_prefix(sanitize_name(base))
