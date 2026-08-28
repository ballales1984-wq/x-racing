#include "dx11_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "engine/input/input.h"

namespace p0::renderer {

namespace {

void multiply_matrices(const float* a, const float* b, float* out) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      out[col * 4 + row] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        out[col * 4 + row] += a[k * 4 + row] * b[col * 4 + k];
      }
    }
  }
}

const char* kVertexShader = R"(
struct VS_INPUT {
  float3 position : POSITION;
  float3 normal : NORMAL;
  float4 bone_ids : BLENDINDICES;
  float4 bone_weights : BLENDWEIGHT;
};

struct VS_OUTPUT {
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR;
};

cbuffer VS_CONSTANTS : register(b0) {
  float4x4 view_projection;
  float4x4 model;
};

cbuffer BONE_CONSTANTS : register(b1) {
  row_major float4x4 bone_matrices[128];
};

VS_OUTPUT main(VS_INPUT input) {
  VS_OUTPUT output;
  uint4 bone_indices = (uint4)(input.bone_ids + 0.5);
  float4x4 skin_matrix = 
    bone_matrices[bone_indices.x] * input.bone_weights.x +
    bone_matrices[bone_indices.y] * input.bone_weights.y +
    bone_matrices[bone_indices.z] * input.bone_weights.z +
    bone_matrices[bone_indices.w] * input.bone_weights.w;

  float4 world_pos = mul(skin_matrix, float4(input.position, 1.0));
  output.position = mul(view_projection, world_pos);
  output.normal = normalize(mul((float3x3)skin_matrix, input.normal));
  output.color = float4(0.9, 0.1, 0.1, 1.0);
  return output;
}
)";

const char* kPixelShader = R"(
struct PS_INPUT {
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR;
};

cbuffer PS_CONSTANTS : register(b0) {
  float3 light_dir;
  float ambient;
  float diffuse;
  float3 padding;
};

float4 main(PS_INPUT input) : SV_TARGET {
  float3 n = normalize(input.normal);
  float ndl = max(0.0, dot(n, normalize(light_dir)));
  float light = ambient + diffuse * ndl;
  return float4(input.color.rgb * light, 1.0);
}
)";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (msg == WM_KEYDOWN && wparam == VK_ESCAPE) {
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return 0;
  }
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

}

DX11Renderer::DX11Renderer(simulation::Simulation& sim, const DX11Config& config)
    : sim_(sim), config_(config) {
}

DX11Renderer::~DX11Renderer() {
  if (context_) {
    context_->ClearState();
    context_->Flush();
  }
  if (bone_buffer_) bone_buffer_->Release();
  if (ps_constant_buffer_) ps_constant_buffer_->Release();
  if (vs_constant_buffer_) vs_constant_buffer_->Release();
  if (index_buffer_) index_buffer_->Release();
  if (vertex_buffer_) vertex_buffer_->Release();
  if (input_layout_) input_layout_->Release();
  if (pixel_shader_) pixel_shader_->Release();
  if (vertex_shader_) vertex_shader_->Release();
  if (depth_stencil_state_) depth_stencil_state_->Release();
  if (depth_stencil_view_) depth_stencil_view_->Release();
  if (render_target_view_) render_target_view_->Release();
  if (swap_chain_) swap_chain_->Release();
  if (context_) context_->Release();
  if (device_) device_->Release();
  if (window_) DestroyWindow(window_);
}

