# Project 0 — FBX to OBJ converter using Blender's Python API
#
# This script is intended to be run from Blender's embedded Python interpreter:
#   blender --background --python convert_fbx_to_obj.py -- <fbx_path> <obj_path>
#
# It imports an FBX scene and exports it as a clean, triangulated OBJ
# with normals and UVs, ready for the X-Racing renderer.
import bpy
import sys
import os
import time
import mathutils

# Resolve input/output paths from CLI args or fall back to defaults
# relative to the current Blender file directory.
fbx_path = sys.argv[-2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(bpy.data.filepath), "car.fbx")
obj_path = sys.argv[-1] if len(sys.argv) > 2 else os.path.join(os.path.dirname(bpy.data.filepath), "car.obj")

print(f"Converting {fbx_path} -> {obj_path}")

start = time.time()
bpy.ops.import_scene.fbx(
    filepath=fbx_path,
    global_scale=1.0,
    axis_forward='-Z',
    axis_up='Y'
)
import_time = time.time() - start
print(f"Import took {import_time:.2f} s")

# Deselect all, select only non-cube meshes (the car)
bpy.ops.object.select_all(action='DESELECT')
car_objects = []
for obj in bpy.data.objects:
    if obj.type == 'MESH' and obj.name != 'Cube':
        obj.select_set(True)
        car_objects.append(obj)
        if not bpy.context.view_layer.objects.active:
            bpy.context.view_layer.objects.active = obj

# Compute bounding box to scale mesh to ~5 meters (matching procedural car)
if car_objects:
    min_coord = [float('inf')] * 3
    max_coord = [float('-inf')] * 3
    for obj in car_objects:
        for corner in obj.bound_box:
            world_corner = obj.matrix_world @ mathutils.Vector(corner)
            for i in range(3):
                min_coord[i] = min(min_coord[i], world_corner[i])
                max_coord[i] = max(max_coord[i], world_corner[i])

    size = [max_coord[i] - min_coord[i] for i in range(3)]
    max_dim = max(size)
    if max_dim > 1e-6:
        target_size = 5.0
        scale_factor = target_size / max_dim
        print(f"Mesh size: {size}, max dim: {max_dim:.6f}, scale factor: {scale_factor:.6f}")
        for obj in car_objects:
            obj.scale *= scale_factor
        bpy.ops.object.transform_apply(scale=True)

# Decimate to reduce polygon count for real-time rendering
for obj in car_objects:
    if obj.type == 'MESH':
        vert_count = len(obj.data.vertices)
        if vert_count > 20000:
            ratio = 20000.0 / vert_count
            print(f"Decimating {obj.name}: {vert_count} -> ~20000 verts (ratio: {ratio:.3f})")
            mod = obj.modifiers.new(name="Decimate", type='DECIMATE')
            mod.decimate_type = 'COLLAPSE'
            mod.ratio = ratio
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.modifier_apply(modifier=mod.name)

start = time.time()
bpy.ops.wm.obj_export(
    filepath=obj_path,
    export_selected_objects=True,
    export_materials=False,
    export_normals=True,
    export_uv=True,
    export_triangulated_mesh=True,
    global_scale=1.0
)
export_time = time.time() - start
print(f"Export took {export_time:.2f} s")

print(f"Converted {fbx_path} -> {obj_path} in {import_time + export_time:.2f} s")
