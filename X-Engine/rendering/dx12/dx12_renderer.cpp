#include "rendering/dx12/dx12_renderer.h"
#include "core/logger.h"
#include "core/math.h"
#include "rendering/meshes.h"

#include <vector>
#include <fstream>
#include <cstring>

namespace xe {

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

    texture_cache_.Initialize(device_.Get());

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

    CreateDepthBuffer();
    LoadAssets();

    XE_LOG_INFO("DX12 renderer initialized");
    return true;
}

void DX12Renderer::LoadAssets() {
    D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
    root_sig_desc.NumParameters = 0;
    root_sig_desc.NumStaticSamplers = 0;
    root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    D3D12_ROOT_PARAMETER root_params[3] = {};
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_params[0].Descriptor.ShaderRegister = 0;
    root_params[0].Descriptor.RegisterSpace = 0;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_params[1].Descriptor.ShaderRegister = 1;
    root_params[1].Descriptor.RegisterSpace = 0;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE srv_range = {};
    srv_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srv_range.NumDescriptors = 1;
    srv_range.BaseShaderRegister = 0;
    srv_range.RegisterSpace = 0;
    srv_range.OffsetInDescriptorsFromTableStart = 0;
    root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_params[2].DescriptorTable.NumDescriptorRanges = 1;
    root_params[2].DescriptorTable.pDescriptorRanges = &srv_range;
    root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_sig_desc.NumParameters = 3;
    root_sig_desc.pParameters = root_params;

    D3D12_STATIC_SAMPLER_DESC static_sampler = {};
    static_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    static_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    static_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    static_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    static_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    static_sampler.ShaderRegister = 0;
    static_sampler.RegisterSpace = 0;
    root_sig_desc.NumStaticSamplers = 1;
    root_sig_desc.pStaticSamplers = &static_sampler;

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
    pso_desc.DepthStencilState.DepthEnable = TRUE;
    pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso_desc.DepthStencilState.StencilEnable = FALSE;
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.DSVFormat = dsv_format_;
    pso_desc.SampleDesc.Count = 1;

    D3D12_INPUT_ELEMENT_DESC input_layout[4] = {};
    input_layout[0].SemanticName = "POSITION";
    input_layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    input_layout[0].InputSlot = 0;
    input_layout[0].AlignedByteOffset = 0;
    input_layout[1].SemanticName = "NORMAL";
    input_layout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    input_layout[1].InputSlot = 0;
    input_layout[1].AlignedByteOffset = 12;
    input_layout[2].SemanticName = "TEXCOORD0";
    input_layout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    input_layout[2].InputSlot = 0;
    input_layout[2].AlignedByteOffset = 24;
    input_layout[3].SemanticName = "COLOR";
    input_layout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    input_layout[3].InputSlot = 0;
    input_layout[3].AlignedByteOffset = 32;
    pso_desc.InputLayout = { input_layout, 4 };
    if (FAILED(device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create PSO");
        return;
    }

    auto vertices = xe::MakeCubeVertices();
    const UINT vb_size = static_cast<UINT>(vertices.size() * sizeof(Vertex));

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
        memcpy(mapped_data, vertices.data(), vertices.size() * sizeof(Vertex));
        vertex_buffer_->Unmap(0, nullptr);
    }

    vbv_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = vb_size;
    vbv_.StrideInBytes = sizeof(Vertex);

    auto indices = xe::MakeCubeIndices();
    index_count_ = static_cast<UINT>(indices.size());
    const UINT ib_size = static_cast<UINT>(indices.size() * sizeof(uint16_t));

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
        memcpy(mapped_index_data, indices.data(), indices.size() * sizeof(uint16_t));
        index_buffer_->Unmap(0, nullptr);
    }

    ibv_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = ib_size;
    ibv_.Format = DXGI_FORMAT_R16_UINT;

    constexpr UINT kCbvSize = (sizeof(Mat4) + 255u) & ~255u;
    D3D12_RESOURCE_DESC cb_desc = {};
    cb_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cb_desc.Width = kCbvSize;
    cb_desc.Height = 1;
    cb_desc.MipLevels = 1;
    cb_desc.DepthOrArraySize = 1;
    cb_desc.SampleDesc.Count = 1;
    cb_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    cb_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(device_->CreateCommittedResource(
            &heap_props, D3D12_HEAP_FLAG_NONE,
            &cb_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&constant_buffer_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create constant buffer");
        return;
    }

    if (FAILED(constant_buffer_->Map(0, nullptr, &constant_buffer_mapped_))) {
        XE_LOG_ERROR("DX12Renderer: failed to map constant buffer");
        return;
    }

    constexpr UINT kLightCbvSize = (4 * 16u + 255u) & ~255u;
    D3D12_RESOURCE_DESC lb_desc = {};
    lb_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    lb_desc.Width = kLightCbvSize;
    lb_desc.Height = 1;
    lb_desc.MipLevels = 1;
    lb_desc.DepthOrArraySize = 1;
    lb_desc.SampleDesc.Count = 1;
    lb_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    lb_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(device_->CreateCommittedResource(
            &heap_props, D3D12_HEAP_FLAG_NONE,
            &lb_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&light_buffer_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create light buffer");
        return;
    }

    if (FAILED(light_buffer_->Map(0, nullptr, &light_buffer_mapped_))) {
        XE_LOG_ERROR("DX12Renderer: failed to map light buffer");
        return;
    }
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

    depth_buffer_.Reset();
    dsv_heap_.Reset();
    CreateDepthBuffer();
}

void DX12Renderer::BeginFrame(float total_time) {
    WaitForPreviousFrame();

    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

    command_allocator_->Reset();
    command_list_->Reset(command_allocator_.Get(), pipeline_state_.Get());

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += frame_index_ * rtv_descriptor_size_;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = {};
    if (dsv_heap_) {
        dsv_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
    }
    command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE,
                                      dsv_heap_ ? &dsv_handle : nullptr);

    command_list_->ClearRenderTargetView(rtv_handle, clear_color_, 0, nullptr);
    if (dsv_heap_) {
        command_list_->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)width_, (float)height_, 0.0f, 1.0f };
    D3D12_RECT scissor_rect = { 0, 0, (LONG)width_, (LONG)height_ };
    command_list_->RSSetViewports(1, &viewport);
    command_list_->RSSetScissorRects(1, &scissor_rect);

    command_list_->SetGraphicsRootSignature(root_signature_.Get());
    command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list_->IASetVertexBuffers(0, 1, &vbv_);
    command_list_->IASetIndexBuffer(&ibv_);

    if (scene_ && constant_buffer_ && !per_object_world_.empty()) {
        Mat4 view  = scene_->GetViewMatrix();
        Mat4 proj  = scene_->GetProjectionMatrix(float(width_) / float(height_));

        // Update light CB
        if (light_buffer_mapped_) {
            struct LightCB {
                float direction[4];
                float color[4];
                float camera[4];
                float params[4];
            } lc{};
            float dl = 1.0f;
            if (light_) {
                lc.direction[0] = light_->direction.x;
                lc.direction[1] = light_->direction.y;
                lc.direction[2] = light_->direction.z;
                lc.direction[3] = light_->intensity;
                lc.color[0] = light_->color[0];
                lc.color[1] = light_->color[1];
                lc.color[2] = light_->color[2];
                lc.color[3] = light_->Ambient()[0];  // ambient strength in alpha
                dl = std::sqrt(light_->direction.x * light_->direction.x +
                               light_->direction.y * light_->direction.y +
                               light_->direction.z * light_->direction.z);
            } else {
                lc.direction[3] = 1.0f;
                lc.color[3] = 0.1f;
            }
            (void)dl;
            lc.camera[0] = camera_pos_[0];
            lc.camera[1] = camera_pos_[1];
            lc.camera[2] = camera_pos_[2];
            lc.camera[3] = 0.0f;
            lc.params[0] = 32.0f;  // shininess
            lc.params[1] = 0.6f;   // specular strength
            lc.params[2] = 1.0f;   // use specular
            lc.params[3] = 0.0f;
            std::memcpy(light_buffer_mapped_, &lc, sizeof(lc));
        }

        stat_draw_calls_ = 0;

        if (texture_cache_.GetHeap()) {
            texture_cache_.SetActive(command_list_.Get());
        }

        for (size_t i = 0; i < per_object_world_.size() && i < scene_->objects.size(); ++i) {
            const auto& obj = scene_->objects[i];
            const SubMesh* sm = nullptr;
            for (const auto& s : submeshes_) {
                if (s.kind == obj.instance.mesh) { sm = &s; break; }
            }
            if (!sm) continue;

            Mat4 wvp = Mat4::Multiply(proj, Mat4::Multiply(view, per_object_world_[i]));
            std::memcpy(constant_buffer_mapped_, wvp.m.data(), sizeof(wvp.m));
            command_list_->SetGraphicsRootConstantBufferView(0, constant_buffer_->GetGPUVirtualAddress());
            if (light_buffer_) {
                command_list_->SetGraphicsRootConstantBufferView(1, light_buffer_->GetGPUVirtualAddress());
            }

            if (texture_cache_.GetHeap()) {
                int slot = texture_cache_.GetDefaultSlot();
                if (slot < 0) slot = 0;
                if (!obj.instance.texture_path.empty()) {
                    auto td = texture_cache_.Load(obj.instance.texture_path);
                    if (td.valid && td.slot >= 0) {
                        slot = td.slot;
                    }
                }
                auto gpu = texture_cache_.GetHeap()->GetGPUDescriptorHandleForHeapStart();
                gpu.ptr += static_cast<UINT64>(slot) * texture_cache_.GetDescriptorSize();
                command_list_->SetGraphicsRootDescriptorTable(2, gpu);
            }

            command_list_->DrawIndexedInstanced(
                sm->index_count, 1, sm->index_offset, sm->vertex_offset, 0);
            stat_draw_calls_++;
        }
        return;
    }

    UpdateConstantBuffer(total_time);

    if (constant_buffer_) {
        command_list_->SetGraphicsRootConstantBufferView(0, constant_buffer_->GetGPUVirtualAddress());
    }

    command_list_->DrawIndexedInstanced(index_count_, 1, 0, 0, 0);
}

