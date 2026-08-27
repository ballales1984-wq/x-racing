#pragma once

#include "mesh.h"
#include <string>
#include <vector>

// Project 0 — Assimp-based mesh/animation loader
// Namespace: p0::assets
//
// Requires Assimp (enabled via PROJECT0_BUILD_ASSIMP).
// Loads FBX, GLB/GLTF, OBJ, and many other formats.
// Extracts:
//  - mesh geometry (vertices, normals, UVs, colors)
//  - materials (diffuse color, textures)
//  - animation data (bones, keyframes, tracks)
namespace p0::assets {

struct BoneData {
    std::string name;
    int parent_index;
    std::vector<float> local_transform;  // 4x4 row-major
    std::vector<float> offset_matrix;    // 4x4 row-major
};

struct VertexWeight {
    int vertex_id;
    float weight;
};

struct Keyframe {
    double time;
    std::vector<float> value;  // position (3) or rotation (4 quaternion) or scale (3)
};

struct AnimationTrack {
    std::string bone_name;
    std::vector<Keyframe> position_keys;
    std::vector<Keyframe> rotation_keys;
    std::vector<Keyframe> scaling_keys;
};

struct AnimationData {
    std::string name;
    double duration_seconds;
    double ticks_per_second;
    std::vector<AnimationTrack> tracks;
};

struct SkinnedMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<int> indices;
    std::vector<int> bone_ids;      // 4 bones per vertex
    std::vector<float> bone_weights; // 4 weights per vertex
    std::vector<BoneData> bones;
    std::vector<AnimationData> animations;
    std::vector<Material> materials;
    std::string material_name;
};

class AssimpLoader {
public:
    // Load mesh from any Assimp-supported format.
    static bool LoadMesh(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials);

    // Load skinned mesh with animation data from FBX/GLB.
    static bool LoadSkinnedMesh(const std::string& filename, SkinnedMesh& out_mesh);

    // Check if Assimp is available.
    static bool IsAvailable();
};

}
