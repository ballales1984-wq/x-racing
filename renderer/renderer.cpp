#include "renderer/renderer.h"
#include "telemetry/telemetry.h"
#include "input/input.h"
#include "../engine/assets/mesh.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace p0::renderer {

static Renderer* g_renderer = nullptr;

static const char* kWindowClass = "X-Racing";
static const char* kWindowTitle = "X-Racing Simulator";

// Win32 window procedure for the renderer window.
// Handles WM_DESTROY (quit), WM_KEYDOWN (ESC to quit, M to toggle 3D, 1/2 to switch track).
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_SIZE:
      if (g_renderer && wparam != SIZE_MINIMIZED) {
        int w = LOWORD(lparam);
        int h = HIWORD(lparam);
        g_renderer->resize_back_buffer(w, h);
      }
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
      if (wparam == VK_ESCAPE) {
        PostQuitMessage(0);
        return 0;
      }
      if (wparam == 'M' && g_renderer) {
        g_renderer->toggle_3d_mode();
        return 0;
      }
      if (wparam == '1' && g_renderer) {
        g_renderer->set_track_type(track::TrackType::Default);
        return 0;
      }
      if (wparam == '2' && g_renderer) {
        g_renderer->set_track_type(track::TrackType::PitCircuit);
        return 0;
      }
      if (wparam == '3' && g_renderer) {
        g_renderer->set_track_type(track::TrackType::CustomCircuit);
        return 0;
      }
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

Renderer::Renderer(simulation::Simulation& sim, const RendererConfig& config)
    : sim_(sim), config_(config),
      lap_system_(sim.track().length(), 0) {}

Renderer::~Renderer() {
  g_renderer = nullptr;
  if (mem_bitmap_) DeleteObject(mem_bitmap_);
  if (mem_dc_) DeleteDC(mem_dc_);
  if (window_) DestroyWindow(window_);
}