bool DX11Renderer::initialize() {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = "DX11Renderer";

  if (!RegisterClassEx(&wc)) {
    std::cerr << "Failed to register window class" << std::endl;
    return false;
  }

  RECT rect = {0, 0, config_.width, config_.height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  window_ = CreateWindowEx(
      0, "DX11Renderer", "X-Racing DX11",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      rect.right - rect.left, rect.bottom - rect.top,
      nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

  if (!window_) {
    std::cerr << "Failed to create window" << std::endl;
    return false;
  }

  ShowWindow(window_, SW_SHOWNORMAL);
  UpdateWindow(window_);

  if (!create_device_and_swapchain(window_)) return false;
  if (!create_shaders()) return false;
  if (!create_buffers()) return false;

  return true;
}

bool DX11Renderer::create_device_and_swapchain(HWND window) {
  DXGI_SWAP_CHAIN_DESC swap_desc = {};
  swap_desc.BufferCount = 2;
  swap_desc.BufferDesc.Width = config_.width;
  swap_desc.BufferDesc.Height = config_.height;
  swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_desc.OutputWindow = window;
  swap_desc.SampleDesc.Count = 1;
  swap_desc.Windowed = TRUE;
  swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
      &feature_level, 1, D3D11_SDK_VERSION,
      &swap_desc, &swap_chain_, &device_, nullptr, &context_);
  if (FAILED(hr)) {
    std::cerr << "Failed to create DX11 device" << std::endl;
    return false;
  }

  ID3D11Texture2D* back_buffer = nullptr;
  hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
  if (FAILED(hr)) return false;

  hr = device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_);
  back_buffer->Release();
  if (FAILED(hr)) return false;

  D3D11_TEXTURE2D_DESC depth_desc = {};
  depth_desc.Width = config_.width;
  depth_desc.Height = config_.height;
  depth_desc.MipLevels = 1;
  depth_desc.ArraySize = 1;
  depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_desc.SampleDesc.Count = 1;
  depth_desc.Usage = D3D11_USAGE_DEFAULT;
  depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

  ID3D11Texture2D* depth_buffer = nullptr;
  hr = device_->CreateTexture2D(&depth_desc, nullptr, &depth_buffer);
  if (FAILED(hr)) return false;

  hr = device_->CreateDepthStencilView(depth_buffer, nullptr, &depth_stencil_view_);
  depth_buffer->Release();
  if (FAILED(hr)) return false;

  D3D11_DEPTH_STENCIL_DESC ds_desc = {};
  ds_desc.DepthEnable = TRUE;
  ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  ds_desc.DepthFunc = D3D11_COMPARISON_LESS;
  hr = device_->CreateDepthStencilState(&ds_desc, &depth_stencil_state_);
  if (FAILED(hr)) return false;

  context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);
  context_->OMSetDepthStencilState(depth_stencil_state_, 0);

  D3D11_VIEWPORT viewport = {};
  viewport.Width = (float)config_.width;
  viewport.Height = (float)config_.height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  context_->RSSetViewports(1, &viewport);

  return true;
}

bool DX11Renderer::create_shaders() {
  ID3DBlob* vs_blob = nullptr;
  ID3DBlob* error_blob = nullptr;
  HRESULT hr = D3DCompile(kVertexShader, strlen(kVertexShader), nullptr, nullptr, nullptr,
                          "main", "vs_5_0", 0, 0, &vs_blob, &error_blob);
  if (FAILED(hr)) {
    if (error_blob) {
      OutputDebugStringA((char*)error_blob->GetBufferPointer());
      error_blob->Release();
    }
    std::cerr << "Failed to compile vertex shader" << std::endl;
    return false;
  }

  hr = device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                                    nullptr, &vertex_shader_);
  if (FAILED(hr)) {
    vs_blob->Release();
    return false;
  }

  D3D11_INPUT_ELEMENT_DESC layout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  hr = device_->CreateInputLayout(layout, 4, vs_blob->GetBufferPointer(),
                                  vs_blob->GetBufferSize(), &input_layout_);
  vs_blob->Release();
  if (FAILED(hr)) return false;

  ID3DBlob* ps_blob = nullptr;
  hr = D3DCompile(kPixelShader, strlen(kPixelShader), nullptr, nullptr, nullptr,
                  "main", "ps_5_0", 0, 0, &ps_blob, &error_blob);
  if (FAILED(hr)) {
    if (error_blob) {
      OutputDebugStringA((char*)error_blob->GetBufferPointer());
      error_blob->Release();
    }
    std::cerr << "Failed to compile pixel shader" << std::endl;
    return false;
  }

  hr = device_->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
                                  nullptr, &pixel_shader_);
  ps_blob->Release();
  if (FAILED(hr)) return false;

  D3D11_BUFFER_DESC cb_desc = {};
  cb_desc.Usage = D3D11_USAGE_DYNAMIC;
  cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  cb_desc.ByteWidth = sizeof(ConstantBufferVS);
  hr = device_->CreateBuffer(&cb_desc, nullptr, &vs_constant_buffer_);
  if (FAILED(hr)) return false;

  cb_desc.ByteWidth = sizeof(ConstantBufferPS);
  hr = device_->CreateBuffer(&cb_desc, nullptr, &ps_constant_buffer_);
  if (FAILED(hr)) return false;

  cb_desc.ByteWidth = sizeof(float) * 16 * 128;
  hr = device_->CreateBuffer(&cb_desc, nullptr, &bone_buffer_);
  if (FAILED(hr)) return false;

  return true;
}

