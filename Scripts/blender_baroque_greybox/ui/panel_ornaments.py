"""Panel for ornament generators."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_ornaments(bpy.types.Panel):
    bl_label = "Ornaments"
    bl_idname = "MELODIA_PT_ornaments"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        ornament_gens = [
            ("ornament_volute", "Volute"),
            ("ornament_acanthus", "Acanthus Leaves"),
            ("ornament_rose_window", "Rose Window"),
            ("ornament_cartouche", "Cartouche"),
            ("ornament_finial", "Finial"),
            ("ornament_shell", "Shell Motif"),
        ]

        for gen_id, label in ornament_gens:
            box = layout.box()
            row = box.row()
            row.label(text=label, icon='OUTLINER_OB_MESH')

            storage = getattr(scene, f"melodia_{gen_id}", None)
            if storage:
                from ..generators.base_generator import get_generator
                gen_cls = get_generator(gen_id)
                if gen_cls:
                    draw_generator_params(box, gen_cls, storage)

            op = box.operator("melodia.generate", text=f"Generate {label}", icon='PLAY')
            op.generator_id = gen_id


def register():
    bpy.utils.register_class(MELODIA_PT_ornaments)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_ornaments)
