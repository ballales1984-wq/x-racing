import assimp
import os

fbx_path = r"D:\x-racing\assets\models\car.fbx"
obj_path = r"D:\x-racing\assets\models\car.obj"

scene = assimp.load(fbx_path)

with open(obj_path, 'w') as f:
    f.write("# Converted from FBX using pyassimp\n")
    
    vertex_offset = 0
    for mesh in scene.meshes:
        f.write(f"o {mesh.name}\n")
        
        for vertex in mesh.vertices:
            f.write(f"v {vertex.x:.6f} {vertex.y:.6f} {vertex.z:.6f}\n")
        
        for normal in mesh.normals:
            f.write(f"vn {normal.x:.6f} {normal.y:.6f} {normal.z:.6f}\n")
        
        for uv in mesh.texturecoords[0] if mesh.texturecoords else []:
            f.write(f"vt {uv.x:.6f} {uv.y:.6f}\n")
        
        for face in mesh.faces:
            if len(face) == 3:
                i0 = face[0] + 1 + vertex_offset
                i1 = face[1] + 1 + vertex_offset
                i2 = face[2] + 1 + vertex_offset
                f.write(f"f {i0}/{i0}/{i0} {i1}/{i1}/{i1} {i2}/{i2}/{i2}\n")
        
        vertex_offset += len(mesh.vertices)

assimp.release(scene)
print(f"Converted {fbx_path} -> {obj_path}")
