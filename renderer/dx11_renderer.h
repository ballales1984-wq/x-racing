#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include "common.h"
#include "simulation/simulation.h"
#include "input/input.h"
#include "../engine/assets/gltf_loader.h"

// Project 0 — minimal DirectX 11 renderer
// Renders skinned GLB/GLTF meshes with bone animation and simple lighting.
// Falls back to GDI renderer if DX11 is unavailable.
namespace p0::renderer {

struct DX11Config {
  int width = 1280;
  int height = 720;
  bool vsync = false;
};

class DX11Renderer {
 public:
  explicit DX11Renderer(simulation::Simulation& sim, const DX11Config& config = {});
  ~DX11Renderer();

  bool initialize();
  void run();

  void load_gltf(const std::string& filename);

 private:
  struct ConstantBufferVS {
    float view_projection[16];
    float model[16];
  };

  struct ConstantBufferPS {
    float light_dir[3];
    float ambient;
    float diffuse;
    float padding[3];
  };

  bool create_device_and_swapchain(HWND window);
  bool create_shaders();
  bool create_buffers();
  void render_frame();
  void update_bone_matrices(float time_seconds);
  void play_animation(float delta_time);

  simulation::Simulation& sim_;
  DX11Config config_;

  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;
  IDXGISwapChain* swap_chain_ = nullptr;
  ID3D11RenderTargetView* render_target_view_ = nullptr;
  ID3D11DepthStencilView* depth_stencil_view_ = nullptr;
  ID3D11DepthStencilState* depth_stencil_state_ = nullptr;

  ID3D11VertexShader* vertex_shader_ = nullptr;
  ID3D11PixelShader* pixel_shader_ = nullptr;
  ID3D11InputLayout* input_layout_ = nullptr;
  ID3D11Buffer* vertex_buffer_ = nullptr;
  ID3D11Buffer* index_buffer_ = nullptr;
  ID3D11Buffer* vs_constant_buffer_ = nullptr;
  ID3D11Buffer* ps_constant_buffer_ = nullptr;
  ID3D11Buffer* bone_buffer_ = nullptr;

  std::vector<float> vertices_;
  std::vector<uint32_t> indices_;
  std::vector<float> bone_transforms_;

  p0::assets::GLTFSkinnedMesh skinned_mesh_;
  bool has_animation_ = false;
  float anim_time_ = 0.0f;
  int current_animation_ = 0;

  bool running_ = false;
  float time_ = 0.0f;
  HWND window_ = nullptr;
};

}
