#pragma once

#include "common.h"
#include <string>
#include <vector>

// Project 0 — asset loading and mesh representation
// Namespace: p0::assets
namespace p0::assets {

// Simple mesh container: positions, normals, UVs, colors and triangle indices.
// Loaded from OBJ files via MeshLoader. Colors are optional.
struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<Vec3> colors;
    std::vector<int> indices;
};

// OBJ mesh loader: reads Wavefront .obj files into a Mesh struct.
// Supports v (vertices), vn (normals), vt (UVs) and f (faces).
class MeshLoader {
public:
    static bool LoadOBJ(const std::string& filename, Mesh& out_mesh);
};

}
