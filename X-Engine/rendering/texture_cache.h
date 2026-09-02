#pragma once

#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include <cstdint>

namespace xe {

struct TextureData {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    UINT width = 0;
    UINT height = 0;
    bool valid = false;
    int slot = -1;  // index into the descriptor heap
};

class TextureCache {
public:
    TextureCache() = default;
    ~TextureCache();

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    void Initialize(ID3D12Device* device);
    void Shutdown();

    // Returns cached or newly-loaded texture. Returns empty if file missing.
    TextureData Load(const std::string& path);

    // Bind the descriptor heap before draw. Must be called once per frame.
    void SetActive(ID3D12GraphicsCommandList* cmd_list);

    // Heap pointer used to bind t0/t1/t2 per draw.
    [[nodiscard]] ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }

    [[nodiscard]] UINT GetDescriptorSize() const { return descriptor_size_; }

    [[nodiscard]] UINT GetLoadedCount() const { return next_slot_; }

    // Slot of the default 1x1 white fallback texture.
    [[nodiscard]] int GetDefaultSlot() const { return default_slot_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sampler_heap_;
    UINT descriptor_size_ = 0;
    UINT sampler_descriptor_size_ = 0;
    UINT next_slot_ = 0;
    int default_slot_ = -1;
    static constexpr UINT kMaxTextures = 64;

    struct CacheEntry {
        std::string path;
        TextureData data;
    };
    std::vector<CacheEntry> cache_;
};

}  // namespace xe
