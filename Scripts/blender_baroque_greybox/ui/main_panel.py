"""
Main panel — N-panel sidebar for Melodia Baroque Greybox.

Provides mode toggle (VALID/IMPOSSIBLE), seed control,
Generate All, and Export to UE5 buttons.
"""

import bpy
from ..core.geometry_modes import GeometryMode
from ..core.fbx_export import export_selected_to_ue5, set_pivot_to_base_center
from ..generators.base_generator import GeneratorContext, list_generators_by_category


# ---------------------------------------------------------------------------
# Scene properties
# ---------------------------------------------------------------------------

class MelodiaSceneProps(bpy.types.PropertyGroup):
    """Global scene properties for Melodia addon."""
    geometry_mode: bpy.props.EnumProperty(
        name="Geometry Mode",
        items=[
            ("VALID", "Valid", "Watertight, exportable, manifold mesh"),
            ("IMPOSSIBLE", "Impossible", "Non-manifold, Escher-style geometry"),
        ],
        default="VALID",
        description="Geometry validity mode",
    )
    seed: bpy.props.IntProperty(
        name="Seed",
        default=42,
        min=0,
        max=999999,
        description="Random seed for deterministic generation",
    )
    export_path: bpy.props.StringProperty(
        name="Export Path",
        default="//exports/",
        subtype='DIR_PATH',
        description="FBX export directory for UE5",
    )
    randomize_seed: bpy.props.BoolProperty(
        name="Randomize Seed",
        default=True,
        description="Randomize seed on each generation",
    )


# ---------------------------------------------------------------------------
# Operators
# ---------------------------------------------------------------------------

class MELODIA_OT_generate(bpy.types.Operator):
    """Generate a specific generator by ID."""
    bl_idname = "melodia.generate"
    bl_label = "Generate"
    bl_description = "Generate the selected architectural element"
    bl_options = {'REGISTER', 'UNDO'}

    generator_id: bpy.props.StringProperty(name="Generator ID")

    def execute(self, context):
        from ..generators.base_generator import get_generator
        from .ui_utils import collect_params_from_storage

        scene = context.scene
        props = scene.melodia_props

        gen_cls = get_generator(self.generator_id)
        if gen_cls is None:
            self.report({'ERROR'}, f"Generator '{self.generator_id}' not found")
            return {'CANCELLED'}

        # Build context
        mode = GeometryMode[props.geometry_mode]
        seed = props.seed
        if props.randomize_seed:
            import random
            seed = random.randint(0, 999999)
            props.seed = seed

        ctx = GeneratorContext(
            scene=scene,
            collection=context.collection,
            mode=mode,
            seed=seed,
        )

        # Collect params from storage
        storage = getattr(scene, f"melodia_{self.generator_id}", None)
        if storage:
            params = collect_params_from_storage(storage, gen_cls)
        else:
            params = {}

        # Generate
        gen = gen_cls()
        try:
            obj = gen.generate(params, ctx)
            if obj:
                self.report({'INFO'}, f"Generated: {obj.name}")
                # Select the new object
                bpy.ops.object.select_all(action='DESELECT')
                obj.select_set(True)
                context.view_layer.objects.active = obj
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'}, f"Generation failed: {str(e)}")
            return {'CANCELLED'}


class MELODIA_OT_export_ue5(bpy.types.Operator):
    """Export selected objects to UE5-compatible FBX."""
    bl_idname = "melodia.export_ue5"
    bl_label = "Export to UE5"
    bl_description = "Export selected objects as FBX for UE5 import"

    def execute(self, context):
        props = context.scene.melodia_props
        selected = context.selected_objects

        if not selected:
            self.report({'WARNING'}, "No objects selected for export")
            return {'CANCELLED'}

        export_path = bpy.path.abspath(props.export_path)
        try:
            result = export_selected_to_ue5(directory=export_path)
            self.report({'INFO'}, f"Exported {len(result)} objects to {export_path}")
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'}, f"Export failed: {str(e)}")
            return {'CANCELLED'}


class MELODIA_OT_set_pivot(bpy.types.Operator):
    """Set pivot to base center for selected objects."""
    bl_idname = "melodia.set_pivot"
    bl_label = "Set Pivot to Base"
    bl_description = "Set object pivot to base center (for UE5)"

    def execute(self, context):
        for obj in context.selected_objects:
            if obj.type == 'MESH':
                set_pivot_to_base_center(obj)
        self.report({'INFO'}, f"Updated pivot for {len(context.selected_objects)} objects")
        return {'FINISHED'}


# ---------------------------------------------------------------------------
# Main Panel
# ---------------------------------------------------------------------------

class MELODIA_PT_main(bpy.types.Panel):
    """Melodia Baroque Greybox — Main Panel."""
    bl_label = "Melodia Baroque"
    bl_idname = "MELODIA_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Baroque"

    def draw(self, context):
        layout = self.layout
        props = context.scene.melodia_props

        # ---- MODE ----
        box = layout.box()
        box.label(text="Geometry Mode", icon='MODIFIER')
        row = box.row()
        row.prop(props, "geometry_mode", expand=True)

        # ---- SEED ----
        box = layout.box()
        box.label(text="Seed", icon='EVENT_S')
        row = box.row(align=True)
        row.prop(props, "seed", text="")
        row.prop(props, "randomize_seed", text="", icon='FILE_REFRESH')

        # ---- EXPORT ----
        box = layout.box()
        box.label(text="UE5 Export", icon='EXPORT')
        box.prop(props, "export_path", text="Path")
        row = box.row(align=True)
        row.operator("melodia.export_ue5", icon='EXPORT')
        row.operator("melodia.set_pivot", icon='PIVOT_MEDIAN')


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------

_classes = [
    MelodiaSceneProps,
    MELODIA_OT_generate,
    MELODIA_OT_export_ue5,
    MELODIA_OT_set_pivot,
    MELODIA_PT_main,
]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.melodia_props = bpy.props.PointerProperty(type=MelodiaSceneProps)


def unregister():
    del bpy.types.Scene.melodia_props
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
