#include "rendering/texture_cache.h"
#include "core/logger.h"

#include <wincodec.h>
#include <vector>
#include <cstring>

#pragma comment(lib, "windowscodecs.lib")

namespace xe {

namespace {

struct WICGlobalInit {
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    WICGlobalInit() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IWICImagingFactory, &factory);
    }
    ~WICGlobalInit() { CoUninitialize(); }
};

WICGlobalInit& GetWIC() {
    static WICGlobalInit g;
    return g;
}

TextureData LoadWIC(ID3D12Device* device, IWICImagingFactory* factory, const std::wstring& path) {
    TextureData out;
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &decoder))) {
        return out;
    }
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) return out;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    factory->CreateFormatConverter(&converter);
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut))) {
        return out;
    }

    UINT w = 0, h = 0;
    converter->GetSize(&w, &h);
    if (w == 0 || h == 0) return out;

    std::vector<uint8_t> pixels(w * h * 4);
    converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(pixels.size()), pixels.data());

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = w;
    desc.Height = h;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&out.resource)))) {
        return out;
    }

    UINT row_size = w * 4;
    UINT num_rows = h;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    layout.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    layout.Footprint.Width = w;
    layout.Footprint.Height = h;
    layout.Footprint.Depth = 1;
    layout.Footprint.RowPitch = (row_size + 255) & ~UINT(255);

    UINT64 upload_size = layout.Footprint.RowPitch * num_rows;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = upload_size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12Resource> upload;
    hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)))) {
        return out;
    }

    void* mapped = nullptr;
    D3D12_RANGE no_read = {0, 0};
    if (FAILED(upload->Map(0, &no_read, &mapped))) return out;

    auto* dst = static_cast<uint8_t*>(mapped);
    for (UINT y = 0; y < num_rows; ++y) {
        std::memcpy(dst + y * layout.Footprint.RowPitch,
                    pixels.data() + y * row_size, row_size);
    }
    upload->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = upload.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = layout;

    D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
    dst_loc.pResource = out.resource.Get();
    dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_loc.SubresourceIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;  // we need device for direct copy
    (void)queue;

    // For simplicity, do the upload via a transient command list.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd;
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)))) return out;
    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(),
            nullptr, IID_PPV_ARGS(&cmd)))) return out;
    cmd->CopyTextureRegion(&dst_loc, 0, 0, 0, &src, nullptr);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = out.resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);
    cmd->Close();

    ID3D12CommandList* lists[] = { cmd.Get() };
    // Caller will ExecuteCommandLists — but we don't have a queue here.
    // Instead, defer the barrier with a transient copy queue.
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyq;
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&copyq)))) return out;
    copyq->ExecuteCommandLists(1, lists);

    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) return out;
    HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    copyq->Signal(fence.Get(), 1);
    fence->SetEventOnCompletion(1, evt);
    WaitForSingleObject(evt, INFINITE);
    CloseHandle(evt);

    out.width = w;
    out.height = h;
    out.valid = true;
    return out;
}

}  // namespace

TextureCache::~TextureCache() { Shutdown(); }

