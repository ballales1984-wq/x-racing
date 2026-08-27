#include "assimp_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <cmath>
#include <algorithm>

// Project 0 — Assimp-based mesh/animation loader implementation
// Loads FBX, GLB/GLTF, OBJ and other formats via Assimp.
// Extracts geometry, materials, bones, and animation tracks.
namespace p0::assets {

namespace {

Vec3 ToVec3(const aiVector3D& v) { return Vec3(v.x, v.y, v.z); }
Vec2 ToVec2(const aiVector2D& v) { return Vec2(v.x, v.y); }
Vec3 ToVec3(const aiColor3D& c) { return Vec3(c.r, c.g, c.b); }
Vec3 ToVec3(const aiColor4D& c) { return Vec3(c.r, c.g, c.b); }

std::vector<float> Mat4ToArray(const aiMatrix4x4& m) {
    std::vector<float> result(16);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i * 4 + j] = m[i][j];
        }
    }
    return result;
}

std::vector<float> Vec3ToArray(const aiVector3D& v) {
    return {static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

std::vector<float> QuatToArray(const aiQuaternion& q) {
    return {static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z), static_cast<float>(q.w)};
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<BoneData>& bones, int parent_index) {
    if (!node) return;

    BoneData bone;
    bone.name = node->mName.C_Str();
    bone.parent_index = parent_index;
    bone.local_transform = Mat4ToArray(node->mTransformation);
    bone.offset_matrix = Mat4ToArray(aiMatrix4x4());

    int current_index = static_cast<int>(bones.size());
    bones.push_back(bone);

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, bones, current_index);
    }
}

AnimationTrack ProcessAnimTrack(const aiAnimation* anim, const aiNodeAnim* track) {
    AnimationTrack result;
    result.bone_name = track->mNodeName.C_Str();

    for (unsigned int i = 0; i < track->mNumPositionKeys; ++i) {
        Keyframe kf;
        kf.time = track->mPositionKeys[i].mTime;
        kf.value = Vec3ToArray(track->mPositionKeys[i].mValue);
        result.position_keys.push_back(kf);
    }

    for (unsigned int i = 0; i < track->mNumRotationKeys; ++i) {
        Keyframe kf;
        kf.time = track->mRotationKeys[i].mTime;
        kf.value = QuatToArray(track->mRotationKeys[i].mValue);
        result.rotation_keys.push_back(kf);
    }

    for (unsigned int i = 0; i < track->mNumScalingKeys; ++i) {
        Keyframe kf;
        kf.time = track->mScalingKeys[i].mTime;
        kf.value = Vec3ToArray(track->mScalingKeys[i].mValue);
        result.scaling_keys.push_back(kf);
    }

    return result;
}

}

bool AssimpLoader::IsAvailable() {
#ifdef ASSIMP_AVAILABLE
    return true;
#else
    return false;
#endif
}

bool AssimpLoader::LoadMesh(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials) {
#ifndef ASSIMP_AVAILABLE
    std::cerr << "Assimp not available. Build with PROJECT0_BUILD_ASSIMP=ON." << std::endl;
    return false;
#else
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;

    const aiScene* scene = importer.ReadFile(filename, flags);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp failed to load: " << filename << std::endl;
        std::cerr << "Error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    out_mesh = Mesh();
    out_materials.clear();

    for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
        Material mat;
        mat.name = scene->mMaterials[m]->GetName().C_Str();

        aiColor3D color(0.7f, 0.7f, 0.7f);
        if (scene->mMaterials[m]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            mat.diffuse = ToVec3(color);
        }
        if (scene->mMaterials[m]->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            mat.ambient = ToVec3(color);
        }
        if (scene->mMaterials[m]->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            mat.specular = ToVec3(color);
        }
        float shininess = 0.0f;
        if (scene->mMaterials[m]->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            mat.shininess = shininess;
        }
        float alpha = 1.0f;
        if (scene->mMaterials[m]->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS) {
            mat.alpha = alpha;
        }

        out_materials.push_back(mat);
    }

    if (out_materials.empty()) {
        Material default_mat;
        default_mat.name = "default";
        default_mat.diffuse = Vec3(0.7, 0.7, 0.7);
        out_materials.push_back(default_mat);
    }

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        Material mesh_mat = out_materials[std::min(mesh->mMaterialIndex, scene->mNumMaterials - 1)];

        size_t vertex_offset = out_mesh.vertices.size();

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            if (mesh->mVertices) {
                out_mesh.vertices.push_back(ToVec3(mesh->mVertices[v]));
            }
            if (mesh->mNormals) {
                out_mesh.normals.push_back(ToVec3(mesh->mNormals[v]));
            }
            if (mesh->mTextureCoords[0]) {
                const aiVector3D& uv = mesh->mTextureCoords[0][v];
                out_mesh.uvs.push_back(Vec2(uv.x, uv.y));
            }
            out_mesh.colors.push_back(mesh_mat.diffuse);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;

            for (unsigned int i = 0; i < 3; ++i) {
                out_mesh.indices.push_back(static_cast<int>(vertex_offset + face.mIndices[i]));
            }
        }
    }

    out_mesh.material = out_materials.empty() ? "" : out_materials[0].name;

    std::cout << "Loaded via Assimp: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.vertices.size() << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;
    std::cout << "  Materials: " << out_materials.size() << std::endl;

    return true;
