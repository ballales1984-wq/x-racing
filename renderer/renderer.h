#pragma once

#include <windows.h>
#include "common.h"
#include "simulation/simulation.h"
#include "input/input.h"
#include "../engine/assets/mesh.h"

namespace p0::renderer {

struct RendererConfig {
  int width = 1280;
  int height = 720;
  double scale = 4.0;
  bool show_grid = true;
  bool show_telemetry = true;
};

class Renderer {
 public:
  explicit Renderer(simulation::Simulation& sim, const RendererConfig& config = {});
  ~Renderer();

  bool initialize();
  void run();

  void load_car_mesh(const std::string& filename);

 private:
  void draw_track(HDC hdc);
  void draw_box_lane(HDC hdc);
  void draw_car(HDC hdc, const vehicle::VehicleState& state);
  void draw_car_3d(HDC hdc, const vehicle::VehicleState& state);
  void draw_hud(HDC hdc, const simulation::SimulationResult& result);
  void handle_input(input::InputState& input);

  Vec3 project(const Vec3& world_pos) const;
  Mat4 view_matrix() const;

  simulation::Simulation& sim_;
  RendererConfig config_;
  HWND window_ = nullptr;
  HDC mem_dc_ = nullptr;
  HBITMAP mem_bitmap_ = nullptr;
  bool running_ = false;
  double time_ = 0.0;
  std::vector<p0::assets::Mesh> car_meshes_;
  bool show_3d_car_ = false;
};

}