// Initialize the Win32 window and create the back-buffer rendering surface.
// Registers the window class, creates the overlapped window, and sets up
// a memory DC with a compatible bitmap for double-buffered drawing.
bool Renderer::initialize() {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = window_proc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = CreateSolidBrush(RGB(20, 20, 20));
  wc.lpszClassName = kWindowClass;

  if (!RegisterClassEx(&wc)) return false;

  // Adjust window rect to account for menu/border so client area matches config size.
  RECT rect = {0, 0, config_.width, config_.height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  window_ = CreateWindowEx(
    0, kWindowClass, kWindowTitle,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rect.right - rect.left, rect.bottom - rect.top,
    nullptr, nullptr, GetModuleHandle(nullptr), this
  );

  if (!window_) return false;

  g_renderer = this;

  // Create back-buffer: memory DC + compatible bitmap for double-buffered rendering.
  HDC hdc = GetDC(window_);
  mem_dc_ = CreateCompatibleDC(hdc);
  mem_bitmap_ = CreateCompatibleBitmap(hdc, config_.width, config_.height);
  SelectObject(mem_dc_, mem_bitmap_);
  ReleaseDC(window_, hdc);

  ShowWindow(window_, SW_SHOWNORMAL);
  UpdateWindow(window_);

  return true;
}

void Renderer::run() {
  if (!initialize()) return;

  running_ = true;
  {
    p0::assets::GLTFSkinnedMesh skinned;
    if (p0::assets::GLTFLoader::LoadSkinned("data/models/test_export.glb", skinned) && !skinned.positions.empty()) {
      car_meshes_.clear();
      p0::assets::Mesh mesh;
      mesh.vertices.reserve(skinned.positions.size() / 3);
      for (size_t i = 0; i < skinned.positions.size(); i += 3) {
        mesh.vertices.push_back(Vec3(skinned.positions[i], skinned.positions[i + 1], skinned.positions[i + 2]));
      }
      mesh.normals.reserve(skinned.normals.size() / 3);
      for (size_t i = 0; i < skinned.normals.size(); i += 3) {
        mesh.normals.push_back(Vec3(skinned.normals[i], skinned.normals[i + 1], skinned.normals[i + 2]));
      }
      mesh.uvs.reserve(skinned.uvs.size() / 2);
      for (size_t i = 0; i < skinned.uvs.size(); i += 2) {
        mesh.uvs.push_back(Vec2(skinned.uvs[i], skinned.uvs[i + 1]));
      }
      mesh.indices = skinned.indices;
      if (!skinned.materials.empty()) {
        mesh.colors.resize(mesh.vertices.size(), skinned.materials[0].base_color);
        mesh.material = skinned.materials[0].name;
      }
      if (mesh.colors.empty()) {
        mesh.colors.resize(mesh.vertices.size(), Vec3(0.9, 0.1, 0.1));
      }
      Vec3 min_v = mesh.vertices[0];
      Vec3 max_v = mesh.vertices[0];
      for (const auto& v : mesh.vertices) {
        min_v = min_v.cwiseMin(v);
        max_v = max_v.cwiseMax(v);
      }
      Vec3 size = max_v - min_v;
      double max_dim = std::abs(size.x());
      if (std::abs(size.y()) > max_dim) max_dim = std::abs(size.y());
      if (std::abs(size.z()) > max_dim) max_dim = std::abs(size.z());
      if (max_dim > 1e-6) car_mesh_scale_ = static_cast<float>(5.0 / max_dim);
      car_meshes_.push_back(std::move(mesh));
    }
  }
  if (car_meshes_.empty()) {
    load_car_mesh("data/models/vehicle.obj");
  }
  if (car_meshes_.empty()) {
    load_car_mesh("data/models/car_mesh.obj");
  }
  if (car_meshes_.empty()) {
    load_car_mesh("data/models/car.obj");
  }
  std::cout << "Renderer: loaded " << car_meshes_.size() << " mesh(es), 3D=" << (show_3d_car_ ? "ON" : "OFF") << std::endl;

  // Configure and prime the chase camera from the current car pose.
  camera_.set_config(p0::camera::CameraConfig{
    config_.cam_distance, config_.cam_height, config_.cam_look_ahead, config_.cam_smoothing});
  {
    const auto& s = sim_.state();
    camera_.update(s.position, s.heading, s.speed, 0.0);
  }

  telemetry::Telemetry tel;
  LARGE_INTEGER freq, prev, curr;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&prev);

  MSG msg = {};
  input::InputState input;

  // Main render loop: pump window messages, step the simulation,
  // record telemetry and repaint the back buffer.
  while (running_) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) running_ = false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    QueryPerformanceCounter(&curr);
    const double dt = static_cast<double>(curr.QuadPart - prev.QuadPart) / freq.QuadPart;
    prev = curr;

    // Ignore abnormally large frame deltas (e.g. after a stall) to keep the sim stable.
    if (dt > 0.0 && dt < 0.1) {
      handle_input(input);
      const auto result = sim_.step(input);
      camera_.update(result.state.position, result.state.heading, result.state.speed, dt);
      tel.record(result.state, dt);
      time_ += dt;

      const p0::tracking::TrackPosition track_pos = build_track_position(result);
      lap_system_.update(track_pos, time_);
      current_lap_time_ = lap_system_.current_lap_time();
      if (lap_system_.lap_finished()) {
        last_lap_time_ = lap_system_.last_lap_time();
        if (last_lap_time_ > 0.0 && (best_lap_time_ == 0.0 || last_lap_time_ < best_lap_time_)) {
          best_lap_time_ = last_lap_time_;
        }
        lap_system_.pop_events();
      }
      if (lap_system_.lap_invalidated()) {
        lap_invalidated_ = true;
        lap_system_.pop_events();
      }

      HDC hdc = GetDC(window_);
      HDC mem_dc = mem_dc_;

      RECT fill_rect = {0, 0, config_.width, config_.height};
      FillRect(mem_dc, &fill_rect,
               CreateSolidBrush(RGB(30, 30, 30)));

      draw_track(mem_dc);
      draw_box_lane(mem_dc);
      draw_start_finish(mem_dc);
      draw_direction_arrows(mem_dc);
      if (show_3d_car_) {
        draw_car_3d(mem_dc, result.state);
      } else {
        draw_car(mem_dc, result.state);
      }
      draw_hud(mem_dc, result);

      BitBlt(hdc, 0, 0, config_.width, config_.height, mem_dc, 0, 0, SRCCOPY);
      ReleaseDC(window_, hdc);
    }
  }

  tel.save_csv("telemetry.csv");
}

