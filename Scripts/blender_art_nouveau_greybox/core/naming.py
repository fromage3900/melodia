"""Naming utilities: SM_ prefix, unique names, sanitized identifiers."""


def enforce_sm_prefix(name: str) -> str:
    if not name.startswith('SM_'):
        return f'SM_{name}'
    return name


def unique_name(base: str, existing: set[str]) -> str:
    name = base
    counter = 1
    while name in existing:
        name = f'{base}_{counter:03d}'
        counter += 1
    return name


def sanitize_name(name: str) -> str:
    return ''.join(c if c.isalnum() or c == '_' else '_' for c in name).strip('_')


def make_generator_name(gen_id: str, seed: int = 0) -> str:
    return sanitize_name(f'{gen_id}_seed{seed}')
