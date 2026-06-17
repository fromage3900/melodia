"""UI utilities: dynamic parameter widgets, property registration."""

import bpy


def draw_generator_params(layout, generator_cls, storage, prefix):
    """Build UI layout from ParameterDefinitions with category grouping."""
    params = generator_cls.get_parameters()
    for p in params:
        prop_name = f"{prefix}_{p.name}"
        if p.param_type.value == 'BOOL':
            layout.prop(storage, prop_name, text=p.name.replace('_', ' ').title())
        elif p.param_type.value == 'ENUM':
            layout.prop(storage, prop_name, text=p.name.replace('_', ' ').title())
        elif p.param_type.value == 'INT':
            layout.prop(storage, prop_name, text=p.name.replace('_', ' ').title())
        elif p.param_type.value == 'FLOAT':
            layout.prop(storage, prop_name, text=p.name.replace('_', ' ').title())
        elif p.param_type.value == 'FLOAT_VECTOR':
            layout.prop(storage, prop_name, text=p.name.replace('_', ' ').title())


def register_generator_properties(generator_cls, prefix):
    """Create bpy.props from ParameterDefinitions."""
    props = {}
    for p in generator_cls.get_parameters():
        prop_name = f"{prefix}_{p.name}"
        if p.param_type.value == 'BOOL':
            props[prop_name] = bpy.props.BoolProperty(name=p.name, default=p.default, description=p.description)
        elif p.param_type.value == 'ENUM':
            props[prop_name] = bpy.props.EnumProperty(name=p.name, items=p.enum_items, default=p.default, description=p.description)
        elif p.param_type.value == 'INT':
            props[prop_name] = bpy.props.IntProperty(name=p.name, default=int(p.default), min=int(p.min_val), max=int(p.max_val), description=p.description)
        elif p.param_type.value == 'FLOAT':
            props[prop_name] = bpy.props.FloatProperty(name=p.name, default=p.default, min=p.min_val, max=p.max_val, description=p.description)
        elif p.param_type.value == 'FLOAT_VECTOR':
            props[prop_name] = bpy.props.FloatVectorProperty(name=p.name, default=p.default, description=p.description)
    return props


def collect_params_from_storage(storage, generator_cls, prefix):
    """Read current values from property groups."""
    params = {}
    for p in generator_cls.get_parameters():
        prop_name = f"{prefix}_{p.name}"
        params[p.name] = getattr(storage, prop_name, p.default)
    return params
