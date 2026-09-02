#include "fbx_mesh_loader.h"
#include "assimp_loader.h"
#include "mesh.h"

bool LoadFBXMesh(const std::string& filename, p0::assets::Mesh& out_mesh) {
    std::vector<p0::assets::Material> materials;
    return p0::assets::AssimpLoader::LoadMesh(filename, out_mesh, materials);
}
