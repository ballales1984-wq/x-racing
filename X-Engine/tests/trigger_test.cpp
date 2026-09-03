#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include "debug/console.h"
#include <cstdio>

namespace {

constexpr float kEps = 1e-3f;

// --- Triggers --------------------------------------------------------------

TEST(TriggerTest, TriggerDetectsOverlappingSphere) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    // Static trigger (sensor) sphere.
    xe::RigidBody trig;
    trig.shape = xe::ShapeKind::Sphere;
    trig.position = { 0, 0, 0 };
    trig.radius = 1.0f;
    trig.isTrigger = true;
    trig.dynamic = false;
    int trigIdx = w.Add(trig);
    // Dynamic sphere.
    xe::RigidBody ball;
    ball.shape = xe::ShapeKind::Sphere;
    ball.position = { 1.5f, 0, 0 };   // outside
    ball.radius = 0.5f;
    w.Add(ball);

    w.Step(0.016f);
    // Ball is outside, no event.
    EXPECT_TRUE(w.LastTriggerEvents().empty());
    EXPECT_FALSE(w.IsOverlapping(trigIdx, 1));

    // Move the ball inside the trigger.
    w.Get(1).position = { 0.3f, 0, 0 };
    w.Step(0.016f);
    // Should have one "enter" event.
    bool foundEnter = false;
    for (const auto& e : w.LastTriggerEvents()) {
        if (e.enter) foundEnter = true;
    }
    EXPECT_TRUE(foundEnter);
    EXPECT_TRUE(w.IsOverlapping(trigIdx, 1));
}

TEST(TriggerTest, TriggerEmitsExit) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody trig;
    trig.shape = xe::ShapeKind::Sphere;
    trig.position = { 0, 0, 0 };
    trig.radius = 1.0f;
    trig.isTrigger = true;
    trig.dynamic = false;
    int trigIdx = w.Add(trig);
    xe::RigidBody ball;
    ball.shape = xe::ShapeKind::Sphere;
    ball.position = { 0.3f, 0, 0 };
    ball.radius = 0.5f;
    w.Add(ball);

    // Step: enter.
    w.Step(0.016f);
    // Move ball outside, step: exit.
    w.Get(1).position = { 5, 0, 0 };
    w.Step(0.016f);
    bool foundExit = false;
    for (const auto& e : w.LastTriggerEvents()) {
        if (!e.enter) foundExit = true;
    }
    EXPECT_TRUE(foundExit);
    EXPECT_FALSE(w.IsOverlapping(trigIdx, 1));
}

TEST(TriggerTest, TriggerDoesNotPushBack) {
    // Ball moving fast INTO trigger; should not slow down.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody trig;
    trig.shape = xe::ShapeKind::Sphere;
    trig.position = { 0, 0, 0 };
    trig.radius = 1.0f;
    trig.isTrigger = true;
    trig.dynamic = false;
    w.Add(trig);
    xe::RigidBody ball;
    ball.shape = xe::ShapeKind::Sphere;
    ball.position = { 5, 0, 0 };
    ball.radius = 0.5f;
    ball.velocity = { -1, 0, 0 };
    w.Add(ball);
    w.Step(0.016f);
    // Ball at 5 - 1*0.016 = 4.984 (no impulse from trigger).
    EXPECT_NEAR(w.Get(1).position.x, 4.984f, 0.01f);
}

TEST(TriggerTest, IsOverlappingInvalidReturnsFalse) {
    xe::PhysicsWorld w;
    EXPECT_FALSE(w.IsOverlapping(0, 1));
    EXPECT_FALSE(w.IsOverlapping(5, 10));
}

TEST(TriggerTest, ClearTriggerEvents) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody trig;
    trig.isTrigger = true; trig.dynamic = false;
    trig.shape = xe::ShapeKind::Sphere;
    w.Add(trig);
    w.AddSphere({ 0, 0, 0 }, 0.5f);
    w.Step(0.016f);
    EXPECT_FALSE(w.LastTriggerEvents().empty());
    w.ClearTriggerEvents();
    EXPECT_TRUE(w.LastTriggerEvents().empty());
}

// --- Batch script ----------------------------------------------------------

TEST(ConsoleScriptTest, RunScriptFileExecutesLines) {
    xe::Console c;
    int counter = 0;
    c.Register("inc", "increment counter", [&counter](const auto&) { counter++; });
    c.Register("add", "add N to counter", [&counter](const auto& args) {
        if (args.size() >= 2) counter += std::stoi(args[1]);
    });
    // Write a script file.
    const char* path = "test_script.xescript";
    {
        FILE* fp = std::fopen(path, "w");
        ASSERT_NE(fp, nullptr);
        std::fputs("# this is a comment\n", fp);
        std::fputs("inc\n", fp);
        std::fputs("\n", fp);   // blank line
        std::fputs("  add 5\n", fp);   // leading whitespace
        std::fputs("inc\n", fp);
        std::fclose(fp);
    }
    int n = c.RunScriptFile(path);
    EXPECT_EQ(n, 3);
    EXPECT_EQ(counter, 7);
    std::remove(path);
}

TEST(ConsoleScriptTest, RunScriptFileMissingReturnsMinusOne) {
    xe::Console c;
    EXPECT_EQ(c.RunScriptFile("/nonexistent/foo.xescript"), -1);
}

}  // namespace
