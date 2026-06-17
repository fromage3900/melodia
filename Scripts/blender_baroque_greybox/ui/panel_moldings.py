"""Panel for molding generators."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_moldings(bpy.types.Panel):
    bl_label = "Moldings"
    bl_idname = "MELODIA_PT_moldings"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        molding_gens = [
            ("molding_architrave", "Architrave"),
            ("molding_frieze", "Frieze"),
            ("molding_cornice", "Cornice"),
            ("molding_base", "Base Molding"),
        ]

        for gen_id, label in molding_gens:
            box = layout.box()
            row = box.row()
            row.label(text=label, icon='MOD_ARRAY')

            storage = getattr(scene, f"melodia_{gen_id}", None)
            if storage:
                from ..generators.base_generator import get_generator
                gen_cls = get_generator(gen_id)
                if gen_cls:
                    draw_generator_params(box, gen_cls, storage)

            op = box.operator("melodia.generate", text=f"Generate {label}", icon='PLAY')
            op.generator_id = gen_id


def register():
    bpy.utils.register_class(MELODIA_PT_moldings)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_moldings)
