"""
UI utility functions — dynamic parameter widget builder.

Builds Blender UI layout from generator ParameterDefinitions.
"""

import bpy
from ..generators.base_generator import ParamType


def draw_generator_params(layout, generator_cls, storage, prefix=""):
    """
    Draw all parameters for a generator into a UI layout.

    Args:
        layout: bpy.types.UILayout to draw into
        generator_cls: generator class with get_parameters()
        storage: property group or dict holding current values
        prefix: prefix for property names
    """
    params = generator_cls.get_parameters()
    current_category = None
    box = None

    for param_def in params:
        # Category header
        if param_def.category != current_category:
            current_category = param_def.category
            box = layout.box()
            box.label(text=current_category, icon='PREFERENCES')

        prop_name = f"{prefix}{param_def.name}" if prefix else param_def.name

        if param_def.param_type == ParamType.BOOL:
            box.prop(storage, prop_name, text=param_def.display_name)
        elif param_def.param_type == ParamType.INT:
            col = box.column(align=True)
            col.prop(storage, prop_name, text=param_def.display_name)
            if param_def.min_val is not None and param_def.max_val is not None:
                col.label(text=f"Range: {int(param_def.min_val)} – {int(param_def.max_val)}")
        elif param_def.param_type == ParamType.FLOAT:
            col = box.column(align=True)
            col.prop(storage, prop_name, text=param_def.display_name)
            if param_def.description:
                col.label(text=param_def.description)
        elif param_def.param_type == ParamType.ENUM:
            box.prop(storage, prop_name, text=param_def.display_name)
        elif param_def.param_type == ParamType.FLOAT_VECTOR:
            box.prop(storage, prop_name, text=param_def.display_name)


def register_generator_properties(generator_cls, prefix=""):
    """
    Create bpy.props for all parameters of a generator.
    Returns a dict of property definitions to annotate onto a PropertyGroup.
    """
    props = {}
    for param_def in generator_cls.get_parameters():
        prop_name = f"{prefix}{param_def.name}" if prefix else param_def.name

        if param_def.param_type == ParamType.BOOL:
            props[prop_name] = bpy.props.BoolProperty(
                name=param_def.display_name,
                default=bool(param_def.default),
                description=param_def.description,
            )
        elif param_def.param_type == ParamType.INT:
            props[prop_name] = bpy.props.IntProperty(
                name=param_def.display_name,
                default=int(param_def.default),
                min=int(param_def.min_val) if param_def.min_val is not None else -99999,
                max=int(param_def.max_val) if param_def.max_val is not None else 99999,
                description=param_def.description,
            )
        elif param_def.param_type == ParamType.FLOAT:
            props[prop_name] = bpy.props.FloatProperty(
                name=param_def.display_name,
                default=float(param_def.default),
                min=param_def.min_val if param_def.min_val is not None else -99999.0,
                max=param_def.max_val if param_def.max_val is not None else 99999.0,
                description=param_def.description,
            )
        elif param_def.param_type == ParamType.ENUM:
            items = []
            for item in param_def.enum_items:
                if len(item) == 3:
                    items.append((item[0], item[1], item[2]))
                elif len(item) == 2:
                    items.append((item[0], item[1], ""))
            props[prop_name] = bpy.props.EnumProperty(
                name=param_def.display_name,
                items=items,
                default=param_def.default,
                description=param_def.description,
            )

    return props


def collect_params_from_storage(storage, generator_cls, prefix=""):
    """
    Read current parameter values from a property group.
    Returns a dict of {param_name: value}.
    """
    params = {}
    for param_def in generator_cls.get_parameters():
        prop_name = f"{prefix}{param_def.name}" if prefix else param_def.name
        if hasattr(storage, prop_name):
            params[param_def.name] = getattr(storage, prop_name)
        else:
            params[param_def.name] = param_def.default
    return params


def draw_section_header(layout, text, icon='MESH_CUBE'):
    """Draw a section header with icon."""
    row = layout.row()
    row.label(text=text, icon=icon)
    return row


def draw_generate_button(layout, operator_name, generator_id, text="Generate"):
    """Draw a generate button for a specific generator."""
    op = layout.operator(operator_name, text=text, icon='PLAY')
    op.generator_id = generator_id
    return op
