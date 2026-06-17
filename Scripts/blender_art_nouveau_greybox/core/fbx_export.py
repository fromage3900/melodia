"""FBX export for UE5: scale, pivot, naming conventions."""

import os
import bpy
from mathutils import Vector


def set_pivot_to_base_center(obj: bpy.types.Object) -> bpy.types.Object:
    """Set object pivot to the base center (bottom-center of bounding box)."""
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.object.mode_set(mode='OBJECT')
    # Find bottom center
    min_z = min(v.co.z for v in obj.data.vertices)
    bottom_center = Vector((0, 0, min_z))
    obj.location -= bottom_center
    return obj


def prepare_object_for_export(obj: bpy.types.Object, name: str = None) -> bpy.types.Object:
    """Apply transforms and rename for UE5."""
    if name:
        obj.name = name
        obj.data.name = name
    set_pivot_to_base_center(obj)
    bpy.context.view_layer.update()
    return obj


def export_to_ue5(obj: bpy.types.Object, directory: str, name: str = None,
                  global_scale: float = 0.01) -> str:
    """Export a single object as FBX for UE5 import."""
    if name is None:
        name = obj.name
    if not name.startswith('SM_'):
        name = f'SM_{name}'
    prepare_object_for_export(obj, name)
    filepath = os.path.join(directory, f'{name}.fbx')
    os.makedirs(directory, exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        global_scale=global_scale,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',
        axis_forward='-Z',
        axis_up='Y',
    )
    return filepath


def export_selected_to_ue5(directory: str, global_scale: float = 0.01) -> list[str]:
    """Export all selected objects as FBX for UE5."""
    selected = bpy.context.selected_objects
    paths = []
    for obj in selected:
        paths.append(export_to_ue5(obj, directory, global_scale=global_scale))
    return paths