void TextureCache::Initialize(ID3D12Device* device) {
    device_ = device;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};
    d.NumDescriptors = kMaxTextures;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device_->CreateDescriptorHeap(&d, IID_PPV_ARGS(&heap_)))) {
        XE_LOG_ERROR("TextureCache: failed to create SRV heap");
        return;
    }
    descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC sd = {};
    sd.NumDescriptors = 1;
    sd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    sd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device_->CreateDescriptorHeap(&sd, IID_PPV_ARGS(&sampler_heap_)))) {
        XE_LOG_ERROR("TextureCache: failed to create sampler heap");
        return;
    }
    sampler_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    D3D12_SAMPLER_DESC s = {};
    s.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    s.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    s.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    s.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    s.MaxLOD = D3D12_FLOAT32_MAX;
    device_->CreateSampler(&s, sampler_heap_->GetCPUDescriptorHandleForHeapStart());

    // Allocate a 1x1 white fallback texture at slot 0.
    uint32_t white = 0xFFFFFFFF;
    D3D12_RESOURCE_DESC td = {};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = 1; td.Height = 1;
    td.DepthOrArraySize = 1; td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;

    Microsoft::WRL::ComPtr<ID3D12Resource> white_tex;
    if (SUCCEEDED(device_->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&white_tex)))) {
        D3D12_SUBRESOURCE_DATA srd = {};
        srd.pData = reinterpret_cast<const BYTE*>(&white);
        srd.RowPitch = 4;
        srd.SlicePitch = 4;

        Microsoft::WRL::ComPtr<ID3D12Resource> upload;
        D3D12_RESOURCE_DESC ub = {};
        ub.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ub.Width = 4; ub.Height = 1;
        ub.MipLevels = 1; ub.DepthOrArraySize = 1;
        ub.SampleDesc.Count = 1;
        ub.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ub.Flags = D3D12_RESOURCE_FLAG_NONE;
        hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        if (SUCCEEDED(device_->CreateCommittedResource(
                &hp, D3D12_HEAP_FLAG_NONE, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&upload)))) {
            void* mapped = nullptr;
            D3D12_RANGE no_read = {0,0};
            if (SUCCEEDED(upload->Map(0, &no_read, &mapped))) {
                std::memcpy(mapped, &white, 4);
                upload->Unmap(0, nullptr);
            }

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
            layout.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            layout.Footprint.Width = 1; layout.Footprint.Height = 1;
            layout.Footprint.Depth = 1; layout.Footprint.RowPitch = 4;

            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = upload.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = layout;
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = white_tex.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = 0;

            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd;
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> q;
            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
            device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&cmd));
            cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            D3D12_RESOURCE_BARRIER b = {};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource = white_tex.Get();
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            b.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmd->ResourceBarrier(1, &b);
            cmd->Close();
            D3D12_COMMAND_QUEUE_DESC qd = {};
            qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            device_->CreateCommandQueue(&qd, IID_PPV_ARGS(&q));
            ID3D12CommandList* lists[] = { cmd.Get() };
            q->ExecuteCommandLists(1, lists);
            device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
            HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            q->Signal(fence.Get(), 1);
            fence->SetEventOnCompletion(1, evt);
            WaitForSingleObject(evt, INFINITE);
            CloseHandle(evt);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = heap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    if (white_tex) device_->CreateShaderResourceView(white_tex.Get(), &srv, cpu);

    default_slot_ = 0;
    next_slot_ = 1;
}

void TextureCache::Shutdown() {
    heap_.Reset();
    sampler_heap_.Reset();
    device_.Reset();
    cache_.clear();
}

TextureData TextureCache::Load(const std::string& path) {
    if (!device_ || !heap_) return {};
    for (auto& c : cache_) {
        if (c.path == path) return c.data;
    }
    if (next_slot_ >= kMaxTextures) {
        XE_LOG_WARN("TextureCache: out of slots, cannot load " + path);
        return {};
    }

    std::wstring wpath(path.begin(), path.end());
    auto data = LoadWIC(device_.Get(), GetWIC().factory.Get(), wpath);
    if (!data.valid) {
        XE_LOG_WARN("TextureCache: failed to load " + path);
        return {};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = heap_->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += next_slot_ * descriptor_size_;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    device_->CreateShaderResourceView(data.resource.Get(), &srv, cpu);

    next_slot_++;

    CacheEntry entry;
    entry.path = path;
    entry.data = data;
    entry.data.slot = static_cast<int>(next_slot_ - 1);
    cache_.push_back(std::move(entry));
    return cache_.back().data;
}

void TextureCache::SetActive(ID3D12GraphicsCommandList* cmd_list) {
    if (heap_) cmd_list->SetDescriptorHeaps(1, heap_.GetAddressOf());
}

}  // namespace xe