void Renderer::draw_track(HDC hdc) {
  const track::Track* track = &sim_.track();
  const int step = 5;
  const double length = track->length();

  HPEN track_pen = CreatePen(PS_SOLID, 3, RGB(100, 100, 100));
  HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, track_pen));

  for (int i = 0; i < static_cast<int>(length); i += step) {
    track::TrackPoint p0 = track->at(i);
    track::TrackPoint p1 = track->at((std::min)(i + step, static_cast<int>(length) - 1));
    int x0 = static_cast<int>(p0.position.x() * config_.scale + config_.width / 2);
    int y0 = static_cast<int>(-p0.position.y() * config_.scale + config_.height / 2);
    int x1 = static_cast<int>(p1.position.x() * config_.scale + config_.width / 2);
    int y1 = static_cast<int>(-p1.position.y() * config_.scale + config_.height / 2);

    MoveToEx(hdc, x0, y0, nullptr);
    LineTo(hdc, x1, y1);
  }

  SelectObject(hdc, old_pen);
  DeleteObject(track_pen);
}

void Renderer::draw_box_lane(HDC hdc) {
  const track::Track* track = &sim_.track();
  const int step = 5;
  const double length = track->length();

  HPEN box_pen = CreatePen(PS_SOLID, 2, RGB(255, 80, 80));
  HPEN old_pen = (HPEN)SelectObject(hdc, box_pen);

  for (int i = 0; i < static_cast<int>(length); i += step) {
    track::TrackPoint p0 = track->at(i);
    track::TrackPoint p1 = track->at((std::min)(i + step, static_cast<int>(length) - 1));

    if (!p0.has_box_lane || !p1.has_box_lane) continue;

    const double box_offset0 = -(p0.width / 2.0 + p0.box_lane_width / 2.0);
    const double box_offset1 = -(p1.width / 2.0 + p1.box_lane_width / 2.0);

    Vec2 pos0 = p0.position + p0.normal * box_offset0;
    Vec2 pos1 = p1.position + p1.normal * box_offset1;

    int x0 = static_cast<int>(pos0.x() * config_.scale + config_.width / 2);
    int y0 = static_cast<int>(-pos0.y() * config_.scale + config_.height / 2);
    int x1 = static_cast<int>(pos1.x() * config_.scale + config_.width / 2);
    int y1 = static_cast<int>(-pos1.y() * config_.scale + config_.height / 2);

    MoveToEx(hdc, x0, y0, nullptr);
    LineTo(hdc, x1, y1);
  }

  SelectObject(hdc, old_pen);
  DeleteObject(box_pen);
}

void Renderer::draw_start_finish(HDC hdc) {
  const track::Track* track = &sim_.track();
  const auto tp = track->at(0.0);
  const double half_w = tp.width * 0.5;

  const double nx = tp.normal.x();
  const double ny = tp.normal.y();

  const int x1 = static_cast<int>((tp.position.x() + nx * half_w) * config_.scale + config_.width / 2);
  const int y1 = static_cast<int>(-(tp.position.y() + ny * half_w) * config_.scale + config_.height / 2);
  const int x2 = static_cast<int>((tp.position.x() - nx * half_w) * config_.scale + config_.width / 2);
  const int y2 = static_cast<int>(-(tp.position.y() - ny * half_w) * config_.scale + config_.height / 2);

  HPEN sf_pen = CreatePen(PS_SOLID, 3, RGB(255, 215, 0));
  HPEN old_pen = (HPEN)SelectObject(hdc, sf_pen);
  MoveToEx(hdc, x1, y1, nullptr);
  LineTo(hdc, x2, y2);
  SelectObject(hdc, old_pen);
  DeleteObject(sf_pen);

  const int mid_x = (x1 + x2) / 2;
  const int mid_y = (y1 + y2) / 2;
  const int label_y = mid_y - 12;
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(255, 215, 0));
  TextOutA(hdc, mid_x - 30, label_y, "START/FINISH", 12);
}

