"""Main panel — N-panel sidebar for Melodia Art Nouveau Greybox."""

import bpy
from ..core.geometry_modes import GeometryMode
from ..core.fbx_export import export_selected_to_ue5, set_pivot_to_base_center
from ..generators.base_generator import GeneratorContext, list_generators_by_category
from ..core.seed_manager import create_rng
from ..core.naming import make_generator_name, enforce_sm_prefix


class MelodiaSceneProps(bpy.types.PropertyGroup):
    geometry_mode: bpy.props.EnumProperty(
        name="Geometry Mode",
        items=[("VALID", "Valid", "Watertight, exportable manifold mesh"), ("IMPOSSIBLE", "Impossible", "Non-manifold, overlapping geometry")],
        default="VALID",
    )
    seed: bpy.props.IntProperty(name="Seed", default=42, min=0, max=99999)
    export_path: bpy.props.StringProperty(name="Export Path", subtype='DIR_PATH')
    randomize_seed: bpy.props.BoolProperty(name="Randomize Seed", default=False)


class MELNOUVEAU_OT_generate(bpy.types.Operator):
    bl_idname = "melodia_nouveau.generate"
    bl_label = "Generate"
    bl_description = "Generate selected geometry"

    generator_id: bpy.props.StringProperty()

    def execute(self, context):
        from ..generators.base_generator import get_generator
        from .ui_utils import collect_params_from_storage
        gen_cls = get_generator(self.generator_id)
        if not gen_cls:
            self.report({'ERROR'}, f"Generator {self.generator_id} not found")
            return {'CANCELLED'}
        props = context.scene.melodia_props
        seed = props.seed
        if props.randomize_seed:
            import random
            seed = random.randint(0, 99999)
        rng = create_rng(seed)
        mode = GeometryMode.VALID if props.geometry_mode == 'VALID' else GeometryMode.IMPOSSIBLE
        ctx = GeneratorContext(mode=mode, seed=seed, rng=rng)
        params = collect_params_from_storage(context.scene, gen_cls, self.generator_id)
        generator = gen_cls()
        generator.preview(ctx, params)
        self.report({'INFO'}, f"Generated {gen_cls.generator_name}")
        return {'FINISHED'}


class MELNOUVEAU_OT_export_ue5(bpy.types.Operator):
    bl_idname = "melodia_nouveau.export_ue5"
    bl_label = "Export to UE5"
    bl_description = "Export selected objects as FBX for UE5"

    def execute(self, context):
        props = context.scene.melodia_props
        if not props.export_path:
            self.report({'ERROR'}, "Set export path first")
            return {'CANCELLED'}
        export_selected_to_ue5(directory=props.export_path)
        self.report({'INFO'}, "Exported to UE5")
        return {'FINISHED'}


class MELNOUVEAU_PT_main(bpy.types.Panel):
    bl_label = "Melodia Art Nouveau"
    bl_idname = "MELNOUVEAU_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Melodia Art Nouveau"

    def draw(self, context):
        layout = self.layout
        props = context.scene.melodia_props
        box = layout.box()
        box.label(text="Geometry Mode")
        box.prop(props, "geometry_mode", text="")
        box = layout.box()
        box.label(text="Random Seed")
        box.prop(props, "seed", text="")
        box.prop(props, "randomize_seed")
        box = layout.box()
        box.label(text="Export to UE5")
        box.prop(props, "export_path", text="")
        box.operator("melodia_nouveau.export_ue5", text="Export Selected")


_classes = [MelodiaSceneProps, MELNOUVEAU_OT_generate, MELNOUVEAU_OT_export_ue5, MELNOUVEAU_PT_main]


def register():
    for cls in _classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.melodia_props = bpy.props.PointerProperty(type=MelodiaSceneProps)


def unregister():
    del bpy.types.Scene.melodia_props
    for cls in reversed(_classes):
        bpy.utils.unregister_class(cls)
