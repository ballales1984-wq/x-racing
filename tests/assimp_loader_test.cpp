// Project 0 — unit tests for the Assimp-based mesh/animation loader
#include <gtest/gtest.h>
#include "assets/assimp_loader.h"
#include "assets/mesh.h"

using namespace p0::assets;

// Assimp should be available in this build (PROJECT0_BUILD_ASSIMP=ON).
TEST(AssimpLoader, IsAvailable) {
  EXPECT_TRUE(AssimpLoader::IsAvailable());
}

// A real FBX asset should load into a populated mesh.
TEST(AssimpLoader, LoadsFbxMesh) {
  Mesh mesh;
  std::vector<Material> materials;
  const bool ok = AssimpLoader::LoadMesh("D:/x-racing/assets/models/car.fbx", mesh, materials);
  ASSERT_TRUE(ok);
  EXPECT_GT(mesh.vertices.size(), 0u);
  EXPECT_GT(mesh.indices.size(), 0u);
  EXPECT_EQ(mesh.indices.size() % 3, 0u);
  EXPECT_GT(materials.size(), 0u);
}

// A missing file should fail cleanly without throwing.
TEST(AssimpLoader, MissingFileReturnsFalse) {
  Mesh mesh;
  std::vector<Material> materials;
  EXPECT_FALSE(AssimpLoader::LoadMesh("D:/x-racing/assets/models/does_not_exist.fbx", mesh, materials));
}

// Skinned mesh loading should succeed and report geometry + skeleton.
TEST(AssimpLoader, LoadsSkinnedMesh) {
  SkinnedMesh skinned;
  const bool ok = AssimpLoader::LoadSkinnedMesh("D:/x-racing/assets/models/car.fbx", skinned);
  ASSERT_TRUE(ok);
  EXPECT_GT(skinned.positions.size(), 0u);
  EXPECT_GT(skinned.indices.size(), 0u);
  // bone_ids/bone_weights are emitted per vertex (4 each).
  EXPECT_EQ(skinned.bone_ids.size(), skinned.positions.size() / 3u * 4u);
}
