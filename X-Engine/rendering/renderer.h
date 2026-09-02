#pragma once

#include <cstdint>

namespace xe {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool Initialize(uintptr_t window_handle) = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};

}  // namespace xe
