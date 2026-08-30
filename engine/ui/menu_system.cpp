#include "ui/menu_system.h"
#include <algorithm>

namespace p0::ui {

MenuSystem::MenuSystem() = default;

void MenuSystem::set_state(MenuState state) {
  current_state_ = state;
  current_screen_.items.clear();
  current_screen_.selected_index = 0;
}

void MenuSystem::navigate_up() {
  if (current_screen_.items.empty()) return;
  current_screen_.selected_index--;
  if (current_screen_.selected_index < 0) {
    current_screen_.selected_index = static_cast<int>(current_screen_.items.size()) - 1;
  }
}

void MenuSystem::navigate_down() {
  if (current_screen_.items.empty()) return;
  current_screen_.selected_index++;
  if (current_screen_.selected_index >= static_cast<int>(current_screen_.items.size())) {
    current_screen_.selected_index = 0;
  }
}

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

void MenuSystem::add_item(const MenuItem& item) {
  current_screen_.items.push_back(item);
}

void MenuSystem::clear_items() {
  current_screen_.items.clear();
  current_screen_.selected_index = 0;
}

void MenuSystem::set_selected_index(int index) {
  if (index >= 0 && index < static_cast<int>(current_screen_.items.size())) {
    current_screen_.selected_index = index;
  }
}

void MenuSystem::show(MenuState state) {
  if (current_state_ != MenuState::NONE) {
    navigation_stack_.push_back(current_state_);
  }
  set_state(state);
}

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
