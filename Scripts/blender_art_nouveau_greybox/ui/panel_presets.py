"""Presets panel."""

import bpy
from ..presets.preset_manager import list_builtin_presets


class MELNOUVEAU_PT_presets(bpy.types.Panel):
    bl_label = "Presets"
    bl_idname = "MELNOUVEAU_PT_presets"
    bl_parent_id = "MELNOUVEAU_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        presets = list_builtin_presets()
        for name in presets:
            layout.operator("melodia_nouveau.load_preset", text=name).preset_name = name


class MELNOUVEAU_OT_load_preset(bpy.types.Operator):
    bl_idname = "melodia_nouveau.load_preset"
    bl_label = "Load Preset"
    preset_name: bpy.props.StringProperty()

    def execute(self, context):
        self.report({'INFO'}, f"Loaded preset: {self.preset_name}")
        return {'FINISHED'}


_classes = [MELNOUVEAU_OT_load_preset, MELNOUVEAU_PT_presets]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
