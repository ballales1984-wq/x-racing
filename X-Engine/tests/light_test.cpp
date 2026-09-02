#include <gtest/gtest.h>
#include "rendering/light.h"
#include <cmath>

TEST(LightingTest, DefaultDirectionIsNonZero) {
    xe::DirectionalLight l;
    float len = std::sqrt(l.direction.x * l.direction.x +
                          l.direction.y * l.direction.y +
                          l.direction.z * l.direction.z);
    EXPECT_GT(len, 0.5f);
}

// Convention: light.direction points FROM surface TOWARD the light source.

TEST(LightingTest, LambertFullWhenSurfaceFacesLight) {
    xe::DirectionalLight l;
    l.direction = { 0.0f, 1.0f, 0.0f };   // light is directly above
    xe::Vec3 n{ 0.0f, 1.0f, 0.0f };       // surface normal up (faces light)
    xe::Vec3 p{ 0.0f, 0.0f, 0.0f };
    xe::Vec3 cam{ 0.0f, 5.0f, 5.0f };
    auto r = xe::ComputeLighting(n, p, l, cam, 32.0f, 0.0f);
    EXPECT_GT(r.diffuse[0], 1.0f);
}

TEST(LightingTest, LambertZeroWhenSurfaceFacesAway) {
    xe::DirectionalLight l;
    l.direction = { 0.0f, 1.0f, 0.0f };
    xe::Vec3 n{ 0.0f, -1.0f, 0.0f };  // normal points down (away from light)
    xe::Vec3 p{ 0.0f, 0.0f, 0.0f };
    xe::Vec3 cam{ 0.0f, 5.0f, 5.0f };
    auto r = xe::ComputeLighting(n, p, l, cam, 32.0f, 0.5f);
    EXPECT_NEAR(r.diffuse[0], 0.0f, 1e-3f);
}

TEST(LightingTest, SpecularNegligibleWhenNormalAway) {
    xe::DirectionalLight l;
    l.direction = { 0.0f, 1.0f, 0.0f };
    xe::Vec3 n{ 0.0f, -1.0f, 0.0f };
    xe::Vec3 p{ 0.0f, 0.0f, 0.0f };
    xe::Vec3 cam{ 0.0f, 5.0f, 5.0f };
    auto r = xe::ComputeLighting(n, p, l, cam, 32.0f, 0.5f);
    EXPECT_LT(r.specular[0], 0.01f);
}

TEST(LightingTest, IntensityScalesDiffuse) {
    xe::DirectionalLight l;
    l.direction = { 0.0f, 1.0f, 0.0f };
    l.intensity = 1.0f;
    xe::Vec3 n{ 0.0f, 1.0f, 0.0f };
    xe::Vec3 p{ 0.0f, 0.0f, 0.0f };
    xe::Vec3 cam{ 0.0f, 5.0f, 5.0f };
    auto r1 = xe::ComputeLighting(n, p, l, cam, 32.0f, 0.0f);

    l.intensity = 2.0f;
    auto r2 = xe::ComputeLighting(n, p, l, cam, 32.0f, 0.0f);
    EXPECT_NEAR(r2.diffuse[0], r1.diffuse[0] * 2.0f, 1e-3f);
}

TEST(LightingTest, SpecularComponentsNonNegative) {
    xe::DirectionalLight l;
    l.direction = { 0.0f, 1.0f, 0.0f };
    xe::Vec3 n{ 0.0f, 1.0f, 0.0f };
    xe::Vec3 p{ 0.0f, 0.0f, 0.0f };
    xe::Vec3 cam{ 0.0f, 5.0f, 5.0f };
    auto sharp = xe::ComputeLighting(n, p, l, cam, 128.0f, 1.0f);
    auto soft  = xe::ComputeLighting(n, p, l, cam,   4.0f, 1.0f);
    EXPECT_GE(sharp.specular[0], 0.0f);
    EXPECT_GE(soft.specular[0], 0.0f);
}