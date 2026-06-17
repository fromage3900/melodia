"""Panel for Escher/surreal generators."""

import bpy
from .ui_utils import draw_generator_params


class MELODIA_PT_escher(bpy.types.Panel):
    bl_label = "Escher / Surreal"
    bl_idname = "MELODIA_PT_escher"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        # Warning about IMPOSSIBLE mode
        props = scene.melodia_props
        if props.geometry_mode == 'VALID':
            box = layout.box()
            box.label(text="Tip: Switch to IMPOSSIBLE mode for", icon='INFO')
            box.label(text="full Escher self-intersection effects")

        escher_gens = [
            ("penrose_stairs", "Penrose Stairs", "4-flight impossible loop"),
            ("mobius_walkway", "Mobius Walkway", "Half-twist strip"),
            ("impossible_bridge", "Impossible Bridge", "Self-connecting paradox"),
            ("gravity_platform", "Gravity Platform", "Tilted room"),
            ("recursive_arches", "Recursive Arches", "Golden nested tunnel"),
            ("klein_volume", "Klein Volume", "Figure-8 non-orientable"),
        ]

        for gen_id, label, desc in escher_gens:
            box = layout.box()
            row = box.row()
            row.label(text=label, icon='MESH_TORUS')
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
    bpy.utils.register_class(MELODIA_PT_escher)


def unregister():
    bpy.utils.unregister_class(MELODIA_PT_escher)
