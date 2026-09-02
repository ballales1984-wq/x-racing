#include <gtest/gtest.h>
#include "rendering/scene.h"

TEST(UVLayoutTest, VertexHasUVField) {
    xe::Vertex v{};
    v.uv[0] = 0.25f;
    v.uv[1] = 0.75f;
    EXPECT_FLOAT_EQ(v.uv[0], 0.25f);
    EXPECT_FLOAT_EQ(v.uv[1], 0.75f);
}

TEST(UVLayoutTest, TexturedQuadHasUVs) {
    auto q = xe::MeshData::MakeTexturedQuad();
    ASSERT_EQ(q.vertices.size(), 4u);
    // bottom-left → uv (0,1), bottom-right → (1,1), top-right → (1,0), top-left → (0,0)
    EXPECT_FLOAT_EQ(q.vertices[0].uv[0], 0.0f);
    EXPECT_FLOAT_EQ(q.vertices[0].uv[1], 1.0f);
    EXPECT_FLOAT_EQ(q.vertices[1].uv[0], 1.0f);
    EXPECT_FLOAT_EQ(q.vertices[1].uv[1], 1.0f);
    EXPECT_FLOAT_EQ(q.vertices[2].uv[0], 1.0f);
    EXPECT_FLOAT_EQ(q.vertices[2].uv[1], 0.0f);
    EXPECT_FLOAT_EQ(q.vertices[3].uv[0], 0.0f);
    EXPECT_FLOAT_EQ(q.vertices[3].uv[1], 0.0f);
}

TEST(UVLayoutTest, CubeFacesHaveUVsInRange) {
    auto c = xe::MeshData::MakeCube();
    for (const auto& v : c.vertices) {
        EXPECT_GE(v.uv[0], 0.0f);
        EXPECT_LE(v.uv[0], 1.0f);
        EXPECT_GE(v.uv[1], 0.0f);
        EXPECT_LE(v.uv[1], 1.0f);
    }
}

TEST(UVLayoutTest, TexturePathOnInstance) {
    xe::MeshInstance m;
    EXPECT_TRUE(m.texture_path.empty());
    m.texture_path = "assets/foo.png";
    EXPECT_EQ(m.texture_path, "assets/foo.png");
}