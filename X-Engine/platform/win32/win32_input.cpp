#include "platform/win32/win32_input.h"

namespace xe {

int Win32Input::ToNativeKey(Key key) {
    switch (key) {
        case Key::Escape:     return VK_ESCAPE;
        case Key::W:          return 'W';
        case Key::A:          return 'A';
        case Key::S:          return 'S';
        case Key::D:          return 'D';
        case Key::Space:      return VK_SPACE;
        case Key::Enter:      return VK_RETURN;
        case Key::Tab:        return VK_TAB;
        case Key::Left:       return VK_LEFT;
        case Key::Right:      return VK_RIGHT;
        case Key::Up:         return VK_UP;
        case Key::Down:       return VK_DOWN;
        case Key::F1:         return VK_F1;
        case Key::F2:         return VK_F2;
        case Key::F3:         return VK_F3;
        case Key::F4:         return VK_F4;
        case Key::LeftShift:  return VK_LSHIFT;
        case Key::RightShift: return VK_RSHIFT;
        case Key::LeftCtrl:   return VK_LCONTROL;
        case Key::RightCtrl:  return VK_RCONTROL;
        default:              return 0;
    }
}

void Win32Input::Update() {
    previous_keys_ = current_keys_;
    GetKeyboardState(current_keys_.data());
}

bool Win32Input::IsKeyDown(Key key) const {
    int vk = ToNativeKey(key);
    if (vk < 0 || vk >= 256) return false;
    return (current_keys_[vk] & 0x80) != 0;
}

bool Win32Input::IsKeyPressed(Key key) const {
    int vk = ToNativeKey(key);
    if (vk < 0 || vk >= 256) return false;
    return (current_keys_[vk] & 0x80) != 0 && (previous_keys_[vk] & 0x80) == 0;
}

}  // namespace xe