bool DX11Renderer::create_buffers() {
  if (vertices_.empty() || indices_.empty()) return false;

  D3D11_BUFFER_DESC vb_desc = {};
  vb_desc.ByteWidth = (UINT)(vertices_.size() * sizeof(float));
  vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
  vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vb_data = {};
  vb_data.pSysMem = vertices_.data();
  HRESULT hr = device_->CreateBuffer(&vb_desc, &vb_data, &vertex_buffer_);
  if (FAILED(hr)) return false;

  D3D11_BUFFER_DESC ib_desc = {};
  ib_desc.ByteWidth = (UINT)(indices_.size() * sizeof(uint32_t));
  ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
  ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA ib_data = {};
  ib_data.pSysMem = indices_.data();
  hr = device_->CreateBuffer(&ib_desc, &ib_data, &index_buffer_);
  if (FAILED(hr)) return false;

  return true;
}

void DX11Renderer::load_gltf(const std::string& filename) {
  vertices_.clear();
  indices_.clear();
  bone_transforms_.clear();

  if (!p0::assets::GLTFLoader::LoadSkinned(filename, skinned_mesh_)) {
    std::cerr << "DX11: failed to load skinned GLTF " << filename << std::endl;
    return;
  }

  size_t vertex_count = skinned_mesh_.positions.size() / 3;
  size_t index_count = skinned_mesh_.indices.size();

  vertices_.reserve(vertex_count * 14);
  indices_.reserve(index_count);

  for (size_t i = 0; i < vertex_count; ++i) {
    float px = skinned_mesh_.positions[i * 3];
    float py = skinned_mesh_.positions[i * 3 + 1];
    float pz = skinned_mesh_.positions[i * 3 + 2];
    vertices_.push_back(px);
    vertices_.push_back(py);
    vertices_.push_back(pz);

    float nx = 0, ny = 1, nz = 0;
    if (i * 3 + 2 < skinned_mesh_.normals.size()) {
      nx = skinned_mesh_.normals[i * 3];
      ny = skinned_mesh_.normals[i * 3 + 1];
      nz = skinned_mesh_.normals[i * 3 + 2];
    }
    vertices_.push_back(nx);
    vertices_.push_back(ny);
    vertices_.push_back(nz);

    float b0 = 0, b1 = 0, b2 = 0, b3 = 0;
    if (i * 4 + 3 < skinned_mesh_.bone_ids.size()) {
      b0 = (float)skinned_mesh_.bone_ids[i * 4];
      b1 = (float)skinned_mesh_.bone_ids[i * 4 + 1];
      b2 = (float)skinned_mesh_.bone_ids[i * 4 + 2];
      b3 = (float)skinned_mesh_.bone_ids[i * 4 + 3];
    }
    vertices_.push_back(b0);
    vertices_.push_back(b1);
    vertices_.push_back(b2);
    vertices_.push_back(b3);

    float w0 = 1.0f, w1 = 0, w2 = 0, w3 = 0;
    if (i * 4 + 3 < skinned_mesh_.bone_weights.size()) {
      w0 = skinned_mesh_.bone_weights[i * 4];
      w1 = skinned_mesh_.bone_weights[i * 4 + 1];
      w2 = skinned_mesh_.bone_weights[i * 4 + 2];
      w3 = skinned_mesh_.bone_weights[i * 4 + 3];
    }
    vertices_.push_back(w0);
    vertices_.push_back(w1);
    vertices_.push_back(w2);
    vertices_.push_back(w3);
  }

  indices_.assign(skinned_mesh_.indices.begin(), skinned_mesh_.indices.end());

  if (!skinned_mesh_.animations.empty()) {
    has_animation_ = true;
    current_animation_ = 0;
  }

  bone_transforms_.resize(skinned_mesh_.bones.size() * 16, 0.0f);

  if (vertex_buffer_) vertex_buffer_->Release();
  if (index_buffer_) index_buffer_->Release();
  create_buffers();

  std::cout << "DX11: loaded skinned GLTF " << filename << std::endl;
  std::cout << "  Vertices: " << vertex_count << std::endl;
  std::cout << "  Triangles: " << index_count / 3 << std::endl;
  std::cout << "  Bones: " << skinned_mesh_.bones.size() << std::endl;
  std::cout << "  Animations: " << skinned_mesh_.animations.size() << std::endl;
}

