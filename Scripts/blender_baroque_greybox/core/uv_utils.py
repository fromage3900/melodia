"""
UV projection utilities for architectural meshes.
"""

import bpy
import bmesh
from mathutils import Vector


def uv_project_box(obj):
    """Box projection UV unwrap (best for architectural boxes)."""
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.cube_project(
        cube_size=1.0,
        correct_aspect=True,
        scale_to_bounds=True,
    )
    obj.select_set(False)


def uv_project_cylinder(obj):
    """Cylinder projection UV unwrap (best for columns)."""
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.cylinder_project(
        direction='VIEW_ON_EQUATOR',
        align='POLAR_ZX',
        radius=1.0,
        correct_aspect=True,
        scale_to_bounds=True,
    )
    obj.select_set(False)


def uv_smart_unwrap(obj, angle_limit=66.0):
    """Smart UV project (general purpose)."""
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(
        angle_limit=angle_limit,
        island_margin=0.02,
        area_weight=0.0,
        correct_aspect=True,
        scale_to_bounds=True,
    )
    obj.select_set(False)


def uv_align_to_world(obj):
    """Align UVs to world axes for consistent texturing."""
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.reset()
    obj.select_set(False)


def auto_uv(obj, generator_type='box'):
    """
    Automatically choose UV projection method based on generator type.
    """
    mapping = {
        'column': uv_project_cylinder,
        'baluster': uv_project_cylinder,
        'vault': uv_project_box,
        'facade': uv_project_box,
        'wall': uv_project_box,
        'floor': uv_project_box,
    }
    func = mapping.get(generator_type, uv_smart_unwrap)
    func(obj)
