# Project 0 — FBX to OBJ converter using pyassimp
#
# This script uses the Assimp library (bundled in vendor/assimp) to load an FBX
# file and write it as a Wavefront OBJ with triangulated faces, normals and UVs.
#
# Usage:
#   python convert_fbx_to_obj_assimp.py <input.fbx> [output.obj]
import sys
import os
import time

# Add the vendored Assimp DLL directory to PATH so that `import assimp` can find
# the native library at runtime.
_ASSIMP_BIN = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "vendor", "assimp", "bin",
))
if os.path.isdir(_ASSIMP_BIN):
    os.environ["PATH"] = _ASSIMP_BIN + os.pathsep + os.environ.get("PATH", "")

import assimp
import numpy as np

# Resolve input/output paths from CLI args or use sensible defaults.
if len(sys.argv) > 1:
    fbx_path = sys.argv[1]
else:
    fbx_path = r"D:\x-racing\assets\models\car.fbx"
obj_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(fbx_path)[0] + ".obj"

print(f"Loading FBX: {fbx_path}")
t0 = time.time()
scene = assimp.load(fbx_path)
print(f"  Load took {time.time() - t0:.1f}s")

if not scene.meshes:
    print("No meshes found in FBX file.")
    sys.exit(1)

print(f"  Meshes: {len(scene.meshes)}")

CHUNK = 200_000  # write in chunks to limit memory usage

with open(obj_path, 'w') as f:
    f.write("# Converted from FBX using pyassimp\n")
    f.write(f"# Meshes: {len(scene.meshes)}\n")

    vertex_offset = 0
    normal_offset = 0
    uv_offset = 0

    for mi, mesh in enumerate(scene.meshes):
        name = mesh.name if mesh.name else f"mesh_{mi}"
        verts = np.asarray(mesh.vertices, dtype=np.float64)
        faces = mesh.faces
        print(f"  Mesh '{name}': {len(verts)} verts, {len(faces)} faces", flush=True)

        f.write(f"o {name}\n")

        has_normals = mesh.normals is not None and len(mesh.normals) > 0
        if has_normals:
            normals = np.asarray(mesh.normals, dtype=np.float64)

        texturecoords = mesh.texturecoords
        has_uvs = texturecoords is not None and len(texturecoords) > 0
        if has_uvs:
            uvs = np.asarray(texturecoords[0], dtype=np.float64)

        # Write vertex positions in chunks.
        t1 = time.time()
        for i in range(0, len(verts), CHUNK):
            chunk = verts[i:i + CHUNK]
            lines = [f"v {x:.6f} {y:.6f} {z:.6f}\n" for x, y, z in chunk]
            f.write("".join(lines))
        print(f"    Wrote {len(verts)} vertices in {time.time()-t1:.1f}s", flush=True)

        # Write vertex normals if available.
        if has_normals:
            t1 = time.time()
            for i in range(0, len(normals), CHUNK):
                chunk = normals[i:i + CHUNK]
                lines = [f"vn {x:.6f} {y:.6f} {z:.6f}\n" for x, y, z in chunk]
                f.write("".join(lines))
            print(f"    Wrote {len(normals)} normals in {time.time()-t1:.1f}s", flush=True)

        # Write texture coordinates if available.
        if has_uvs:
            t1 = time.time()
            for i in range(0, len(uvs), CHUNK):
                chunk = uvs[i:i + CHUNK]
                lines = [f"vt {uv[0]:.6f} {uv[1]:.6f}\n" for uv in chunk]
                f.write("".join(lines))
            print(f"    Wrote {len(uvs)} UVs in {time.time()-t1:.1f}s", flush=True)

        # Write triangular faces with proper v/vt/vn indices.
        t1 = time.time()
        for i in range(0, len(faces), CHUNK):
            chunk = faces[i:i + CHUNK]
            if has_normals and has_uvs:
                lines = [
                    f"f {a+1+vertex_offset}/{a+1+uv_offset}/{a+1+normal_offset} "
                    f"{b+1+vertex_offset}/{b+1+uv_offset}/{b+1+normal_offset} "
                    f"{c+1+vertex_offset}/{c+1+uv_offset}/{c+1+normal_offset}\n"
                    for a, b, c in chunk
                ]
            elif has_normals:
                lines = [
                    f"f {a+1+vertex_offset}//{a+1+normal_offset} "
                    f"{b+1+vertex_offset}//{b+1+normal_offset} "
                    f"{c+1+vertex_offset}//{c+1+normal_offset}\n"
                    for a, b, c in chunk
                ]
            elif has_uvs:
                lines = [
                    f"f {a+1+vertex_offset}/{a+1+uv_offset} "
                    f"{b+1+vertex_offset}/{b+1+uv_offset} "
                    f"{c+1+vertex_offset}/{c+1+uv_offset}\n"
                    for a, b, c in chunk
                ]
            else:
                lines = [
                    f"f {a+1+vertex_offset} {b+1+vertex_offset} {c+1+vertex_offset}\n"
                    for a, b, c in chunk
                ]
            f.write("".join(lines))
        print(f"    Wrote {len(faces)} faces in {time.time()-t1:.1f}s", flush=True)

        vertex_offset += len(verts)
        if has_normals:
            normal_offset += len(normals)
        if has_uvs:
            uv_offset += len(uvs)

assimp.release(scene)
print(f"Converted {fbx_path} -> {obj_path}")
