import bpy
import sys
import os

fbx_path = sys.argv[-2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(bpy.data.filepath), "car.fbx")
obj_path = sys.argv[-1] if len(sys.argv) > 2 else os.path.join(os.path.dirname(bpy.data.filepath), "car.obj")

bpy.ops.import_scene.fbx(filepath=fbx_path)

bpy.ops.export_scene.obj(
    filepath=obj_path,
    axis_forward='-Z',
    axis_up='Y',
    use_materials=False,
    use_triangles=True
)

print(f"Converted {fbx_path} -> {obj_path}")
