#include "rendering/dx12/dx12_renderer.h"
#include "core/logger.h"

#include <vector>
#include <fstream>
#include <cstring>

namespace xe {

struct Vertex {
    float position[3];
    float color[4];
};

static std::vector<uint8_t> LoadShaderBytecode(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        XE_LOG_ERROR("DX12Renderer: failed to load shader file");
        return {};
    }
    size_t size = file.tellg();
    std::vector<uint8_t> bytecode(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytecode.data()), size);
    return bytecode;
}

bool DX12Renderer::Initialize(uintptr_t window_handle) {
    hwnd_ = reinterpret_cast<HWND>(window_handle);
    if (!hwnd_) {
        XE_LOG_ERROR("DX12Renderer: invalid window handle");
        return false;
    }

    RECT rect;
    GetClientRect(hwnd_, &rect);
    width_ = static_cast<UINT>(rect.right - rect.left);
    height_ = static_cast<UINT>(rect.bottom - rect.top);

    LoadSizeDependentResources();

#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
            debug_controller->EnableDebugLayer();
        }
    }
#endif

    UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
    dxgi_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create DXGI factory");
        return false;
    }

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create D3D12 device");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create command queue");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.BufferCount = kFrameCount;
    swap_chain_desc.Width = width_;
    swap_chain_desc.Height = height_;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swap_chain1;
    if (FAILED(factory_->CreateSwapChainForHwnd(
            command_queue_.Get(), hwnd_, &swap_chain_desc, nullptr, nullptr, &swap_chain1))) {
        XE_LOG_ERROR("DX12Renderer: failed to create swap chain");
        return false;
    }

    if (FAILED(swap_chain1.As(&swap_chain_))) {
        XE_LOG_ERROR("DX12Renderer: failed to query IDXGISwapChain3");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = kFrameCount;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create RTV heap");
        return false;
    }
    rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kFrameCount; i++) {
        if (FAILED(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i])))) {
            XE_LOG_ERROR("DX12Renderer: failed to get back buffer");
            return false;
        }
        device_->CreateRenderTargetView(render_targets_[i].Get(), nullptr, rtv_handle);
        rtv_handle.ptr += rtv_descriptor_size_;
    }

    if (FAILED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create command allocator");
        return false;
    }

    if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create command list");
        return false;
    }
    command_list_->Close();

    if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create fence");
        return false;
    }

    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

    LoadAssets();

    XE_LOG_INFO("DX12 renderer initialized");
    return true;
}

