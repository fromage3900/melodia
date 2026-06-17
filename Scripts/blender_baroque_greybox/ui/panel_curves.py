"""Panel for curve architecture generator."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_curves(bpy.types.Panel):
    bl_label = "Curve Architecture"
    bl_idname = "MELODIA_PT_curves"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        storage = getattr(scene, "melodia_curve_architecture", None)
        if storage:
            from ..generators.curve_architecture import CurveArchitectureGenerator
            draw_generator_params(layout, CurveArchitectureGenerator, storage)

        layout.separator()
        op = layout.operator("melodia.generate", text="Generate Curves", icon='PLAY')
        op.generator_id = "curve_architecture"


def register():
    bpy.utils.register_class(MELODIA_PT_curves)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_curves)
