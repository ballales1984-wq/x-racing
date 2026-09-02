#pragma once

#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>
#include "rendering/renderer.h"
#include "rendering/scene.h"
#include "rendering/light.h"
#include "rendering/texture_cache.h"

namespace xe {

using Microsoft::WRL::ComPtr;

class DX12Renderer : public Renderer {
public:
    DX12Renderer() = default;
    ~DX12Renderer() override = default;

    DX12Renderer(const DX12Renderer&) = delete;
    DX12Renderer& operator=(const DX12Renderer&) = delete;

    bool Initialize(uintptr_t window_handle) override;
    void BeginFrame(float total_time = 0.0f) override;
    void EndFrame() override;
    void Resize(uint32_t width, uint32_t height) override;

    void SetClearColor(float r, float g, float b, float a = 1.0f);
    void SetViewProjection(const float mvp[16]);
    void SetScene(const Scene* scene);
    void SetLight(const DirectionalLight* light);
    void SetCameraPosition(float x, float y, float z);

    void SetStats(uint32_t draw_calls, uint32_t vertices, uint32_t indices);

    TextureCache* GetTextureCache() { return &texture_cache_; }

private:
    static constexpr UINT kFrameCount = 2;

    void WaitForPreviousFrame();
    void LoadAssets();
    void LoadSizeDependentResources();
    void UpdateConstantBuffer(float total_time);
    void CreateDepthBuffer();
    void RebuildMeshBuffers();

    HWND hwnd_ = nullptr;
    UINT width_ = 1280;
    UINT height_ = 720;

    ComPtr<IDXGIFactory4> factory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> command_queue_;
    ComPtr<IDXGISwapChain3> swap_chain_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> command_list_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    ComPtr<ID3D12Resource> render_targets_[kFrameCount];

    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
    ComPtr<ID3D12Resource> vertex_buffer_;
    ComPtr<ID3D12Resource> index_buffer_;
ComPtr<ID3D12Resource> constant_buffer_;
    ComPtr<ID3D12Resource> light_buffer_;
    ComPtr<ID3D12Resource> depth_buffer_;
    ComPtr<ID3D12DescriptorHeap> dsv_heap_;
    void* constant_buffer_mapped_ = nullptr;
    void* light_buffer_mapped_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbv_ = {};
    D3D12_INDEX_BUFFER_VIEW ibv_ = {};
    DXGI_FORMAT dsv_format_ = DXGI_FORMAT_D32_FLOAT;
    UINT dsv_descriptor_size_ = 0;
    UINT index_count_ = 3;

    float clear_color_[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
    float override_mvp_[16] = {};
    bool use_override_mvp_ = false;

    const Scene* scene_ = nullptr;
    const DirectionalLight* light_ = nullptr;
    float camera_pos_[3] = { 0.0f, 1.5f, -4.0f };
    std::vector<Mat4> per_object_world_;
    TextureCache texture_cache_;
    uint32_t stat_draw_calls_ = 0;
    uint32_t stat_vertices_ = 0;
    uint32_t stat_indices_ = 0;

    struct SubMesh {
        MeshKind kind;
        uint32_t index_offset;
        uint32_t index_count;
        uint32_t vertex_offset;
    };
    std::vector<SubMesh> submeshes_;

    UINT rtv_descriptor_size_ = 0;
    UINT frame_index_ = 0;
    UINT64 fence_value_ = 0;
};

}  // namespace xe
