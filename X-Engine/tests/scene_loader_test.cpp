#include <gtest/gtest.h>
#include <cstdio>
#include "rendering/scene_loader.h"

TEST(SceneLoaderTest, ToJsonContainsCamera) {
    xe::Scene s;
    s.camera_position = { 0.0f, 1.0f, -5.0f };
    auto j = xe::SceneLoader::ToJson(s);
    EXPECT_NE(j.find("camera_position"), std::string::npos);
    EXPECT_NE(j.find("objects"), std::string::npos);
}

TEST(SceneLoaderTest, RoundTripPreservesObjects) {
    xe::Scene src;
    src.camera_position = { 1.0f, 2.0f, 3.0f };
    src.camera_target   = { 0.0f, 0.0f, 0.0f };
    src.camera_fov_y = 0.9f;

    xe::SceneObject obj;
    obj.name = "test_cube";
    obj.instance.mesh = xe::MeshKind::Cube;
    obj.instance.position = { 1.0f, 0.0f, 0.0f };
    obj.instance.tint = { 0.5f, 0.6f, 0.7f, 1.0f };
    src.objects.push_back(obj);

    auto j = xe::SceneLoader::ToJson(src);
    auto dst = xe::SceneLoader::FromJson(j);

    EXPECT_FLOAT_EQ(dst.camera_position.x, 1.0f);
    EXPECT_FLOAT_EQ(dst.camera_position.y, 2.0f);
    EXPECT_FLOAT_EQ(dst.camera_position.z, 3.0f);
    EXPECT_NEAR(dst.camera_fov_y, 0.9f, 1e-4f);
    ASSERT_EQ(dst.objects.size(), 1u);
    EXPECT_EQ(dst.objects[0].name, "test_cube");
    EXPECT_EQ(dst.objects[0].instance.mesh, xe::MeshKind::Cube);
    EXPECT_FLOAT_EQ(dst.objects[0].instance.position.x, 1.0f);
}

TEST(SceneLoaderTest, FromJsonHandlesEmpty) {
    auto s = xe::SceneLoader::FromJson("{}");
    EXPECT_EQ(s.objects.size(), 0u);
}

TEST(SceneLoaderTest, FromJsonHandlesMultipleObjects) {
    const char* j = R"({
      "camera_position": [0,0,-5],
      "camera_target": [0,0,0],
      "objects": [
        {"name":"a","mesh":"cube","position":[1,0,0]},
        {"name":"b","mesh":"triangle","position":[-1,0,0]}
      ]
    })";
    auto s = xe::SceneLoader::FromJson(j);
    ASSERT_EQ(s.objects.size(), 2u);
    EXPECT_EQ(s.objects[0].name, "a");
    EXPECT_EQ(s.objects[0].instance.mesh, xe::MeshKind::Cube);
    EXPECT_EQ(s.objects[1].name, "b");
    EXPECT_EQ(s.objects[1].instance.mesh, xe::MeshKind::Triangle);
}

TEST(SceneLoaderTest, SaveAndLoadFromFile) {
    const std::string p = "test_scene.json";
    xe::Scene s;
    xe::SceneObject o;
    o.name = "from_file";
    o.instance.position = { 2.0f, 0.0f, 0.0f };
    s.objects.push_back(o);

    ASSERT_TRUE(xe::SceneLoader::SaveToFile(s, p));

    auto loaded = xe::SceneLoader::LoadFromFile(p);
    ASSERT_EQ(loaded.objects.size(), 1u);
    EXPECT_EQ(loaded.objects[0].name, "from_file");
    EXPECT_FLOAT_EQ(loaded.objects[0].instance.position.x, 2.0f);

    std::remove(p.c_str());
}

TEST(SceneLoaderTest, TexturePathRoundTrip) {
    xe::Scene s;
    xe::SceneObject o;
    o.name = "textured";
    o.instance.mesh = xe::MeshKind::Quad;
    o.instance.texture_path = "assets/checker.png";
    s.objects.push_back(o);

    auto j = xe::SceneLoader::ToJson(s);
    auto back = xe::SceneLoader::FromJson(j);
    ASSERT_EQ(back.objects.size(), 1u);
    EXPECT_EQ(back.objects[0].instance.texture_path, "assets/checker.png");
}