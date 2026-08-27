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

start = time.time()
bpy.ops.wm.obj_export(
    filepath=obj_path,
    export_selected_objects=False,
    export_materials=False,
    export_normals=True,
    export_uv=True,
    export_triangulated_mesh=True
)
export_time = time.time() - start
print(f"Export took {export_time:.2f} s")

print(f"Converted {fbx_path} -> {obj_path} in {import_time + export_time:.2f} s")
