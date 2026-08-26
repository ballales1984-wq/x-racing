#pragma once

#include "common.h"
#include <string>
#include <vector>

namespace p0::assets {

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<int> indices;
};

class MeshLoader {
public:
    static bool LoadOBJ(const std::string& filename, Mesh& out_mesh);
};

}