void Renderer::draw_direction_arrows(HDC hdc) {
  const track::Track* track = &sim_.track();
  const double length = track->length();
  const double step = 80.0;

  HPEN arrow_pen = CreatePen(PS_SOLID, 2, RGB(180, 180, 180));
  HPEN old_pen = (HPEN)SelectObject(hdc, arrow_pen);
  HBRUSH old_brush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

  for (double d = 20.0; d < length; d += step) {
    const auto tp = track->at(d);
    const int cx = static_cast<int>(tp.position.x() * config_.scale + config_.width / 2);
    const int cy = static_cast<int>(-tp.position.y() * config_.scale + config_.height / 2);

    const double head_len = 6.0;
    const double ang = std::atan2(tp.tangent.y(), tp.tangent.x());

    POINT pts[3];
    pts[0].x = cx + static_cast<int>(std::cos(ang) * 10.0);
    pts[0].y = cy - static_cast<int>(std::sin(ang) * 10.0);
    pts[1].x = cx - static_cast<int>(std::cos(ang - 0.5) * head_len);
    pts[1].y = cy - static_cast<int>(std::sin(ang - 0.5) * head_len);
    pts[2].x = cx - static_cast<int>(std::cos(ang + 0.5) * head_len);
    pts[2].y = cy - static_cast<int>(std::sin(ang + 0.5) * head_len);

    Polygon(hdc, pts, 3);
  }

  SelectObject(hdc, old_pen);
  SelectObject(hdc, old_brush);
  DeleteObject(arrow_pen);
}

// Draw the car as a small rotated rectangle centered at its world position.
void Renderer::draw_car(HDC hdc, const vehicle::VehicleState& state) {
  int cx = static_cast<int>(state.position.x() * config_.scale + config_.width / 2);
  int cy = static_cast<int>(-state.position.y() * config_.scale + config_.height / 2);
  double heading = state.heading;

  HPEN car_pen = CreatePen(PS_SOLID, 2, RGB(255, 100, 100));
  HBRUSH car_brush = CreateSolidBrush(RGB(255, 100, 100));
  HPEN old_pen = (HPEN)SelectObject(hdc, car_pen);
  HBRUSH old_brush = (HBRUSH)SelectObject(hdc, car_brush);

  const track::Track* track = &sim_.track();
  const auto tp = track->at(state.distance_along_track);
  const double car_wid_world = tp.width / 4.0;
  const double car_len_world = car_wid_world * 2.0;
  const int car_len = static_cast<int>(car_len_world * config_.scale);
  const int car_wid = static_cast<int>(car_wid_world * config_.scale);

  POINT pts[4];
  for (int i = 0; i < 4; ++i) {
    double lx = (i < 2 ? 1.0 : -1.0) * car_len * 0.5;
    double ly = (i % 2 == 0 ? 1.0 : -1.0) * car_wid * 0.5;
    double rx = lx * std::cos(heading) - ly * std::sin(heading);
    double ry = lx * std::sin(heading) + ly * std::cos(heading);
    pts[i].x = cx + static_cast<int>(rx);
    pts[i].y = cy - static_cast<int>(ry);
  }

  Polygon(hdc, pts, 4);

  SelectObject(hdc, old_pen);
  SelectObject(hdc, old_brush);
  DeleteObject(car_pen);
  DeleteObject(car_brush);
}

// Draw the HUD: speed, RPM, gear, time and lap readouts.
void Renderer::draw_hud(HDC hdc, const simulation::SimulationResult& result) {
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(255, 255, 255));

  char buf[128];
  const auto& s = result.state;

  const int margin_x = static_cast<int>(config_.width * 0.02);
  const int margin_y = static_cast<int>(std::min(config_.height * 0.03, config_.height * 0.35));
  const int line_h = static_cast<int>(config_.height * 0.04);
  int y = margin_y;

  snprintf(buf, sizeof(buf), "Speed: %.1f km/h", s.speed * 3.6);
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "RPM: %d", static_cast<int>(s.rpm));
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Gear: %d", s.gear);
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Time: %s", format_time(current_lap_time_).c_str());
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Best Lap: %s", format_time(best_lap_time_).c_str());
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Last Lap: %s", format_time(last_lap_time_).c_str());
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Lap: %d", s.lap);
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  if (lap_invalidated_) {
    SetTextColor(hdc, RGB(255, 80, 80));
    TextOutA(hdc, margin_x, y, "INVALID LAP", 11);
    SetTextColor(hdc, RGB(255, 255, 255));
    y += line_h;
  }

  snprintf(buf, sizeof(buf), "3D Model: %s", show_3d_car_ ? "ON (M to toggle)" : "OFF (M to toggle)");
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
  y += line_h;

  snprintf(buf, sizeof(buf), "Track: %s (1/2/3 to switch)", current_track_type_ == track::TrackType::Default ? "Default" : current_track_type_ == track::TrackType::PitCircuit ? "PitCircuit" : "CustomCircuit");
  TextOutA(hdc, margin_x, y, buf, static_cast<int>(std::strlen(buf)));
}

