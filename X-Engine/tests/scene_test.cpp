#include <gtest/gtest.h>
#include "rendering/scene.h"

TEST(SceneTest, DefaultSceneEmpty) {
    xe::Scene s;
    EXPECT_EQ(s.objects.size(), 0u);
}

TEST(SceneTest, GetViewMatrixNonIdentity) {
    xe::Scene s;
    s.camera_position = { 0.0f, 0.0f, -5.0f };
    auto v = s.GetViewMatrix();
    EXPECT_NE(v.m[2][3], 0.0f);
}

TEST(SceneTest, MeshInstanceWorldMatrix) {
    xe::MeshInstance mi;
    mi.position = { 1.0f, 2.0f, 3.0f };
    mi.rotation_rad = { 0.0f, 1.57f, 0.0f };
    auto m = mi.GetWorldMatrix();
    // After rotation around Y by 90deg and translation, position.x translation
    // shifts to -3 on the new x axis.
    EXPECT_GT(m.m[0][3], 0.5f);  // x roughly +1
    EXPECT_GT(m.m[1][3], 1.5f);  // y == 2 unchanged
}

TEST(SceneTest, MeshDataFactoryKinds) {
    auto cube = xe::MeshData::MakeCube();
    auto tri  = xe::MeshData::MakeTriangle();
    auto quad = xe::MeshData::MakeQuad();
    EXPECT_EQ(cube.kind, xe::MeshKind::Cube);
    EXPECT_EQ(tri.kind,  xe::MeshKind::Triangle);
    EXPECT_EQ(quad.kind, xe::MeshKind::Quad);
    EXPECT_EQ(cube.indices.size(), 36u);
    EXPECT_EQ(tri.indices.size(),  3u);
    EXPECT_EQ(quad.indices.size(), 6u);
}

TEST(SceneTest, AddObjects) {
    xe::Scene s;
    xe::SceneObject obj1;
    obj1.name = "cube_a";
    obj1.instance.position = { 1.0f, 0.0f, 0.0f };
    s.objects.push_back(obj1);
    EXPECT_EQ(s.objects.size(), 1u);
    EXPECT_EQ(s.objects[0].name, "cube_a");
}