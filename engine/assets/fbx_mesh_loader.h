#pragma once

#include <string>

namespace p0::assets {
struct Mesh;
}

// Load an FBX file into a Mesh with per-vertex colors from the FBX materials.
// Returns true on success. Requires Assimp (PROJECT0_BUILD_ASSIMP=ON).
bool LoadFBXMesh(const std::string& filename, p0::assets::Mesh& out_mesh);

