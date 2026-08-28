import struct
import os

def read_glb(glb_path):
    with open(glb_path, 'rb') as f:
        header = f.read(12)
        magic, version, length = struct.unpack('<III', header)
        
        chunk_header = f.read(8)
        chunk_length, chunk_type = struct.unpack('<II', chunk_header)
        json_data = f.read(chunk_length)
        
        chunk_header2 = f.read(8)
        chunk_length2, chunk_type2 = struct.unpack('<II', chunk_header2)
        bin_data = f.read(chunk_length2)
        
        return json_data, bin_data

def parse_accessor(bin_data, accessor, buffer_views):
    bv = buffer_views[accessor.bufferView]
    offset = (bv.byteOffset or 0) + (accessor.byteOffset or 0)
    data = bin_data[offset:offset + bv.byteLength]
    
    count = accessor.count
    component_type = accessor.componentType
    accessor_type = accessor.type
    
    if accessor_type == 'SCALAR':
        if component_type == 5125:  # UNSIGNED_INT
            return [struct.unpack_from('<I', data, i*4)[0] for i in range(count)]
        elif component_type == 5123:  # UNSIGNED_SHORT
            return [struct.unpack_from('<H', data, i*2)[0] for i in range(count)]
    elif accessor_type == 'VEC3':
        if component_type == 5126:  # FLOAT
            result = []
            for i in range(count):
                x, y, z = struct.unpack_from('<fff', data, i*12)
                result.append((x, y, z))
            return result
    elif accessor_type == 'VEC2':
        if component_type == 5126:  # FLOAT
            result = []
            for i in range(count):
                u, v = struct.unpack_from('<ff', data, i*8)
                result.append((u, v))
            return result
    
    return []

def export_glb_to_obj(glb_path, out_dir):
    from pygltflib import GLTF2
    
    gltf = GLTF2().load(glb_path)
    json_data, bin_data = read_glb(glb_path)
    
    os.makedirs(out_dir, exist_ok=True)
    base_name = os.path.splitext(os.path.basename(glb_path))[0]
    obj_path = os.path.join(out_dir, base_name + ".obj")
    mtl_path = os.path.join(out_dir, base_name + ".mtl")
    
    # Parse JSON to get material colors
    import json
    gltf_json = json.loads(json_data)
    
    material_colors = {}
    if 'materials' in gltf_json:
        for i, mat in enumerate(gltf_json['materials']):
            color = (0.7, 0.7, 0.7)
            if 'pbrMetallicRoughness' in mat and 'baseColorFactor' in mat['pbrMetallicRoughness']:
                bcf = mat['pbrMetallicRoughness']['baseColorFactor']
                if len(bcf) >= 3:
                    color = (bcf[0], bcf[1], bcf[2])
            material_colors[i] = color
    
    # Export OBJ+MTL
    with open(mtl_path, 'w') as f:
        f.write(f"# MTL generated from {os.path.basename(glb_path)}\n")
        for mat_idx, color in material_colors.items():
            mat_name = f"Material_{mat_idx}"
            f.write(f"\nnewmtl {mat_name}\n")
            f.write(f"Kd {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
            f.write(f"Ka {color[0]*0.2:.6f} {color[1]*0.2:.6f} {color[2]*0.2:.6f}\n")
            f.write(f"Ks 0.000000 0.000000 0.000000\n")
            f.write(f"Ns 0.0\n")
            f.write(f"d 1.0\n")
    
    with open(obj_path, 'w') as f:
        f.write(f"# Converted from {os.path.basename(glb_path)}\n")
        f.write(f"mtllib {os.path.basename(mtl_path)}\n")
        
        vertex_offset = 0
        
        for mesh_idx, mesh in enumerate(gltf.meshes):
            for prim_idx, prim in enumerate(mesh.primitives):
                positions = parse_accessor(bin_data, gltf.accessors[prim.attributes.POSITION], gltf.bufferViews)
                normals = parse_accessor(bin_data, gltf.accessors[prim.attributes.NORMAL], gltf.bufferViews) if prim.attributes.NORMAL is not None else None
                indices = parse_accessor(bin_data, gltf.accessors[prim.indices], gltf.bufferViews)
                
                mat_idx = prim.material if prim.material is not None else 0
                mat_name = f"Material_{mat_idx}"
                color = material_colors.get(mat_idx, (0.7, 0.7, 0.7))
                
                f.write(f"usemtl {mat_name}\n")
                
                for v in positions:
                    f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f} {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
                
                if normals:
                    for n in normals:
                        f.write(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
                
                for i in range(len(positions)):
                    f.write(f"vt 0.000000 0.000000\n")
                
                for i in range(0, len(indices), 3):
                    vi0 = indices[i] + 1 + vertex_offset
                    vi1 = indices[i+1] + 1 + vertex_offset
                    vi2 = indices[i+2] + 1 + vertex_offset
                    f.write(f"f {vi0}/{vi0}/{vi0} {vi1}/{vi1}/{vi1} {vi2}/{vi2}/{vi2}\n")
                
                vertex_offset += len(positions)
    
    print(f"Exported: {obj_path}")
    print(f"Exported: {mtl_path}")

if __name__ == "__main__":
    glb_path = r"D:\x-racing\data\models\vehicle.glb"
    out_dir = r"D:\x-racing\assets\models"
    export_glb_to_obj(glb_path, out_dir)
