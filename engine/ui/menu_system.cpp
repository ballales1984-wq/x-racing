#include "ui/menu_system.h"
#include <algorithm>

namespace p0::ui {

//! @brief Default constructor.
MenuSystem::MenuSystem() = default;

//! @brief Sets the current menu state and resets the screen.
//! @param state The new menu state.
void MenuSystem::set_state(MenuState state) {
  current_state_ = state;
  current_screen_.items.clear();
  current_screen_.selected_index = 0;
}

//! @brief Moves selection up by one item, wrapping to bottom if at top.
void MenuSystem::navigate_up() {
  if (current_screen_.items.empty()) return;
  current_screen_.selected_index--;
  if (current_screen_.selected_index < 0) {
    current_screen_.selected_index = static_cast<int>(current_screen_.items.size()) - 1;
  }
}

//! @brief Moves selection down by one item, wrapping to top if at bottom.
void MenuSystem::navigate_down() {
  if (current_screen_.items.empty()) return;
  current_screen_.selected_index++;
  if (current_screen_.selected_index >= static_cast<int>(current_screen_.items.size())) {
    current_screen_.selected_index = 0;
  }
}

//! @brief Activates the currently selected menu item.
//!        Calls the item's action callback if enabled.
void MenuSystem::select() {
  if (current_screen_.items.empty()) return;
  if (current_screen_.selected_index >= 0 &&
      current_screen_.selected_index < static_cast<int>(current_screen_.items.size())) {
    const auto& item = current_screen_.items[current_screen_.selected_index];
    if (item.enabled && item.action) {
      item.action();
    }
  }
}

//! @brief Navigates back to the previous menu state.
//!        Uses the navigation stack, or falls back to MAIN_MENU.
void MenuSystem::go_back() {
  if (!navigation_stack_.empty()) {
    current_state_ = navigation_stack_.back();
    navigation_stack_.pop_back();
    current_screen_.items.clear();
    current_screen_.selected_index = 0;
  } else {
    current_state_ = MenuState::MAIN_MENU;
    current_screen_.items.clear();
    current_screen_.selected_index = 0;
  }
}

//! @brief Adds a menu item to the current screen.
//! @param item The menu item to add.
void MenuSystem::add_item(const MenuItem& item) {
  current_screen_.items.push_back(item);
}

//! @brief Removes all items from the current screen and resets selection.
void MenuSystem::clear_items() {
  current_screen_.items.clear();
  current_screen_.selected_index = 0;
}

//! @brief Sets the selected item index with bounds checking.
//! @param index The new selected index.
void MenuSystem::set_selected_index(int index) {
  if (index >= 0 && index < static_cast<int>(current_screen_.items.size())) {
    current_screen_.selected_index = index;
  }
}

//! @brief Shows a new menu state, pushing the current state onto the navigation stack.
//! @param state The menu state to show.
void MenuSystem::show(MenuState state) {
  if (current_state_ != MenuState::NONE) {
    navigation_stack_.push_back(current_state_);
  }
  set_state(state);
}

//! @brief Hides the current menu, restoring the previous state from the stack.
void MenuSystem::hide() {
  if (!navigation_stack_.empty()) {
    current_state_ = navigation_stack_.back();
    navigation_stack_.pop_back();
  } else {
    current_state_ = MenuState::NONE;
  }
  current_screen_.items.clear();
  current_screen_.selected_index = 0;
}

}