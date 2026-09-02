#include <gtest/gtest.h>
#include "rendering/meshes.h"

TEST(MeshTest, CubeHas24Vertices) {
    auto verts = xe::MakeCubeVertices();
    EXPECT_EQ(verts.size(), 24u);
}

TEST(MeshTest, CubeHas36Indices) {
    auto idx = xe::MakeCubeIndices();
    EXPECT_EQ(idx.size(), 36u);
}

TEST(MeshTest, CubeIndicesAllValid) {
    auto verts = xe::MakeCubeVertices();
    auto idx = xe::MakeCubeIndices();
    for (auto i : idx) {
        EXPECT_LT(i, verts.size());
    }
}

TEST(MeshTest, CubeIndexCountDivisibleByThree) {
    auto idx = xe::MakeCubeIndices();
    EXPECT_EQ(idx.size() % 3, 0u);
}

TEST(MeshTest, CubeFacesHaveUniqueColors) {
    auto verts = xe::MakeCubeVertices();
    // 6 faces, 4 verts each. Each face must be a uniform color.
    for (int face = 0; face < 6; ++face) {
        const auto& c0 = verts[face * 4].color;
        for (int j = 1; j < 4; ++j) {
            const auto& cj = verts[face * 4 + j].color;
            EXPECT_FLOAT_EQ(c0[0], cj[0]);
            EXPECT_FLOAT_EQ(c0[1], cj[1]);
            EXPECT_FLOAT_EQ(c0[2], cj[2]);
        }
    }
}

TEST(MeshTest, CubeFacesHaveFaceNormals) {
    auto verts = xe::MakeCubeVertices();
    // Each face must have all 4 vertices with the same normal (face normal).
    for (int face = 0; face < 6; ++face) {
        const auto& n0 = verts[face * 4].normal;
        for (int j = 1; j < 4; ++j) {
            const auto& nj = verts[face * 4 + j].normal;
            EXPECT_FLOAT_EQ(n0[0], nj[0]);
            EXPECT_FLOAT_EQ(n0[1], nj[1]);
            EXPECT_FLOAT_EQ(n0[2], nj[2]);
        }
    }
}

TEST(MeshTest, CubeFaceNormalsAreUnitLength) {
    auto verts = xe::MakeCubeVertices();
    for (size_t i = 0; i < verts.size(); ++i) {
        const auto& n = verts[i].normal;
        float l = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        EXPECT_NEAR(l, 1.0f, 1e-5f);
    }
}

TEST(MeshTest, CubeFaceNormalsMatchFaceDirection) {
    auto verts = xe::MakeCubeVertices();
    // +X face index 0..3 should have normal (1,0,0)
    EXPECT_NEAR(verts[0].normal[0], 1.0f, 1e-5f);
    EXPECT_NEAR(verts[0].normal[1], 0.0f, 1e-5f);
    EXPECT_NEAR(verts[0].normal[2], 0.0f, 1e-5f);
    // -X face index 4..7 should have normal (-1,0,0)
    EXPECT_NEAR(verts[4].normal[0], -1.0f, 1e-5f);
    // +Y face index 8..11 should have normal (0,1,0)
    EXPECT_NEAR(verts[8].normal[1], 1.0f, 1e-5f);
    // +Z face index 16..19 should have normal (0,0,1)
    EXPECT_NEAR(verts[16].normal[2], 1.0f, 1e-5f);
}