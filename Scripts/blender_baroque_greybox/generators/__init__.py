"""
Generator registry — imports all generators so they self-register.
"""

from .base_generator import (
    BaseGenerator, GeneratorContext, ParameterDefinition, ParamType,
    register_generator, get_generator, list_generators,
    list_generators_by_category,
)
