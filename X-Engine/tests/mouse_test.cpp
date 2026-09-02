#include <gtest/gtest.h>
#include "platform/mouse.h"

namespace {

class FakeMouse : public xe::Mouse {
public:
    void Update() override { updates_++; }
    int updates_ = 0;
};

}  // namespace

TEST(MouseTest, DefaultStateIsClean) {
    FakeMouse m;
    m.Update();
    EXPECT_FALSE(m.GetState().IsDown(xe::MouseButton::Left));
    EXPECT_FALSE(m.GetState().WasPressed(xe::MouseButton::Left));
}

TEST(MouseTest, UpdateIncrementsCounter) {
    FakeMouse m;
    m.Update();
    m.Update();
    EXPECT_EQ(m.updates_, 2);
}

TEST(MouseTest, StateStartsZero) {
    FakeMouse m;
    EXPECT_EQ(m.GetState().x, 0);
    EXPECT_EQ(m.GetState().y, 0);
    EXPECT_EQ(m.GetState().dx, 0);
    EXPECT_EQ(m.GetState().dy, 0);
    EXPECT_EQ(m.GetState().wheel, 0);
}