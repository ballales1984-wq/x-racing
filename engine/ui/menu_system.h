// Project 0 — Console menu system (navigable menu stack)
// Namespace: p0::ui
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace p0::ui {

// Top-level menu states in the game flow.
enum class MenuState : uint8_t {
  MAIN_MENU,
  RACE_SETUP,
  SETTINGS,
  PAUSED,
  RESULTS,
  NONE
};

// Single actionable item inside a menu screen. Supports static actions
// and slider-style numeric inputs.
struct MenuItem {
  std::string label;  // displayed text
  std::function<void()> action;  // invoked on select
  bool enabled = true;
  int value = 0;  // current value (for sliders)
  int min_value = 0;  // slider minimum
  int max_value = 100;  // slider maximum
  bool is_slider = false;
};

struct MenuScreen {
  std::string title;
  std::vector<MenuItem> items;
  int selected_index = 0;
};

class MenuSystem {
 public:
  MenuSystem();

  void set_state(MenuState state);
  MenuState state() const { return current_state_; }

  void navigate_up();
  void navigate_down();
  void select();
  void go_back();

  void add_item(const MenuItem& item);
  void clear_items();

  const MenuScreen& current_screen() const { return current_screen_; }
  MenuScreen& current_screen() { return current_screen_; }

  int selected_index() const { return current_screen_.selected_index; }
  void set_selected_index(int index);

  bool is_active() const { return current_state_ != MenuState::NONE; }
  void show(MenuState state);
  void hide();

 private:
  MenuState current_state_ = MenuState::NONE;
  MenuScreen current_screen_;
  std::vector<MenuState> navigation_stack_;
};

}
