"""Panel for batch generation — multi-generator orchestration."""

import bpy
import json


class MELODIA_OT_batch_generate(bpy.types.Operator):
    """Run batch generation from a layout description."""
    bl_idname = "melodia.batch_generate"
    bl_label = "Batch Generate"
    bl_description = "Generate multiple elements from a layout description"

    def execute(self, context):
        props = context.scene.melodia_props
        batch_props = context.scene.melodia_batch

        layout_json = batch_props.layout_json
        if not layout_json.strip():
            self.report({'WARNING'}, "No layout description provided")
            return {'CANCELLED'}

        try:
            from ..batch.batch_generator import run_batch
            from ..core.geometry_modes import GeometryMode

            mode = GeometryMode[props.geometry_mode]
            layout_data = json.loads(layout_json)

            results = run_batch(layout_data, mode=mode, seed=props.seed,
                                collection=context.collection)
            self.report({'INFO'}, f"Batch generated {len(results)} elements")
            return {'FINISHED'}
        except json.JSONDecodeError as e:
            self.report({'ERROR'}, f"Invalid JSON: {e}")
            return {'CANCELLED'}
        except Exception as e:
            self.report({'ERROR'}, f"Batch failed: {e}")
            return {'CANCELLED'}


class MELODIA_OT_load_layout_file(bpy.types.Operator):
    """Load a layout description from a JSON file."""
    bl_idname = "melodia.load_layout"
    bl_label = "Load Layout File"
    bl_description = "Load layout description from a JSON file"

    filepath: bpy.props.StringProperty(subtype='FILE_PATH')
    filter_glob: bpy.props.StringProperty(default="*.json", options={'HIDDEN'})

    def execute(self, context):
        try:
            with open(self.filepath, 'r') as f:
                data = f.read()
            context.scene.melodia_batch.layout_json = data
            self.report({'INFO'}, f"Loaded: {self.filepath}")
        except Exception as e:
            self.report({'ERROR'}, f"Load failed: {e}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class MelodiaBatchProps(bpy.types.PropertyGroup):
    """Batch generation properties."""
    layout_json: bpy.props.StringProperty(
        name="Layout JSON",
        default='{\n  "elements": []\n}',
        description="JSON layout description for batch generation",
    )


class MELODIA_PT_batch(bpy.types.Panel):
    bl_label = "Batch Generation"
    bl_idname = "MELODIA_PT_batch"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        batch_props = context.scene.melodia_batch

        # Layout JSON input
        box = layout.box()
        box.label(text="Layout Description (JSON):", icon='TEXT')
        box.prop(batch_props, "layout_json", text="")

        # Example layout
        box2 = layout.box()
        box2.label(text="Example:", icon='INFO')
        example = (
            '{"elements": [\n'
            '  {"type": "column_doric", "params": {"height": 600}},\n'
            '  {"type": "balustrade", "params": {"length": 400}},\n'
            '  {"type": "vault_barrel", "params": {"width": 400}}\n'
            ']}'
        )
        for line in example.split('\n'):
            box2.label(text=line)

        # Buttons
        layout.separator()
        row = layout.row(align=True)
        row.operator("melodia.batch_generate", text="Batch Generate", icon='PLAY')
        row.operator("melodia.load_layout", text="Load File", icon='FILE_FOLDER')


_classes = [
    MelodiaBatchProps,
    MELODIA_OT_batch_generate,
    MELODIA_OT_load_layout_file,
    MELODIA_PT_batch,
]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.melodia_batch = bpy.props.PointerProperty(type=MelodiaBatchProps)


def unregister():
    del bpy.types.Scene.melodia_batch
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