void DX12Renderer::CreateDepthBuffer() {
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.NumDescriptors = 1;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device_->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create DSV heap");
        return;
    }
    dsv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_RESOURCE_DESC depth_desc = {};
    depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_desc.Width = width_;
    depth_desc.Height = height_;
    depth_desc.DepthOrArraySize = 1;
    depth_desc.MipLevels = 1;
    depth_desc.Format = dsv_format_;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = dsv_format_;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device_->CreateCommittedResource(
            &heap_props, D3D12_HEAP_FLAG_NONE,
            &depth_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear_value, IID_PPV_ARGS(&depth_buffer_)))) {
        XE_LOG_ERROR("DX12Renderer: failed to create depth buffer");
        return;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = dsv_format_;
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
    device_->CreateDepthStencilView(depth_buffer_.Get(), &dsv_desc,
                                    dsv_heap_->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::UpdateConstantBuffer(float total_time) {
    if (!constant_buffer_mapped_) return;

    if (use_override_mvp_) {
        std::memcpy(constant_buffer_mapped_, override_mvp_, sizeof(override_mvp_));
        return;
    }

    Mat4 ry = Mat4::RotationY(total_time);
    Mat4 rx = Mat4::RotationX(total_time * 0.6f);
    Mat4 world = Mat4::Multiply(ry, rx);
    Mat4 view  = Mat4::LookAt(0.0f, 1.0f, -3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    Mat4 proj  = Mat4::Perspective(1.0472f, float(width_) / float(height_), 0.1f, 100.0f);
    Mat4 wvp   = Mat4::Multiply(proj, Mat4::Multiply(view, world));

    std::memcpy(constant_buffer_mapped_, wvp.m.data(), sizeof(wvp.m));
}

void DX12Renderer::SetClearColor(float r, float g, float b, float a) {
    clear_color_[0] = r;
    clear_color_[1] = g;
    clear_color_[2] = b;
    clear_color_[3] = a;
}

void DX12Renderer::SetViewProjection(const float mvp[16]) {
    use_override_mvp_ = true;
    for (int i = 0; i < 16; ++i) override_mvp_[i] = mvp[i];
}

void DX12Renderer::SetScene(const Scene* scene) {
    scene_ = scene;
    if (scene_) {
        RebuildMeshBuffers();
    }
}

void DX12Renderer::SetLight(const DirectionalLight* light) {
    light_ = light;
}

void DX12Renderer::SetCameraPosition(float x, float y, float z) {
    camera_pos_[0] = x;
    camera_pos_[1] = y;
    camera_pos_[2] = z;
}

void DX12Renderer::SetStats(uint32_t draw_calls, uint32_t vertices, uint32_t indices) {
    stat_draw_calls_ = draw_calls;
    stat_vertices_ = vertices;
    stat_indices_ = indices;
}

void DX12Renderer::RebuildMeshBuffers() {
    submeshes_.clear();
    per_object_world_.clear();
    if (!scene_) return;

    // Collect per-object world matrices in scene order
    per_object_world_.reserve(scene_->objects.size());
    for (const auto& obj : scene_->objects) {
        per_object_world_.push_back(obj.instance.GetWorldMatrix());
    }

    // Build a single big VB/IB with the base mesh for each kind.
    // Instances share the same base mesh data, so we only need to upload the
    // kind-specific vertex/index data once. We will DrawIndexedInstanced per
    // object using base vertex location to point into the appropriate section.
    struct KindBuf {
        std::vector<Vertex> verts;
        std::vector<uint16_t> idx;
    };
    KindBuf cube_buf   { MeshData::MakeCube().vertices,     MeshData::MakeCube().indices };
    KindBuf tri_buf    { MeshData::MakeTriangle().vertices, MeshData::MakeTriangle().indices };
    KindBuf quad_buf   { MeshData::MakeQuad().vertices,     MeshData::MakeQuad().indices };

    uint32_t voff[3] = { 0, 0, 0 };
    uint32_t ioff[3] = { 0, 0, 0 };

    std::vector<Vertex> all_v;
    std::vector<uint16_t> all_i;

    auto append = [&](MeshKind kind, KindBuf& b) {
        uint32_t vbase = static_cast<uint32_t>(all_v.size());
        all_v.insert(all_v.end(), b.verts.begin(), b.verts.end());
        std::vector<uint16_t> remapped;
        remapped.reserve(b.idx.size());
        for (auto ix : b.idx) remapped.push_back(static_cast<uint16_t>(vbase + ix));
        uint32_t ibase = static_cast<uint32_t>(all_i.size());
        all_i.insert(all_i.end(), remapped.begin(), remapped.end());
        SubMesh s{ kind, ibase, static_cast<uint32_t>(remapped.size()), vbase };
        submeshes_.push_back(s);
        (void)voff; (void)ioff;
    };

    append(MeshKind::Cube, cube_buf);
    append(MeshKind::Triangle, tri_buf);
    append(MeshKind::Quad, quad_buf);

    stat_vertices_ = static_cast<uint32_t>(all_v.size());
    stat_indices_  = static_cast<uint32_t>(all_i.size());

    // Recreate VB/IB
    WaitForPreviousFrame();
    vertex_buffer_.Reset();
    index_buffer_.Reset();

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

    auto upload_buffer = [&](ComPtr<ID3D12Resource>& buf, const void* data, size_t bytes, D3D12_RESOURCE_STATES s) {
        D3D12_RESOURCE_DESC r = {};
        r.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        r.Width = (bytes + 255) & ~size_t(255);
        r.Height = 1;
        r.MipLevels = 1;
        r.DepthOrArraySize = 1;
        r.SampleDesc.Count = 1;
        r.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        r.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (FAILED(device_->CreateCommittedResource(
                &heap_props, D3D12_HEAP_FLAG_NONE, &r, s, nullptr, IID_PPV_ARGS(&buf)))) {
            XE_LOG_ERROR("DX12Renderer: buffer recreate failed");
        }
    };

    upload_buffer(vertex_buffer_, all_v.data(), all_v.size() * sizeof(Vertex), D3D12_RESOURCE_STATE_GENERIC_READ);
    upload_buffer(index_buffer_,  all_i.data(), all_i.size() * sizeof(uint16_t), D3D12_RESOURCE_STATE_GENERIC_READ);

    if (vertex_buffer_) {
        D3D12_RANGE r{0,0};
        void* m = nullptr;
        if (SUCCEEDED(vertex_buffer_->Map(0, &r, &m))) {
            std::memcpy(m, all_v.data(), all_v.size() * sizeof(Vertex));
            vertex_buffer_->Unmap(0, nullptr);
        }
        vbv_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
        vbv_.SizeInBytes = static_cast<UINT>(all_v.size() * sizeof(Vertex));
        vbv_.StrideInBytes = sizeof(Vertex);
    }
    if (index_buffer_) {
        D3D12_RANGE r{0,0};
        void* m = nullptr;
        if (SUCCEEDED(index_buffer_->Map(0, &r, &m))) {
            std::memcpy(m, all_i.data(), all_i.size() * sizeof(uint16_t));
            index_buffer_->Unmap(0, nullptr);
        }
        ibv_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
        ibv_.SizeInBytes = static_cast<UINT>(all_i.size() * sizeof(uint16_t));
        ibv_.Format = DXGI_FORMAT_R16_UINT;
    }
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