void DX12Renderer::LoadAssets() {
    D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
    root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> serialized_root_sig;
    ComPtr<ID3DBlob> error_blob;
    if (FAILED(D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1,
            serialized_root_sig.ReleaseAndGetAddressOf(),
            error_blob.ReleaseAndGetAddressOf()))) {
        XE_LOG_ERROR("DX12Renderer: failed to serialize root signature");
        return;
    }

    if (FAILED(device_->CreateRootSignature(0,
            serialized_root_sig->GetBufferPointer(),
            serialized_root_sig->GetBufferSize(),
            IID_PPV_ARGS(&root_signature_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create root signature");
        return;
    }

    std::vector<uint8_t> vs_bytecode = LoadShaderBytecode("triangle_vs.cso");
    std::vector<uint8_t> ps_bytecode = LoadShaderBytecode("triangle_ps.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.VS = { vs_bytecode.data(), vs_bytecode.size() };
    pso_desc.PS = { ps_bytecode.data(), ps_bytecode.size() };
    pso_desc.pRootSignature = root_signature_.Get();
    pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
    pso_desc.BlendState.IndependentBlendEnable = FALSE;
    pso_desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    pso_desc.DepthStencilState.DepthEnable = FALSE;
    pso_desc.DepthStencilState.StencilEnable = FALSE;
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count = 1;
    if (FAILED(device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create PSO");
        return;
    }

    Vertex vertices[] = {
        {  0.0f,  0.25f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
        {  0.25f, -0.25f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
        { -0.25f, -0.25f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
    };

    const UINT vb_size = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC res_desc = {};
    res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    res_desc.Width = vb_size;
    res_desc.Height = 1;
    res_desc.MipLevels = 1;
    res_desc.DepthOrArraySize = 1;
    res_desc.SampleDesc.Count = 1;
    res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(device_->CreateCommittedResource(
            &heap_props, D3D12_HEAP_FLAG_NONE,
            &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&vertex_buffer_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create vertex buffer");
        return;
    }

    D3D12_RANGE read_piece = { 0, 0 };
    void* mapped_data = nullptr;
    if (SUCCEEDED(vertex_buffer_->Map(0, &read_piece, &mapped_data))) {
        memcpy(mapped_data, vertices, vb_size);
        vertex_buffer_->Unmap(0, nullptr);
    }

    vbv_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = vb_size;
    vbv_.StrideInBytes = sizeof(Vertex);

    uint16_t indices[] = { 0, 1, 2 };
    const UINT ib_size = sizeof(indices);

    res_desc.Width = ib_size;

    if (FAILED(device_->CreateCommittedResource(
            &heap_props, D3D12_HEAP_FLAG_NONE,
            &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&index_buffer_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create index buffer");
        return;
    }

    D3D12_RANGE read_piece_idx = { 0, 0 };
    void* mapped_index_data = nullptr;
    if (SUCCEEDED(index_buffer_->Map(0, &read_piece_idx, &mapped_index_data))) {
        memcpy(mapped_index_data, indices, ib_size);
        index_buffer_->Unmap(0, nullptr);
    }

    ibv_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = ib_size;
    ibv_.Format = DXGI_FORMAT_R16_UINT;
}

void DX12Renderer::LoadSizeDependentResources() {
}

void DX12Renderer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    WaitForPreviousFrame();

    for (UINT i = 0; i < kFrameCount; i++) {
        render_targets_[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    swap_chain_->GetDesc1(&desc);
    swap_chain_->ResizeBuffers(kFrameCount, width, height, desc.Format, desc.Flags);

    width_ = width;
    height_ = height;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kFrameCount; i++) {
        swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i]));
        device_->CreateRenderTargetView(render_targets_[i].Get(), nullptr, rtv_handle);
        rtv_handle.ptr += rtv_descriptor_size_;
    }
}

void DX12Renderer::BeginFrame() {
    WaitForPreviousFrame();

    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

    command_allocator_->Reset();
    command_list_->Reset(command_allocator_.Get(), pipeline_state_.Get());

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += frame_index_ * rtv_descriptor_size_;
    command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    const float clear_color[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
    command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)width_, (float)height_, 0.0f, 1.0f };
    D3D12_RECT scissor_rect = { 0, 0, (LONG)width_, (LONG)height_ };
    command_list_->RSSetViewports(1, &viewport);
    command_list_->RSSetScissorRects(1, &scissor_rect);

    command_list_->SetGraphicsRootSignature(root_signature_.Get());
    command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list_->IASetVertexBuffers(0, 1, &vbv_);
    command_list_->IASetIndexBuffer(&ibv_);
    command_list_->DrawIndexedInstanced(3, 1, 0, 0, 0);
}

void DX12Renderer::EndFrame() {
    command_list_->Close();

    ID3D12CommandList* pp_command_lists[] = { command_list_.Get() };
    command_queue_->ExecuteCommandLists(1, pp_command_lists);

    swap_chain_->Present(1, 0);

    command_queue_->Signal(fence_.Get(), fence_value_);
    fence_value_++;
}

void DX12Renderer::WaitForPreviousFrame() {
    if (fence_->GetCompletedValue() < fence_value_) {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (event) {
            fence_->SetEventOnCompletion(fence_value_, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }
    }
}

}  // namespace xe
