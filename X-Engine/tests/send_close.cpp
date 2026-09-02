#include <windows.h>
#include <iostream>

int main() {
    HWND hwnd = FindWindowA("XEngineWindowClass", nullptr);
    if (hwnd == nullptr) {
        std::cout << "Window not found" << std::endl;
        return 1;
    }
    PostMessageA(hwnd, WM_CLOSE, 0, 0);
    std::cout << "WM_CLOSE sent" << std::endl;
    return 0;
}
