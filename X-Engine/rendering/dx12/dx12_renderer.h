#pragma once

#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include "rendering/renderer.h"

namespace xe {

using Microsoft::WRL::ComPtr;

class DX12Renderer : public Renderer {
public:
    DX12Renderer() = default;
    ~DX12Renderer() override = default;

    DX12Renderer(const DX12Renderer&) = delete;
    DX12Renderer& operator=(const DX12Renderer&) = delete;

    bool Initialize(uintptr_t window_handle) override;
    void BeginFrame() override;
    void EndFrame() override;
    void Resize(uint32_t width, uint32_t height) override;

private:
    static constexpr UINT kFrameCount = 2;

    void WaitForPreviousFrame();
    void PopulateCommandList();
    void LoadAssets();
    void LoadSizeDependentResources();

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
    D3D12_VERTEX_BUFFER_VIEW vbv_ = {};
    D3D12_INDEX_BUFFER_VIEW ibv_ = {};

    UINT rtv_descriptor_size_ = 0;
    UINT frame_index_ = 0;
    UINT64 fence_value_ = 0;
};

}  // namespace xe