void Renderer::toggle_3d_mode() {
  show_3d_car_ = !show_3d_car_;
  std::cout << "3D car mode: " << (show_3d_car_ ? "ON" : "OFF") << std::endl;
  if (window_) {
    std::string title = show_3d_car_ ? "X-Racing Simulator [3D ON]" : "X-Racing Simulator [3D OFF]";
    SetWindowTextA(window_, title.c_str());
  }
}

// Switch the active track type at runtime.
// Rebuilds the track object and updates the simulation reference.
void Renderer::set_track_type(track::TrackType type) {
  current_track_type_ = type;
  current_track_ = track::Track(type);
  sim_.set_track(current_track_);
  lap_system_ = p0::tracking::LapSystem(current_track_.length(), 0);
  current_lap_time_ = 0.0;
  best_lap_time_ = 0.0;
  last_lap_time_ = 0.0;
  lap_invalidated_ = false;
}

// Poll the keyboard and populate the per-frame input state.
// W/S or Up/Down drive throttle/brake; A/D or Left/Right steer; arrows also shift.
void Renderer::handle_input(input::InputState& input) {
  input.throttle = 0.0;
  input.brake = 0.0;
  input.steering = 0.0;
  input.upshift = false;
  input.downshift = false;

  if (GetAsyncKeyState('W') & 0x8000) input.throttle = 1.0;
  if (GetAsyncKeyState('S') & 0x8000) input.brake = 1.0;
  if (GetAsyncKeyState('A') & 0x8000) input.steering = -1.0;
  if (GetAsyncKeyState('D') & 0x8000) input.steering = 1.0;
  if (GetAsyncKeyState('X') & 0x8000) input.reverse = true;
  if (GetAsyncKeyState(VK_UP) & 0x8000) input.upshift = true;
  if (GetAsyncKeyState(VK_DOWN) & 0x8000) input.downshift = true;
}

// Recreate the back buffer at a new client size.
void Renderer::resize_back_buffer(int width, int height) {
  if (width == config_.width && height == config_.height) return;
  config_.width = width;
  config_.height = height;

  if (mem_bitmap_) {
    DeleteObject(mem_bitmap_);
    mem_bitmap_ = nullptr;
  }
  if (window_) {
    HDC hdc = GetDC(window_);
    if (hdc && mem_dc_) {
      mem_bitmap_ = CreateCompatibleBitmap(hdc, width, height);
      SelectObject(mem_dc_, mem_bitmap_);
      ReleaseDC(window_, hdc);
    }
  }
}
// Computes a uniform scale factor so the mesh fits within ~5 world units.
void Renderer::load_car_mesh(const std::string& filename) {
  car_meshes_.clear();
  car_mesh_scale_ = 1.0f;
  p0::assets::Mesh mesh;
  std::vector<p0::assets::Material> materials;
  if (p0::assets::MeshLoader::LoadOBJ(filename, mesh, materials)) {
    if (!mesh.vertices.empty()) {
      // Compute bounding box to derive a uniform scale factor.
      Vec3 min_v = mesh.vertices[0];
      Vec3 max_v = mesh.vertices[0];
      for (const auto& v : mesh.vertices) {
        min_v = min_v.cwiseMin(v);
        max_v = max_v.cwiseMax(v);
      }
      Vec3 size = max_v - min_v;
      double max_dim = std::abs(size.x());
      if (std::abs(size.y()) > max_dim) max_dim = std::abs(size.y());
      if (std::abs(size.z()) > max_dim) max_dim = std::abs(size.z());
      if (max_dim > 1e-6) {
        car_mesh_scale_ = static_cast<float>(5.0 / max_dim);
      }
    }
    car_meshes_.push_back(std::move(mesh));
  }
}

// Build the chase-camera view matrix.
// Delegates to the smoothed ChaseCamera owned by the renderer.
Mat4 Renderer::view_matrix() const {
  return camera_.view_matrix();
}

std::string Renderer::format_time(double seconds) const {
  if (seconds <= 0.0) return "--:--.---";
  int mins = static_cast<int>(seconds) / 60;
  double secs = seconds - mins * 60.0;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << mins << ":"
      << std::fixed << std::setprecision(3) << std::setw(6) << secs;
  return oss.str();
}

