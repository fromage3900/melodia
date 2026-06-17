"""
UE5-optimized FBX export with correct scale, pivot, and naming.
"""

import os
import bpy
import bmesh
from mathutils import Vector
from .naming import enforce_sm_prefix


def set_pivot_to_base_center(obj):
    """
    Move the object's origin to the base center (bottom of bbox, centered XY).
    This matches UE5's convention for static mesh placement.
    """
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.object.mode_set(mode='OBJECT')

    # Calculate bounding box center at base
    bbox_corners = [obj.matrix_world @ Vector(corner)
                    for corner in obj.bound_box]
    min_z = min(v.z for v in bbox_corners)
    center_x = sum(v.x for v in bbox_corners) / 8.0
    center_y = sum(v.y for v in bbox_corners) / 8.0

    # Set cursor to base center
    bpy.context.scene.cursor.location = Vector((center_x, center_y, min_z))

    # Set origin to cursor
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='MEDIAN')

    # Reset cursor to world origin
    bpy.context.scene.cursor.location = Vector((0, 0, 0))


def prepare_object_for_export(obj):
    """Prepare a single object for UE5 FBX export."""
    # Apply all transforms
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Enforce SM_ naming
    obj.name = enforce_sm_prefix(obj.name)
    obj.data.name = obj.name

    # Set pivot to base center
    set_pivot_to_base_center(obj)

    # Recalculate normals outside
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    obj.select_set(False)


def export_to_ue5(objects, directory, collection_name="MelodiaGreybox",
                  global_scale=0.01, batch=True):
    """
    Export objects as FBX for UE5 import.

    Args:
        objects: list of bpy.types.Object to export
        directory: output directory path
        collection_name: name for the FBX collection
        global_scale: scale factor (0.01 for Blender m → UE5 cm)
        batch: if True, export each object as separate FBX
    """
    if not os.path.exists(directory):
        os.makedirs(directory)

    if batch:
        exported = []
        for obj in objects:
            prepare_object_for_export(obj)
            filename = f"{obj.name}.fbx"
            filepath = os.path.join(directory, filename)

            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj

            bpy.ops.export_scene.fbx(
                filepath=filepath,
                use_selection=True,
                global_scale=global_scale,
                apply_unit_scale=True,
                axis_forward='-Z',
                axis_up='Y',
                use_tspace=True,
                mesh_smooth_type='FACE',
                add_leaf_bones=False,
                use_custom_props=True,
                object_types={'MESH'},
            )
            exported.append(filepath)
            obj.select_set(False)
        return exported
    else:
        # Single file export
        for obj in objects:
            prepare_object_for_export(obj)

        filepath = os.path.join(directory, f"{collection_name}.fbx")
        bpy.ops.object.select_all(action='DESELECT')
        for obj in objects:
            obj.select_set(True)

        bpy.ops.export_scene.fbx(
            filepath=filepath,
            use_selection=True,
            global_scale=global_scale,
            apply_unit_scale=True,
            axis_forward='-Z',
            axis_up='Y',
            use_tspace=True,
            mesh_smooth_type='FACE',
            add_leaf_bones=False,
            use_custom_props=True,
            object_types={'MESH'},
        )
        return [filepath]


def export_selected_to_ue5(directory=None, global_scale=0.01):
    """Export currently selected objects to UE5."""
    if directory is None:
        prefs = bpy.context.preferences.addons[__package__.split('.')[0]]
        if hasattr(prefs, 'preferences'):
            directory = prefs.preferences.export_directory
        if not directory:
            directory = os.path.join(bpy.path.abspath("//"), "FBX_Export")

    selected = [obj for obj in bpy.context.selected_objects
                if obj.type == 'MESH']
    if not selected:
        return []

    return export_to_ue5(selected, directory, global_scale=global_scale)
