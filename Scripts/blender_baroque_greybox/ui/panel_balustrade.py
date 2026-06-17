"""Panel for balustrade generator."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_balustrade(bpy.types.Panel):
    bl_label = "Balustrade"
    bl_idname = "MELODIA_PT_balustrade"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        # Generator params
        storage = getattr(scene, "melodia_balustrade", None)
        if storage:
            from ..generators.balustrade import BalustradeGenerator
            draw_generator_params(layout, BalustradeGenerator, storage)

        # Generate button
        layout.separator()
        op = layout.operator("melodia.generate", text="Generate Balustrade", icon='PLAY')
        op.generator_id = "balustrade"


def register():
    bpy.utils.register_class(MELODIA_PT_balustrade)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_balustrade)
