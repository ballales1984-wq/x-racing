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

// A single full-screen menu page with a title and selectable items.
struct MenuScreen {
  std::string title;  // header text rendered at the top of the screen
  std::vector<MenuItem> items;
  int selected_index = 0;  // currently highlighted item
};

// Manages a stack of menu screens with keyboard-style navigation.
// Supports push/pop navigation, slider inputs, and per-screen item management.
class MenuSystem {
 public:
  MenuSystem();

  // Switch to a new top-level menu state (clears current screen).
  void set_state(MenuState state);
  MenuState state() const { return current_state_; }

  // Navigate the selection highlight.
  void navigate_up();
  void navigate_down();
  // Activate the currently selected item.
  void select();
  // Return to the previous menu in the navigation stack.
  void go_back();

  // Add an item to the current screen.
  void add_item(const MenuItem& item);
  void clear_items();

  // Access the currently active screen (const and mutable).
  const MenuScreen& current_screen() const { return current_screen_; }
  MenuScreen& current_screen() { return current_screen_; }

  int selected_index() const { return current_screen_.selected_index; }
  void set_selected_index(int index);  // clamp to [0, items.size())

  // True when any menu is currently visible.
  bool is_active() const { return current_state_ != MenuState::NONE; }
  // Show a specific menu state (pushes current state onto the stack).
  void show(MenuState state);
  // Hide the current menu and return to NONE.
  void hide();

  private:
   MenuState current_state_ = MenuState::NONE;
   MenuScreen current_screen_;
   // Stack of previous states for back-navigation.
   std::vector<MenuState> navigation_stack_;
};

}
