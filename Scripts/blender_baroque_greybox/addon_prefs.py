"""Addon preferences: export path, default seed, unit scale."""

import bpy
from bpy.types import AddonPreferences
from bpy.props import StringProperty, IntProperty, FloatProperty, EnumProperty


class MELBAROQUE_Preferences(AddonPreferences):
    bl_idname = __package__  # matches the addon package name

    export_directory: StringProperty(
        name="UE5 Export Directory",
        description="Default directory for FBX export (UE5 Content browser)",
        subtype='DIR_PATH',
        default="",
    )

    default_seed: IntProperty(
        name="Default Seed",
        description="Default random seed for generators",
        default=42,
        min=0,
        max=99999,
    )

    unit_scale: FloatProperty(
        name="Unit Scale",
        description="Scale factor for FBX export (0.01 = Blender meters to UE5 cm)",
        default=0.01,
        min=0.001,
        max=1.0,
        precision=4,
    )

    default_mode: EnumProperty(
        name="Default Geometry Mode",
        items=[
            ('VALID', "Valid", "Watertight, exportable manifold mesh"),
            ('IMPOSSIBLE', "Impossible", "Non-manifold, Escher-style geometry"),
        ],
        default='VALID',
    )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "export_directory")
        layout.prop(self, "default_seed")
        layout.prop(self, "unit_scale")
        layout.prop(self, "default_mode")


_classes = [MELBAROQUE_Preferences]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
