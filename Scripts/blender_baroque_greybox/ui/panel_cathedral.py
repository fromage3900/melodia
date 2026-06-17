"""Panel for cathedral generator."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_cathedral(bpy.types.Panel):
    bl_label = "Cathedral"
    bl_idname = "MELODIA_PT_cathedral"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        storage = getattr(scene, "melodia_cathedral", None)
        if storage:
            from ..generators.cathedral import CathedralGenerator
            draw_generator_params(layout, CathedralGenerator, storage)

        layout.separator()
        op = layout.operator("melodia.generate", text="Generate Cathedral", icon='PLAY')
        op.generator_id = "cathedral"


def register():
    bpy.utils.register_class(MELODIA_PT_cathedral)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_cathedral)
