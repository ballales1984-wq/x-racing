# Project 0 — FBX to OBJ converter using pyassimp
#
# This script uses the Assimp library (via pyassimp) to load an FBX file
# and write it as a Wavefront OBJ with triangulated faces, normals and UVs.
#
# Usage:
#   python convert_fbx_to_obj_assimp.py <input.fbx> [output.obj]
import assimp
import sys
import os

# Resolve input/output paths from CLI args or use sensible defaults.
fbx_path = sys.argv[1] if len(sys.argv) > 1 else r"D:\x-racing\assets\models\car.fbx"
obj_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(fbx_path)[0] + ".obj"

print(f"Loading FBX: {fbx_path}")
scene = assimp.load(fbx_path)

if not scene.meshes:
    print("No meshes found in FBX file.")
    sys.exit(1)

with open(obj_path, 'w') as f:
    f.write("# Converted from FBX using pyassimp\n")
    f.write(f"# Meshes: {len(scene.meshes)}\n")

    # OBJ uses 1-based indices; Assimp uses 0-based.
    vertex_offset = 0
    for mesh in scene.meshes:
        f.write(f"o {mesh.name}\n")

        # Write vertex positions.
        for vertex in mesh.vertices:
            f.write(f"v {vertex.x:.6f} {vertex.y:.6f} {vertex.z:.6f}\n")

        # Write vertex normals if available.
        if mesh.normals:
            for normal in mesh.normals:
                f.write(f"vn {normal.x:.6f} {normal.y:.6f} {normal.z:.6f}\n")

        # Write texture coordinates (first UV channel) if available.
        if mesh.texturecoords and mesh.texturecoords[0]:
            for uv in mesh.texturecoords[0]:
                f.write(f"vt {uv.x:.6f} {uv.y:.6f}\n")

        # Write triangular faces with v/vt/vn indices.
        for face in mesh.faces:
            if len(face) == 3:
                i0 = face[0] + 1 + vertex_offset
                i1 = face[1] + 1 + vertex_offset
                i2 = face[2] + 1 + vertex_offset
                f.write(f"f {i0}/{i0}/{i0} {i1}/{i1}/{i1} {i2}/{i2}/{i2}\n")

        vertex_offset += len(mesh.vertices)

assimp.release(scene)
print(f"Converted {fbx_path} -> {obj_path}")
