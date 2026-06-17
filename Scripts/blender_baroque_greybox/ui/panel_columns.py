"""Panel for column order generators (5 orders)."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_columns(bpy.types.Panel):
    bl_label = "Columns"
    bl_idname = "MELODIA_PT_columns"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        # Column order buttons
        column_gens = [
            ("column_doric", "Doric", "Simple, sturdy"),
            ("column_ionic", "Ionic", "Scroll volutes"),
            ("column_corinthian", "Corinthian", "Acanthus leaves"),
            ("column_composite", "Composite", "Ionic + Corinthian"),
            ("column_solomonic", "Solomonic", "Baroque twisted"),
        ]

        for gen_id, label, desc in column_gens:
            box = layout.box()
            row = box.row()
            row.label(text=label, icon='COLUMN')
            row.label(text=desc)

            storage = getattr(scene, f"melodia_{gen_id}", None)
            if storage:
                from ..generators.base_generator import get_generator
                gen_cls = get_generator(gen_id)
                if gen_cls:
                    draw_generator_params(box, gen_cls, storage)

            op = box.operator("melodia.generate", text=f"Generate {label}", icon='PLAY')
            op.generator_id = gen_id


def register():
    bpy.utils.register_class(MELODIA_PT_columns)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_columns)
