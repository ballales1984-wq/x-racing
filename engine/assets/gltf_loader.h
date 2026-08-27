#pragma once

#include "mesh.h"
#include <string>
#include <vector>

// Project 0 — minimal GLB/GLTF loader
// Parses binary glTF 2.0 files without external dependencies.
// Supports:
//  - triangle meshes
//  - positions, normals, UVs, colors
//  - PBR material base color
//  - skinning data (bone weights/indices) when present
//  - animation data (keyframe tracks) when present
namespace p0::assets {

struct GLTFMaterial {
    std::string name;
    Vec3 base_color{0.7, 0.7, 0.7};
    float metallic = 0.0f;
    float roughness = 1.0f;
    float alpha = 1.0f;
};

struct GLTFBone {
    std::string name;
    int parent_index;
    std::vector<float> local_transform; // 4x4 row-major
    std::vector<float> offset_matrix;   // 4x4 row-major
};

struct GLTFKeyframe {
    double time;
    std::vector<float> value;
};

struct GLTFTrack {
    std::string bone_name;
    std::vector<GLTFKeyframe> positions;
    std::vector<GLTFKeyframe> rotations;
    std::vector<GLTFKeyframe> scales;
};

struct GLTFAnimation {
    std::string name;
    double duration_seconds;
    std::vector<GLTFTrack> tracks;
};

struct GLTFSkinnedMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<int> indices;
    std::vector<int> bone_ids;      // 4 per vertex
    std::vector<float> bone_weights; // 4 per vertex
    std::vector<GLTFBone> bones;
    std::vector<GLTFAnimation> animations;
    std::vector<GLTFMaterial> materials;
    std::string material_name;
};

class GLTFLoader {
public:
    // Load static GLB/GLTF mesh.
    static bool Load(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials);

    // Load skinned GLB/GLTF mesh with animations.
    static bool LoadSkinned(const std::string& filename, GLTFSkinnedMesh& out_mesh);
};

}
