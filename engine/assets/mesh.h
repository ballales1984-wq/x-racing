#pragma once

#include "common.h"
#include <string>
#include <vector>

// Project 0 — asset loading and mesh representation
// Namespace: p0::assets
namespace p0::assets {

// Simple mesh container: positions, normals, UVs, colors, material and triangle indices.
// Loaded from OBJ/MTL files via MeshLoader. Colors come from the material.
struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<Vec3> colors;
    std::vector<int> indices;
    std::string material;
};

// Material definition parsed from MTL files.
struct Material {
    std::string name;
    Vec3 diffuse{0.7, 0.7, 0.7};
    Vec3 ambient{0.2, 0.2, 0.2};
    Vec3 specular{0.0, 0.0, 0.0};
    double shininess = 0.0;
    double alpha = 1.0;
};

// OBJ/MTL mesh loader: reads Wavefront .obj and .mtl files into Mesh/Material structs.
// Supports v (vertices), vn (normals), vt (UVs), f (faces), usemtl, and MTL materials.
class MeshLoader {
public:
    static bool LoadOBJ(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials);
};

}
