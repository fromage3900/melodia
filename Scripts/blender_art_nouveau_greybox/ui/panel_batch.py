"""Batch generation panel."""

import bpy


class MelodiaBatchProps(bpy.types.PropertyGroup):
    layout_json: bpy.props.StringProperty(name="Layout JSON", default='{"type": "elements", "elements": []}')


class MELNOUVEAU_OT_batch_generate(bpy.types.Operator):
    bl_idname = "melodia_nouveau.batch_generate"
    bl_label = "Batch Generate"

    def execute(self, context):
        from ..batch.batch_generator import run_batch_from_json
        from ..core.geometry_modes import GeometryMode
        props = context.scene.melodia_props
        batch_props = context.scene.melodia_batch_props
        mode = GeometryMode.VALID if props.geometry_mode == 'VALID' else GeometryMode.IMPOSSIBLE
        run_batch_from_json(batch_props.layout_json, mode, props.seed)
        self.report({'INFO'}, "Batch generation complete")
        return {'FINISHED'}


class MELNOUVEAU_PT_batch(bpy.types.Panel):
    bl_label = "Batch Generation"
    bl_idname = "MELNOUVEAU_PT_batch"
    bl_parent_id = "MELNOUVEAU_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        batch_props = context.scene.melodia_batch_props
        layout.prop(batch_props, "layout_json")
        layout.operator("melodia_nouveau.batch_generate", text="Generate Batch")


_classes = [MelodiaBatchProps, MELNOUVEAU_OT_batch_generate, MELNOUVEAU_PT_batch]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.melodia_batch_props = bpy.props.PointerProperty(type=MelodiaBatchProps)


def unregister():
    del bpy.types.Scene.melodia_batch_props
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
