"""Panel for preset management."""

import bpy
import os
import json


class MELODIA_OT_save_preset(bpy.types.Operator):
    """Save current parameters as a preset."""
    bl_idname = "melodia.save_preset"
    bl_label = "Save Preset"
    bl_description = "Save current generator parameters as a JSON preset"

    preset_name: bpy.props.StringProperty(name="Preset Name", default="my_preset")

    def execute(self, context):
        from ..presets.preset_manager import save_preset
        props = context.scene.melodia_props

        # Collect all current parameters
        data = self._collect_all_params(context.scene)

        # Save to file
        preset_dir = os.path.join(os.path.dirname(__file__), '..', 'presets', 'defaults')
        os.makedirs(preset_dir, exist_ok=True)
        filepath = os.path.join(preset_dir, f"{self.preset_name}.json")

        try:
            save_preset(filepath, data)
            self.report({'INFO'}, f"Preset saved: {self.preset_name}")
        except Exception as e:
            self.report({'ERROR'}, f"Save failed: {e}")
        return {'FINISHED'}

    def _collect_all_params(self, scene):
        data = {"mode": scene.melodia_props.geometry_mode, "generators": {}}
        # Collect from each generator's property group
        for attr_name in dir(scene):
            if attr_name.startswith("melodia_") and attr_name != "melodia_props":
                storage = getattr(scene, attr_name, None)
                if storage and hasattr(storage, '__annotations__'):
                    gen_data = {}
                    for prop_name in storage.__annotations__:
                        gen_data[prop_name] = getattr(storage, prop_name)
                    gen_id = attr_name.replace("melodia_", "")
                    data["generators"][gen_id] = gen_data
        return data

    def invoke(self, context, event):
        return context.window_manager.invoke_props_dialog(self)

    def draw(self, context):
        self.layout.prop(self, "preset_name")


class MELODIA_OT_load_preset(bpy.types.Operator):
    """Load a preset and apply parameters."""
    bl_idname = "melodia.load_preset"
    bl_label = "Load Preset"
    bl_description = "Load a JSON preset and apply parameters"

    filepath: bpy.props.StringProperty(subtype='FILE_PATH')
    filter_glob: bpy.props.StringProperty(default="*.json", options={'HIDDEN'})

    def execute(self, context):
        from ..presets.preset_manager import load_preset, apply_preset

        try:
            data = load_preset(self.filepath)
            apply_preset(context.scene, data)
            self.report({'INFO'}, f"Preset loaded: {os.path.basename(self.filepath)}")
        except Exception as e:
            self.report({'ERROR'}, f"Load failed: {e}")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class MELODIA_PT_presets(bpy.types.Panel):
    bl_label = "Presets"
    bl_idname = "MELODIA_PT_presets"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"
    bl_parent_id = "MELODIA_PT_main"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout

        # Built-in presets info
        box = layout.box()
        box.label(text="Built-in Presets:", icon='PRESET')
        presets_dir = os.path.join(os.path.dirname(__file__), '..', 'presets', 'defaults')
        if os.path.isdir(presets_dir):
            for f in sorted(os.listdir(presets_dir)):
                if f.endswith('.json'):
                    row = box.row()
                    row.label(text=f.replace('.json', ''))
                    op = row.operator("melodia.load_preset", text="Load", icon='IMPORT')
                    op.filepath = os.path.join(presets_dir, f)
        else:
            box.label(text="No presets found")

        # Save/Load
        layout.separator()
        layout.operator("melodia.save_preset", text="Save Current as Preset", icon='FILE_TICK')
        layout.operator("melodia.load_preset", text="Load Preset File...", icon='FILE_FOLDER')


_classes = [
    MELODIA_OT_save_preset,
    MELODIA_OT_load_preset,
    MELODIA_PT_presets,
]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