void DX11Renderer::update_bone_matrices(float time_seconds) {
  if (skinned_mesh_.bones.empty()) return;

  for (size_t i = 0; i < skinned_mesh_.bones.size(); ++i) {
    const auto& bone = skinned_mesh_.bones[i];
    const float* m = bone.local_transform.data();
    for (int r = 0; r < 16; ++r) {
      bone_transforms_[i * 16 + r] = m[r];
    }
  }
}

void DX11Renderer::play_animation(float delta_time) {
  if (!has_animation_ || skinned_mesh_.animations.empty()) return;

  const auto& anim = skinned_mesh_.animations[current_animation_];
  anim_time_ += delta_time;
  if (anim_time_ > (float)anim.duration_seconds) {
    anim_time_ = fmod(anim_time_, (float)anim.duration_seconds);
  }

  update_bone_matrices(anim_time_);
}

void DX11Renderer::render_frame() {
  float clear_color[4] = {0.12f, 0.12f, 0.12f, 1.0f};
  context_->ClearRenderTargetView(render_target_view_, clear_color);
  context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH, 1.0f, 0);

  if (!vertices_.empty() && vertex_buffer_ && index_buffer_) {
    context_->IASetInputLayout(input_layout_);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = 14 * sizeof(float);
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
    context_->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);

    context_->VSSetShader(vertex_shader_, nullptr, 0);
    context_->PSSetShader(pixel_shader_, nullptr, 0);

    D3D11_MAPPED_SUBRESOURCE mapped_vs = {};
    if (SUCCEEDED(context_->Map(vs_constant_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vs))) {
      ConstantBufferVS* cb = (ConstantBufferVS*)mapped_vs.pData;
      float aspect = (float)config_.width / (float)config_.height;
      float fov = 1.0f / tanf(60.0f * 3.14159f / 360.0f);

      float proj[16] = {
        fov / aspect, 0, 0, 0,
        0, fov, 0, 0,
        0, 0, 200.1f / -199.9f, -1,
        0, 0, (200.0f * 0.1f) / -199.9f, 0
      };

      const auto& state = sim_.state();
      float cx = (float)state.position.x();
      float cz = (float)state.position.y();
      float heading = (float)state.heading;

      float eye_x = cx - cosf(heading) * 8.0f;
      float eye_y = 4.0f;
      float eye_z = cz - sinf(heading) * 8.0f;
      float target_x = cx + cosf(heading) * 2.0f;
      float target_z = cz + sinf(heading) * 2.0f;

      float fx = target_x - eye_x;
      float fy = 0.0f - eye_y;
      float fz = target_z - eye_z;
      float flen = sqrtf(fx*fx + fy*fy + fz*fz);
      fx /= flen; fy /= flen; fz /= flen;

      float sx = 0.0f - fz;
      float sy = 0.0f;
      float sz = fx;
      float slen = sqrtf(sx*sx + sy*sy + sz*sz);
      sx /= slen; sy /= slen; sz /= slen;

      float ux = sy * fz - fy * sz;
      float uy = fz * sx - fx * sz;
      float uz = fx * sy - fy * sx;

      float view[16] = {
        sx, ux, -fx, 0,
        sy, uy, -fy, 0,
        sz, uz, -fz, 0,
        -(sx*eye_x + sy*eye_y + sz*eye_z),
        -(ux*eye_x + uy*eye_y + uz*eye_z),
        (fx*eye_x + fy*eye_y + fz*eye_z),
        1
      };

      float view_proj[16];
      multiply_matrices(view, proj, view_proj);

      float model[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        cx, 0.5f, cz, 1
      };

      for (int i = 0; i < 16; ++i) cb->view_projection[i] = view_proj[i];
      for (int i = 0; i < 16; ++i) cb->model[i] = model[i];
      context_->Unmap(vs_constant_buffer_, 0);
    }

    D3D11_MAPPED_SUBRESOURCE mapped_ps = {};
    if (SUCCEEDED(context_->Map(ps_constant_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_ps))) {
      ConstantBufferPS* cb = (ConstantBufferPS*)mapped_ps.pData;
      cb->light_dir[0] = 0.4f; cb->light_dir[1] = 0.8f; cb->light_dir[2] = 0.3f;
      cb->ambient = 0.35f;
      cb->diffuse = 0.65f;
      context_->Unmap(ps_constant_buffer_, 0);
    }

    if (!bone_transforms_.empty() && bone_buffer_) {
      D3D11_MAPPED_SUBRESOURCE mapped_bone = {};
      if (SUCCEEDED(context_->Map(bone_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_bone))) {
        memcpy(mapped_bone.pData, bone_transforms_.data(),
               std::min(bone_transforms_.size(), (size_t)(16 * 128)) * sizeof(float));
        context_->Unmap(bone_buffer_, 0);
      }
      context_->VSSetConstantBuffers(1, 1, &bone_buffer_);
    }

    context_->VSSetConstantBuffers(0, 1, &vs_constant_buffer_);
    context_->PSSetConstantBuffers(0, 1, &ps_constant_buffer_);

    context_->DrawIndexed((UINT)indices_.size(), 0, 0);
  }

  swap_chain_->Present(config_.vsync ? 1 : 0, 0);
}

void DX11Renderer::run() {
  running_ = true;
  load_gltf("D:/x-racing/data/models/vehicle.glb");

  LARGE_INTEGER freq, prev;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&prev);

  MSG msg = {};
  while (running_) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) running_ = false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    LARGE_INTEGER curr;
    QueryPerformanceCounter(&curr);
    double dt = (double)(curr.QuadPart - prev.QuadPart) / (double)freq.QuadPart;
    prev = curr;

    if (dt > 0.0 && dt < 0.1) {
      time_ += (float)dt;

      input::InputState input;
      if (GetAsyncKeyState('W') & 0x8000) input.throttle = 1.0;
      if (GetAsyncKeyState('S') & 0x8000) input.brake = 1.0;
      if (GetAsyncKeyState('A') & 0x8000) input.steering = -1.0;
      if (GetAsyncKeyState('D') & 0x8000) input.steering = 1.0;
      if (GetAsyncKeyState('R') & 0x8000) input.reset = true;

      sim_.step(input);
      play_animation((float)dt);
      render_frame();
    }
  }
}

}
