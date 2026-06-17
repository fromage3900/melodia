"""Vaults.Py panel."""

import bpy
from ..generators.base_generator import get_generator
from ..ui.ui_utils import draw_generator_params, register_generator_properties, collect_params_from_storage


class MELNOUVEAU_Props_panel_vaults(bpy.types.PropertyGroup):
    pass


def register_props():
    props_cls = MELNOUVEAU_Props_panel_vaults
    for gen_id, gen_name in [('gaudi_vault', 'Gaudi Vault')]:
        gen_cls = get_generator(gen_id)
        if gen_cls:
            props = register_generator_properties(gen_cls, gen_id)
            for name, prop in props.items():
                setattr(props_cls, name, prop)


MELNOUVEAU_Props_panel_vaults._generators = [('gaudi_vault', 'Gaudi Vault')]


class MELNOUVEAU_PT_panel_vaults(bpy.types.Panel):
    bl_label = "Vaults.Py"
    bl_idname = "MELNOUVEAU_PT_panel_vaults"
    bl_parent_id = "MELNOUVEAU_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_options = {DEFAULT_CLOSED}

    def draw(self, context):
        layout = self.layout
        storage = context.scene
        for gen_id, gen_name in MELNOUVEAU_Props_panel_vaults._generators:
            gen_cls = get_generator(gen_id)
            if not gen_cls:
                continue
            box = layout.box()
            box.label(text=gen_cls.generator_name)
            draw_generator_params(box, gen_cls, storage, gen_id)
            op = box.operator("melodia_nouveau.generate", text=f"Generate {gen_cls.generator_name}")
            op.generator_id = gen_id


_classes = [MELNOUVEAU_Props_panel_vaults, MELNOUVEAU_PT_panel_vaults]


def register():
    register_props()
    for cls in _classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