p0::tracking::TrackPosition Renderer::build_track_position(const simulation::SimulationResult& result) const {
  p0::tracking::TrackPosition pos{};
  const auto& s = result.state;
  const track::Track* track = &sim_.track();
  pos.s = s.distance_along_track;
  pos.on_track = !result.off_track;
  pos.heading = s.heading;
  if (track) {
    const auto tp = track->at(s.distance_along_track);
    pos.curvature = tp.curvature;
    pos.track_width = tp.width;
    pos.banking = tp.banking;
    pos.distance_to_centerline = 0.0;
    const Vec2 to_car = s.position - tp.position;
    pos.lateral = to_car.dot(tp.normal);
  }
  return pos;
}

// Project a world-space 3D point to screen coordinates.
// Returns (-9999, -9999, 0) if the point is behind the camera or invalid.
// Delegates to the ChaseCamera owned by the renderer.
Vec3 Renderer::project(const Vec3& world_pos) const {
  return camera_.project(world_pos, config_.width, config_.height);
}

// Draw the car as a filled 3D mesh with simple directional lighting.
// Uses per-vertex colors from the loaded GLTF/OBJ material and a fixed light direction.
void Renderer::draw_car_3d(HDC hdc, const vehicle::VehicleState& state) {
  if (car_meshes_.empty()) return;

  Vec3 car_pos(state.position.x(), 0.5, state.position.y());
  double heading = state.heading;

  Mat4 model = Mat4::Identity();
  model(0, 0) = std::cos(heading); model(0, 2) = std::sin(heading);
  model(2, 0) = -std::sin(heading); model(2, 2) = std::cos(heading);

  const Vec3 light_dir = Vec3(0.4, 0.8, 0.3).normalized();

  for (const auto& mesh : car_meshes_) {
    size_t tri_count = mesh.indices.size() / 3;
    for (size_t i = 0; i < tri_count; ++i) {
      int i0 = mesh.indices[i * 3];
      int i1 = mesh.indices[i * 3 + 1];
      int i2 = mesh.indices[i * 3 + 2];

      Vec3 v0 = mesh.vertices[i0] * car_mesh_scale_;
      Vec3 v1 = mesh.vertices[i1] * car_mesh_scale_;
      Vec3 v2 = mesh.vertices[i2] * car_mesh_scale_;

      Vec4 p0_4d = model * Vec4(v0.x(), v0.y(), v0.z(), 1.0);
      Vec4 p1_4d = model * Vec4(v1.x(), v1.y(), v1.z(), 1.0);
      Vec4 p2_4d = model * Vec4(v2.x(), v2.y(), v2.z(), 1.0);

      Vec3 p0 = Vec3(p0_4d.x(), p0_4d.y(), p0_4d.z()) + car_pos;
      Vec3 p1 = Vec3(p1_4d.x(), p1_4d.y(), p1_4d.z()) + car_pos;
      Vec3 p2 = Vec3(p2_4d.x(), p2_4d.y(), p2_4d.z()) + car_pos;

      Vec3 s0 = project(p0);
      Vec3 s1 = project(p1);
      Vec3 s2 = project(p2);

      if (s0.z() > -1.0 && s1.z() > -1.0 && s2.z() > -1.0) {
        Vec3 base_color(0.6, 0.6, 0.6);
        if (mesh.colors.size() == mesh.vertices.size() && mesh.colors.size() > static_cast<size_t>(i0)) {
          base_color = (mesh.colors[i0] + mesh.colors[i1] + mesh.colors[i2]) / 3.0;
        }

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.cross(edge2).normalized();
        double ndl = std::max(0.0, normal.dot(light_dir));
        double light = std::min(1.0, 0.35 + ndl * 0.65);

        int r = static_cast<int>(std::clamp(base_color.x() * light, 0.0, 1.0) * 255.0);
        int g = static_cast<int>(std::clamp(base_color.y() * light, 0.0, 1.0) * 255.0);
        int b = static_cast<int>(std::clamp(base_color.z() * light, 0.0, 1.0) * 255.0);

        HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
        HBRUSH old_brush = (HBRUSH)SelectObject(hdc, brush);
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);

        POINT pts[3];
        pts[0].x = static_cast<int>(s0.x()); pts[0].y = static_cast<int>(s0.y());
        pts[1].x = static_cast<int>(s1.x()); pts[1].y = static_cast<int>(s1.y());
        pts[2].x = static_cast<int>(s2.x()); pts[2].y = static_cast<int>(s2.y());
        Polygon(hdc, pts, 3);

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
        DeleteObject(brush);
        DeleteObject(pen);
      }
    }
  }
}

}
