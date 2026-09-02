# Project 0 — FBX to OBJ converter using a lightweight standalone FBX binary parser.
#
# This is a fallback converter for environments where the Assimp-vendored
# convert_fbx_to_obj_assimp.py cannot be used (e.g. no native DLL).
# It parses the FBX binary format directly and writes a Wavefront OBJ.
#
# Usage:
#   python simple_fbx_convert.py <input.fbx> [output.obj]
import struct
import os
import sys
import zlib
import time
from io import BytesIO

def read_uint32(f):
    return struct.unpack('<I', f.read(4))[0]

def read_uint64(f):
    return struct.unpack('<Q', f.read(8))[0]

def read_uint8(f):
    return f.read(1)[0]

def read_string(f, length):
    return f.read(length).decode('utf-8', errors='ignore')

def read_int32(f):
    return struct.unpack('<i', f.read(4))[0]


class FBXParser:
    def __init__(self, filename):
        self.filename = filename
        self.vertices = []
        self.faces = []
        self.normals = []
        self.uvs = []

    def parse(self):
        with open(self.filename, 'rb') as f:
            header = f.read(23)
            if header[:20] != b'Kaydara FBX Binary  ':
                raise ValueError("Not a binary FBX file")
            version = read_uint32(f)
            print(f"FBX version: {version}")

            # Recursively parse nodes looking for Geometry/Mesh data
            root = self._read_node(f)
            if root:
                self._find_geometry(root)

    def _read_node(self, f):
        """Read a single FBX binary node. Returns dict or None for null terminator."""
        pos = f.tell()
        end_offset = read_uint64(f)
        if end_offset == 0:
            # Null terminator: skip 15 remaining bytes
            f.read(15)
            return None

        num_props = read_uint32(f)
        prop_list_len = read_uint32(f)
        name_len = read_uint8(f)
        name = read_string(f, name_len) if name_len > 0 else ""

        # Read property data
        prop_data = f.read(prop_list_len)
        props = self._parse_properties(BytesIO(prop_data), num_props)

        node = {
            'pos': pos,
            'end_offset': end_offset,
            'name': name,
            'props': props,
            'children': [],
        }

        # Parse child nodes until end_offset
        while f.tell() < end_offset:
            child = self._read_node(f)
            if child is None:
                break
            node['children'].append(child)
            # Safety: prevent seeking backward
            if f.tell() <= pos:
                f.seek(end_offset)
                break

        f.seek(end_offset)
        return node

    def _parse_properties(self, ps, num_props):
        """Parse the property list of a node."""
        props = []
        for _ in range(num_props):
            ptype_byte = ps.read(1)
            if not ptype_byte:
                break
            ptype = ptype_byte.decode('ascii', errors='replace')

            if ptype == 'S':
                slen = read_uint32(ps)
                sval = ps.read(slen).decode('utf-8', errors='replace')
                props.append(('S', sval))
            elif ptype == 'R':
                rlen = read_uint32(ps)
                rval = ps.read(rlen)
                props.append(('R', rval))
            elif ptype == 'I':
                props.append(('I', struct.unpack('<i', ps.read(4))[0]))
            elif ptype == 'i':
                props.append(('i', read_uint32(ps)))
            elif ptype == 'l':
                props.append(('l', struct.unpack('<q', ps.read(8))[0]))
            elif ptype == 'F':
                props.append(('F', struct.unpack('<d', ps.read(8))[0]))
            elif ptype == 'f':
                props.append(('f', struct.unpack('<f', ps.read(4))[0]))
            elif ptype == 'Y':
                props.append(('Y', read_uint8(ps)))
            elif ptype == 'C':
                props.append(('C', read_uint8(ps)))
            elif ptype == 'D':
                props.append(('D', read_uint64(ps)))
            elif ptype in ('d', 'f', 'i', 'l', 'b'):
                # Array property types
                # Format: encoding(1) + count(4) + length(4) + data
                # But these are embedded in the property data stream
                encoding = read_uint8(ps)
                count = read_uint32(ps)
                data_len = read_uint32(ps)
                raw = ps.read(data_len)
                if encoding == 1:
                    raw = zlib.decompress(raw)

                # Determine element format based on property type
                if ptype == 'd':
                    fmt, esize = '<d', 8
                elif ptype == 'f':
                    fmt, esize = '<f', 4
                elif ptype in ('i', 'I'):
                    fmt, esize = '<i', 4
                elif ptype == 'l':
                    fmt, esize = '<q', 8
                elif ptype == 'b':
                    fmt, esize = '<B', 1
                else:
                    props.append((ptype, []))
                    continue

                arr = list(struct.unpack(f'{len(raw)//esize}{fmt}', raw))
                props.append((ptype, arr))
            else:
                props.append((ptype, None))
        return props

    def _find_geometry(self, node):
        """Recursively search for Geometry nodes with Mesh type."""
        if node is None:
            return

        if node['name'] == 'Geometry' and node['props']:
            geom_type = ""
            ptype, pval = node['props'][0]
            if ptype == 'S':
                geom_type = pval
            elif ptype == 'R':
                geom_type = pval.decode('utf-8', errors='replace')

            if geom_type == 'Mesh':
                print(f"Found Mesh Geometry at pos={node['pos']}")
                self._extract_geometry(node)

        for child in node['children']:
            self._find_geometry(child)

    def _extract_geometry(self, node):
        """Extract vertices, faces, normals, UVs from a Geometry node."""
        for child in node['children']:
            name = child['name']
            if not child['props']:
                continue

            ptype, pval = child['props'][0]

            if name == 'Vertices':
                if ptype == 'S':
                    # String property containing binary data
                    self.vertices = list(struct.unpack(
                        f'{len(pval)//8}d', pval if len(pval) % 8 == 0 else pval[:len(pval)//8*8]
                    ))
                elif isinstance(pval, list):
                    self.vertices = pval
            elif name == 'PolygonVertexIndex':
                if isinstance(pval, list):
                    self.faces = pval
            elif name == 'Normals':
                if isinstance(pval, list):
                    self.normals = pval
            elif name == 'UV' and isinstance(pval, list):
                self.uvs = pval

        # Also check for nested UV structure (Direct + IndexToDirect)
        for child in node['children']:
            if child['name'] == 'UV':
                for gc in child['children']:
                    if gc['name'] in ('UV', 'Direct') and gc['props']:
                        ptype, pval = gc['props'][0]
                        if isinstance(pval, list):
                            self.uvs = pval
            elif child['name'] in ('Vertices', 'PolygonVertexIndex', 'Normals'):
                pass  # Already handled above

        print(f"  Vertices: {len(self.vertices)} ({len(self.vertices)//3} unique)")
        print(f"  Faces: {len(self.faces)}")
        print(f"  Normals: {len(self.normals)} ({len(self.normals)//3} unique)")
        print(f"  UVs: {len(self.uvs)} ({len(self.uvs)//2} unique)")

    def export_obj(self, output_path):
        with open(output_path, 'w') as f:
            f.write("# Converted from FBX\n")
            for i in range(0, len(self.vertices), 3):
                f.write(f"v {self.vertices[i]:.6f} {self.vertices[i+1]:.6f} {self.vertices[i+2]:.6f}\n")
            if self.normals:
                for i in range(0, len(self.normals), 3):
                    f.write(f"vn {self.normals[i]:.6f} {self.normals[i+1]:.6f} {self.normals[i+2]:.6f}\n")
            if self.uvs:
                for i in range(0, len(self.uvs), 2):
                    f.write(f"vt {self.uvs[i]:.6f} {self.uvs[i+1]:.6f}\n")

            # PolygonVertexIndex: -1 (or negative) marks end of polygon;
            # positive values (or negative with sign flipped) are vertex indices
            face_start = 0
            for i in range(len(self.faces)):
                if self.faces[i] < 0:
                    polygon = self.faces[face_start:i]
                    self.faces[face_start:i] = [v if v >= 0 else (-v - 1) for v in polygon]
                    polygon = self.faces[face_start:i]
                    if len(polygon) >= 3:
                        v0 = polygon[0] + 1
                        for j in range(1, len(polygon) - 1):
                            v1 = polygon[j] + 1
                            v2 = polygon[j + 1] + 1
                            f.write(f"f {v0} {v1} {v2}\n")
                    face_start = i + 1

            # Handle trailing polygon
            polygon = [v if v >= 0 else (-v - 1) for v in self.faces[face_start:]]
            if len(polygon) >= 3:
                v0 = polygon[0] + 1
                for j in range(1, len(polygon) - 1):
                    v1 = polygon[j] + 1
                    v2 = polygon[j + 1] + 1
                    f.write(f"f {v0} {v1} {v2}\n")

        print(f"Exported {len(self.vertices)//3} vertices, {len(self.faces)} face indices, "
              f"{len(self.normals)//3} normals, {len(self.uvs)//2} UVs")


if __name__ == '__main__':
    fbx_path = sys.argv[1] if len(sys.argv) > 1 else r"D:\x-racing\assets\models\car.fbx"
    obj_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(fbx_path)[0] + ".obj"

    print(f"Converting {fbx_path} -> {obj_path}")
    start = time.time()

    parser = FBXParser(fbx_path)
    parser.parse()
    parser.export_obj(obj_path)

    elapsed = time.time() - start
    print(f"Conversion complete in {elapsed:.2f} s")
