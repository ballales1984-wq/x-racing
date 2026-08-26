import struct
import os

def read_uint32(f):
    return struct.unpack('<I', f.read(4))[0]

def read_uint64(f):
    return struct.unpack('<Q', f.read(8))[0]

def read_string(f, length):
    return f.read(length).decode('utf-8', errors='ignore')

def read_bytes(f, length):
    return f.read(length)

class FBXParser:
    def __init__(self, filename):
        self.filename = filename
        self.vertices = []
        self.faces = []
        self.normals = []
        self.uvs = []
        
    def parse(self):
        with open(self.filename, 'rb') as f:
            header = read_bytes(f, 23)
            if header != b'Kaydara FBX Binary  \x00':
                raise ValueError("Not a binary FBX file")
            
            version = read_uint32(f)
            print(f"FBX version: {version}")
            
            self._parse_nodes(f)
    
    def _parse_nodes(self, f):
        while True:
            try:
                pos = f.tell()
                end_offset = read_uint64(f)
                if end_offset == 0:
                    break
                    
                num_props = read_uint32(f)
                prop_list_len = read_uint32(f)
                name_len = read_uint8(f)
                name = read_string(f, name_len) if name_len > 0 else ""
                
                # Skip properties for now
                prop_data = read_bytes(f, prop_list_len)
                
                # Parse nested nodes
                if end_offset > f.tell():
                    self._parse_nodes(f)
                
                f.seek(end_offset)
            except Exception as e:
                break
    
    def export_obj(self, output_path):
        with open(output_path, 'w') as f:
            f.write("# Converted from FBX\n")
            for v in self.vertices:
                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            for face in self.faces:
                if len(face) == 3:
                    f.write(f"f {face[0]+1} {face[1]+1} {face[2]+1}\n")
        print(f"Exported {len(self.vertices)} vertices, {len(self.faces)} faces")

# Simple conversion using pymeshlab or just parsing
# For now, let's try a different approach - use the Unity FBX importer
# Or just copy the procedural mesh as fallback

import shutil
src = r"D:\x-racing\assets\models\vehicle.obj"
dst = r"D:\x-racing\assets\models\car.obj"
if os.path.exists(src):
    shutil.copy2(src, dst)
    print(f"Copied procedural mesh to car.obj as fallback")
else:
    print("No procedural mesh found")