#endif
}

bool AssimpLoader::LoadSkinnedMesh(const std::string& filename, SkinnedMesh& out_mesh) {
#ifndef ASSIMP_AVAILABLE
    std::cerr << "Assimp not available. Build with PROJECT0_BUILD_ASSIMP=ON." << std::endl;
    return false;
#else
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;

    const aiScene* scene = importer.ReadFile(filename, flags);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp failed to load: " << filename << std::endl;
        std::cerr << "Error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    out_mesh = SkinnedMesh();

    // Process skeleton
    if (scene->mRootNode) {
        ProcessNode(scene->mRootNode, scene, out_mesh.bones, -1);
    }

    // Process animations
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        AnimationData anim;
        anim.name = scene->mAnimations[a]->mName.C_Str();
        anim.duration_seconds = scene->mAnimations[a]->mDuration;
        anim.ticks_per_second = scene->mAnimations[a]->mTicksPerSecond;

        for (unsigned int c = 0; c < scene->mAnimations[a]->mNumChannels; ++c) {
            const aiNodeAnim* track = scene->mAnimations[a]->mChannels[c];
            anim.tracks.push_back(ProcessAnimTrack(scene->mAnimations[a], track));
        }

        out_mesh.animations.push_back(anim);
    }

    // Process first mesh
    if (scene->mNumMeshes > 0) {
        const aiMesh* mesh = scene->mMeshes[0];

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            if (mesh->mVertices) {
                out_mesh.positions.push_back(mesh->mVertices[v].x);
                out_mesh.positions.push_back(mesh->mVertices[v].y);
                out_mesh.positions.push_back(mesh->mVertices[v].z);
            }
            if (mesh->mNormals) {
                out_mesh.normals.push_back(mesh->mNormals[v].x);
                out_mesh.normals.push_back(mesh->mNormals[v].y);
                out_mesh.normals.push_back(mesh->mNormals[v].z);
            }
            if (mesh->mTextureCoords[0]) {
                out_mesh.uvs.push_back(mesh->mTextureCoords[0][v].x);
                out_mesh.uvs.push_back(mesh->mTextureCoords[0][v].y);
            }

            if (mesh->mBones && v < mesh->mNumVertices) {
                int bone_ids[4] = {0, 0, 0, 0};
                float bone_weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

                for (unsigned int b = 0; b < mesh->mBones[v]->mNumWeights && b < 4; ++b) {
                    bone_ids[b] = mesh->mBones[v]->mWeights[b].mVertexId;
                    bone_weights[b] = mesh->mBones[v]->mWeights[b].mWeight;
                }

                out_mesh.bone_ids.push_back(bone_ids[0]);
                out_mesh.bone_ids.push_back(bone_ids[1]);
                out_mesh.bone_ids.push_back(bone_ids[2]);
                out_mesh.bone_ids.push_back(bone_ids[3]);
                out_mesh.bone_weights.push_back(bone_weights[0]);
                out_mesh.bone_weights.push_back(bone_weights[1]);
                out_mesh.bone_weights.push_back(bone_weights[2]);
                out_mesh.bone_weights.push_back(bone_weights[3]);
            } else {
                out_mesh.bone_ids.push_back(0);
                out_mesh.bone_ids.push_back(0);
                out_mesh.bone_ids.push_back(0);
                out_mesh.bone_ids.push_back(0);
                out_mesh.bone_weights.push_back(1.0f);
                out_mesh.bone_weights.push_back(0.0f);
                out_mesh.bone_weights.push_back(0.0f);
                out_mesh.bone_weights.push_back(0.0f);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int i = 0; i < 3; ++i) {
                out_mesh.indices.push_back(static_cast<int>(face.mIndices[i]));
            }
        }

        // Process materials
        for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
            Material mat;
            mat.name = scene->mMaterials[m]->GetName().C_Str();
            aiColor3D color(0.7f, 0.7f, 0.7f);
            if (scene->mMaterials[m]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                mat.diffuse = ToVec3(color);
            }
            out_mesh.materials.push_back(mat);
        }
        if (out_mesh.materials.empty() && mesh->mMaterialIndex < scene->mNumMaterials) {
            Material mat;
            mat.name = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
            aiColor3D color(0.7f, 0.7f, 0.7f);
            if (scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                mat.diffuse = ToVec3(color);
            }
            out_mesh.materials.push_back(mat);
        }
        out_mesh.material_name = out_mesh.materials.empty() ? "" : out_mesh.materials[0].name;
    }

    std::cout << "Loaded skinned mesh via Assimp: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.positions.size() / 3 << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;
    std::cout << "  Bones: " << out_mesh.bones.size() << std::endl;
    std::cout << "  Animations: " << out_mesh.animations.size() << std::endl;

    return true;
#endif
}

}
