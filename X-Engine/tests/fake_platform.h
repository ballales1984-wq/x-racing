#pragma once

#include "platform/window.h"
#include "platform/input.h"
#include <cstring>
#include <thread>
#include <chrono>

namespace xe::test {

class FakeWindow : public xe::Window {
public:
    int poll_count = 0;
    int create_count = 0;
    bool create_called = false;

    int auto_close_after = 0;

    bool Create(const std::string& title, int width, int height) override {
        create_called = true;
        create_count++;
        last_title = title;
        last_width = width;
        last_height = height;
        return true;
    }

    void PollEvents() override {
        poll_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (auto_close_after > 0 && poll_count >= auto_close_after) {
            should_close_ = true;
        }
    }

    bool ShouldClose() const override { return should_close_; }
    void GetSize(int& width, int& height) const override {
        width = last_width;
        height = last_height;
    }

    uintptr_t GetNativeHandle() const override { return 0; }
    void* GetNativeDC() const override { return nullptr; }

    void SetResizeCallback(ResizeCallback callback) override {
        resize_callback_ = std::move(callback);
    }

    void SimulateResize(int w, int h) {
        if (resize_callback_) {
            resize_callback_(w, h);
        }
    }

    std::string last_title;
    int last_width = 0;
    int last_height = 0;

private:
    ResizeCallback resize_callback_;
    bool should_close_ = false;
};

class FakeInput : public xe::Input {
public:
    void Update() override {
        std::memcpy(previous_keys_, current_keys_, sizeof(current_keys_));
        update_count++;
        if (press_on_frame > 0 && update_count >= press_on_frame) {
            current_keys_[static_cast<int>(key_to_press)] = true;
        }
    }

    bool IsKeyDown(xe::Key key) const override {
        return current_keys_[static_cast<int>(key)];
    }

    bool IsKeyPressed(xe::Key key) const override {
        int idx = static_cast<int>(key);
        if (update_count == 0) return false;
        return current_keys_[idx] && !previous_keys_[idx];
    }

    void SetKey(xe::Key key, bool down) {
        current_keys_[static_cast<int>(key)] = down;
    }

    void SimulateKeyPressOnFrame(xe::Key key, int frame) {
        key_to_press = key;
        press_on_frame = frame;
    }

    int update_count = 0;
    bool current_keys_[256] = {};
    bool previous_keys_[256] = {};
    xe::Key key_to_press = xe::Key::Unknown;
    int press_on_frame = 0;
};

}  // namespace xe::test
