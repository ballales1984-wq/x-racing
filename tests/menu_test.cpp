#include <gtest/gtest.h>
#include "ui/menu_system.h"

using namespace p0::ui;

TEST(MenuSystem, DefaultState) {
  MenuSystem menu;
  EXPECT_EQ(menu.state(), MenuState::NONE);
  EXPECT_FALSE(menu.is_active());
}

TEST(MenuSystem, SetState) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);
  EXPECT_EQ(menu.state(), MenuState::MAIN_MENU);
  EXPECT_TRUE(menu.is_active());
}

TEST(MenuSystem, NavigateDown) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  MenuItem item1{"Start", nullptr};
  MenuItem item2{"Settings", nullptr};
  MenuItem item3{"Quit", nullptr};

  menu.add_item(item1);
  menu.add_item(item2);
  menu.add_item(item3);

  EXPECT_EQ(menu.selected_index(), 0);
  menu.navigate_down();
  EXPECT_EQ(menu.selected_index(), 1);
  menu.navigate_down();
  EXPECT_EQ(menu.selected_index(), 2);
  menu.navigate_down();
  EXPECT_EQ(menu.selected_index(), 0);
}

TEST(MenuSystem, NavigateUp) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  MenuItem item1{"Start", nullptr};
  MenuItem item2{"Settings", nullptr};

  menu.add_item(item1);
  menu.add_item(item2);

  EXPECT_EQ(menu.selected_index(), 0);
  menu.navigate_up();
  EXPECT_EQ(menu.selected_index(), 1);
  menu.navigate_up();
  EXPECT_EQ(menu.selected_index(), 0);
}

TEST(MenuSystem, Select) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  bool clicked = false;
  MenuItem item1{"Start", [&clicked]() { clicked = true; }};
  menu.add_item(item1);

  menu.select();
  EXPECT_TRUE(clicked);
}

TEST(MenuSystem, SelectDisabled) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  bool clicked = false;
  MenuItem item1{"Start", [&clicked]() { clicked = true; }, false};
  menu.add_item(item1);

  menu.select();
  EXPECT_FALSE(clicked);
}

TEST(MenuSystem, GoBack) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);
  menu.show(MenuState::SETTINGS);
  EXPECT_EQ(menu.state(), MenuState::SETTINGS);

  menu.go_back();
  EXPECT_EQ(menu.state(), MenuState::MAIN_MENU);
}

TEST(MenuSystem, ShowHide) {
  MenuSystem menu;
  menu.show(MenuState::PAUSED);
  EXPECT_TRUE(menu.is_active());
  EXPECT_EQ(menu.state(), MenuState::PAUSED);

  menu.hide();
  EXPECT_FALSE(menu.is_active());
}

TEST(MenuSystem, ClearItems) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  MenuItem item1{"Start", nullptr};
  MenuItem item2{"Settings", nullptr};
  menu.add_item(item1);
  menu.add_item(item2);

  menu.clear_items();
  EXPECT_EQ(menu.current_screen().items.size(), 0u);
  EXPECT_EQ(menu.selected_index(), 0);
}

TEST(MenuSystem, SetSelectedIndex) {
  MenuSystem menu;
  menu.set_state(MenuState::MAIN_MENU);

  MenuItem item1{"Start", nullptr};
  MenuItem item2{"Settings", nullptr};
  MenuItem item3{"Quit", nullptr};
  menu.add_item(item1);
  menu.add_item(item2);
  menu.add_item(item3);

  menu.set_selected_index(2);
  EXPECT_EQ(menu.selected_index(), 2);

  menu.set_selected_index(10);
  EXPECT_EQ(menu.selected_index(), 2);
}

TEST(MenuSystem, MenuItemSlider) {
  MenuItem item{"Volume", nullptr, true, 50, 0, 100, true};
  EXPECT_TRUE(item.is_slider);
  EXPECT_EQ(item.value, 50);
  EXPECT_EQ(item.min_value, 0);
  EXPECT_EQ(item.max_value, 100);
}
