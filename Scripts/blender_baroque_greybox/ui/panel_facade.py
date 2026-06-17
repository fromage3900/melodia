"""Panel for facade generator."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_facade(bpy.types.Panel):
    bl_label = "Facade"
    bl_idname = "MELODIA_PT_facade"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        storage = getattr(scene, "melodia_facade", None)
        if storage:
            from ..generators.facade import FacadeGenerator
            draw_generator_params(layout, FacadeGenerator, storage)

        layout.separator()
        op = layout.operator("melodia.generate", text="Generate Facade", icon='PLAY')
        op.generator_id = "facade"


def register():
    bpy.utils.register_class(MELODIA_PT_facade)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_facade)
