#pragma once

#define NOMINMAX
#include <windows.h>
#include "common.h"
#include "simulation/simulation.h"
#include "input/input.h"
#include "../engine/camera/camera.h"
#include "../engine/assets/mesh.h"
#include "../engine/assets/gltf_loader.h"
#include "../engine/tracking/lap_system.h"
#include "../engine/tracking/track_position.h"
#include <sstream>
#include <iomanip>

// Project 0 — Windows GDI renderer with optional 3D wireframe car
// Namespace: p0::renderer
namespace p0::renderer {

// Rendering configuration: window size, zoom scale and debug overlays.
struct RendererConfig {
  int width = 1280;
  int height = 720;
  double scale = 4.0;
  bool show_grid = true;
  bool show_telemetry = true;

  // Chase-camera tuning (Phase 1.4).
  double cam_distance = 8.0;    // m, eye distance behind the car
  double cam_height = 4.0;      // m, eye height above the car
  double cam_look_ahead = 2.0; // m, target offset ahead of the car
  double cam_smoothing = 8.0;   // 1/s, camera smoothing rate
};

// Real-time renderer for the simulation.
// Owns the Win32 window, back buffer and input handling.
// Draws the track, car (2D rect or 3D wireframe) and a simple HUD.
class Renderer {
 public:
  explicit Renderer(simulation::Simulation& sim, const RendererConfig& config = {});
  ~Renderer();

  // Create the Win32 window and back buffer. Returns false on failure.
  bool initialize();
  // Enter the main loop: pump messages, step sim, draw and present.
  void run();

  // Load an external car mesh (OBJ) for 3D rendering.
  void load_car_mesh(const std::string& filename);
  // Toggle 3D wireframe car mode (called from window procedure).
  void toggle_3d_mode();
  // Switch the active track layout at runtime.
  void set_track_type(track::TrackType type);

  private:
   // Draw the track centerline and boundaries.
   void draw_track(HDC hdc);
   // Draw the pit/box lane in red.
   void draw_box_lane(HDC hdc);
   // Draw start/finish line in gold.
   void draw_start_finish(HDC hdc);
   // Draw direction arrows along the track centerline.
   void draw_direction_arrows(HDC hdc);
   // Draw the car as a 2D rotated rectangle.
   void draw_car(HDC hdc, const vehicle::VehicleState& state);
  // Draw the car as a 3D wireframe using the loaded mesh and a simple projection.
  void draw_car_3d(HDC hdc, const vehicle::VehicleState& state);
  // Draw speed, RPM, gear, lap and telemetry HUD overlay.
  void draw_hud(HDC hdc, const simulation::SimulationResult& result);
   // Poll keyboard and build the per-frame input state.
   void handle_input(input::InputState& input);
   // Recreate the back buffer at a new size (called on WM_SIZE).
   void resize_back_buffer(int width, int height);

  // Project a 3D world point to 2D screen coordinates using a look-at view matrix
    // and a perspective projection (delegates to the chase camera).
    Vec3 project(const Vec3& world_pos) const;
    // Build a simple look-at view matrix following the car from behind.
    Mat4 view_matrix() const;

    std::string format_time(double seconds) const;
    p0::tracking::TrackPosition build_track_position(const simulation::SimulationResult& result) const;

     simulation::Simulation& sim_;
     RendererConfig config_;
     HWND window_ = nullptr;
     HDC mem_dc_ = nullptr;
     HBITMAP mem_bitmap_ = nullptr;
     bool running_ = false;
     double time_ = 0.0;
     std::vector<p0::assets::Mesh> car_meshes_;
     bool show_3d_car_ = true;
     float car_mesh_scale_ = 1.0f;
     track::TrackType current_track_type_ = track::TrackType::Default;
     track::Track current_track_;
     p0::camera::ChaseCamera camera_;
     p0::tracking::LapSystem lap_system_;
     double current_lap_time_ = 0.0;
     double best_lap_time_ = 0.0;
     double last_lap_time_ = 0.0;
     bool lap_invalidated_ = false;

};

