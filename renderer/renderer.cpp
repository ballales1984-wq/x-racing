#include "renderer/renderer.h"
#include "telemetry/telemetry.h"
#include "input/input.h"
#include "../engine/assets/mesh.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace p0::renderer {

static const char* kWindowClass = "X-Racing";
static const char* kWindowTitle = "X-Racing Simulator";

// Window message handler: closes the application on window destroy or ESC.
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
      if (wparam == VK_ESCAPE) PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

Renderer::Renderer(simulation::Simulation& sim, const RendererConfig& config)
    : sim_(sim), config_(config) {}

Renderer::~Renderer() {
  if (mem_bitmap_) DeleteObject(mem_bitmap_);
  if (mem_dc_) DeleteDC(mem_dc_);
  if (window_) DestroyWindow(window_);
}

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
  load_car_mesh("D:/x-racing/assets/models/vehicle.obj");
  if (car_meshes_.empty()) {
    load_car_mesh("D:/x-racing/assets/models/car.obj");
  }
  std::cout << "Renderer: loaded " << car_meshes_.size() << " mesh(es), 3D=" << (show_3d_car_ ? "ON" : "OFF") << std::endl;
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
      tel.record(result.state, dt);
      time_ += dt;

      HDC hdc = GetDC(window_);
      HDC mem_dc = mem_dc_;

      RECT fill_rect = {0, 0, config_.width, config_.height};
      FillRect(mem_dc, &fill_rect,
               CreateSolidBrush(RGB(30, 30, 30)));

      draw_track(mem_dc);
      draw_box_lane(mem_dc);
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
  HPEN old_pen = (HPEN)SelectObject(hdc, track_pen);

  for (int i = 0; i + step < static_cast<int>(length); i += step) {
    track::TrackPoint p0 = track->at(i);
    track::TrackPoint p1 = track->at(i + step);
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

  for (int i = 0; i + step < static_cast<int>(length); i += step) {
    track::TrackPoint p0 = track->at(i);
    track::TrackPoint p1 = track->at(i + step);

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

// Draw the car as a small rotated rectangle centered at its world position.
void Renderer::draw_car(HDC hdc, const vehicle::VehicleState& state) {
  // World-to-screen center (y is flipped so +y points up on screen).
  int cx = static_cast<int>(state.position.x() * config_.scale + config_.width / 2);
  int cy = static_cast<int>(-state.position.y() * config_.scale + config_.height / 2);
  double heading = state.heading;

  HPEN car_pen = CreatePen(PS_SOLID, 2, RGB(255, 100, 100));
  HBRUSH car_brush = CreateSolidBrush(RGB(255, 100, 100));
  HPEN old_pen = (HPEN)SelectObject(hdc, car_pen);
  HBRUSH old_brush = (HBRUSH)SelectObject(hdc, car_brush);

  const int car_len = 8;
  const int car_wid = 4;

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

  sprintf(buf, "Speed: %.1f km/h", s.speed * 3.6);
  TextOutA(hdc, 10, 10, buf, (int)strlen(buf));

  sprintf(buf, "RPM: %d", static_cast<int>(s.rpm));
  TextOutA(hdc, 10, 30, buf, (int)strlen(buf));

  sprintf(buf, "Gear: %d", s.gear);
  TextOutA(hdc, 10, 50, buf, (int)strlen(buf));

  sprintf(buf, "Time: %.2f s", result.time);
  TextOutA(hdc, 10, 70, buf, (int)strlen(buf));

  sprintf(buf, "Lap: %d", s.lap);
  TextOutA(hdc, 10, 90, buf, (int)strlen(buf));

  sprintf(buf, "3D Model: %s", show_3d_car_ ? "ON (M to toggle)" : "OFF (M to toggle)");
  TextOutA(hdc, 10, 110, buf, (int)strlen(buf));

  sprintf(buf, "Track: %s (1/2 to switch)", current_track_type_ == track::TrackType::Default ? "Default" : "PitCircuit");
  TextOutA(hdc, 10, 130, buf, (int)strlen(buf));
}

// Poll the keyboard and populate the per-frame input state.
// W/S or Up/Down drive throttle/brake; A/D or Left/Right steer; arrows also shift.
void Renderer::set_track_type(track::TrackType type) {
  current_track_type_ = type;
  sim_.set_track(track::Track(type));
}

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
  if (GetAsyncKeyState(VK_UP) & 0x8000) input.upshift = true;
  if (GetAsyncKeyState(VK_DOWN) & 0x8000) input.downshift = true;
  if (GetAsyncKeyState('M') & 0x8000) show_3d_car_ = !show_3d_car_;
  if (GetAsyncKeyState('1') & 0x8000) set_track_type(track::TrackType::Default);
  if (GetAsyncKeyState('2') & 0x8000) set_track_type(track::TrackType::PitCircuit);
}

void Renderer::load_car_mesh(const std::string& filename) {
  car_meshes_.clear();
  p0::assets::Mesh mesh;
  if (p0::assets::MeshLoader::LoadOBJ(filename, mesh)) {
    car_meshes_.push_back(std::move(mesh));
  }
}

Mat4 Renderer::view_matrix() const {
  const auto& state = sim_.state();
  Vec3 car_pos(state.position.x(), 0.0, state.position.y());

  double heading = state.heading;
  Vec3 eye = car_pos + Vec3(-std::sin(heading) * 8.0, 4.0, -std::cos(heading) * 8.0);
  Vec3 target = car_pos + Vec3(std::sin(heading) * 2.0, 0.0, std::cos(heading) * 2.0);
  Vec3 up(0.0, 1.0, 0.0);

  Vec3 f = (target - eye).normalized();
  Vec3 s = f.cross(up).normalized();
  Vec3 u = s.cross(f);

  Mat4 view = Mat4::Identity();
  view(0, 0) = s.x(); view(0, 1) = s.y(); view(0, 2) = s.z(); view(0, 3) = -s.dot(eye);
  view(1, 0) = u.x(); view(1, 1) = u.y(); view(1, 2) = u.z(); view(1, 3) = -u.dot(eye);
  view(2, 0) = -f.x(); view(2, 1) = -f.y(); view(2, 2) = -f.z(); view(2, 3) = f.dot(eye);
  return view;
}

Vec3 Renderer::project(const Vec3& world_pos) const {
  Mat4 view = view_matrix();
  Vec4 view_pos = view * Vec4(world_pos.x(), world_pos.y(), world_pos.z(), 1.0);

  if (view_pos.z() >= -0.1) return Vec3(-9999.0, -9999.0, 0.0);

  double fov = 60.0 * kDegToRad;
  double aspect = static_cast<double>(config_.width) / static_cast<double>(config_.height);
  double near_plane = 0.1;
  double far_plane = 200.0;
  double f = 1.0 / std::tan(fov / 2.0);

  Mat4 proj = Mat4::Identity();
  proj(0, 0) = f / aspect;
  proj(1, 1) = f;
  proj(2, 2) = (far_plane + near_plane) / (near_plane - far_plane);
  proj(2, 3) = (2.0 * far_plane * near_plane) / (near_plane - far_plane);
  proj(3, 2) = -1.0;
  proj(3, 3) = 0.0;

  Vec4 clip = proj * view_pos;
  if (clip.w() <= 0.0) return Vec3(-9999.0, -9999.0, 0.0);

  Vec3 ndc(clip.x() / clip.w(), clip.y() / clip.w(), clip.z() / clip.w());

  int sx = static_cast<int>((ndc.x() * 0.5 + 0.5) * config_.width);
  int sy = static_cast<int>((1.0 - (ndc.y() * 0.5 + 0.5)) * config_.height);

  return Vec3(static_cast<double>(sx), static_cast<double>(sy), ndc.z());
}

void Renderer::draw_car_3d(HDC hdc, const vehicle::VehicleState& state) {
  if (car_meshes_.empty()) return;

  Vec3 car_pos(state.position.x(), 0.5, state.position.y());
  double heading = state.heading;

  Mat4 model = Mat4::Identity();
  model(0, 0) = std::cos(heading); model(0, 2) = std::sin(heading);
  model(2, 0) = -std::sin(heading); model(2, 2) = std::cos(heading);

  HPEN car_pen = CreatePen(PS_SOLID, 1, RGB(255, 150, 150));
  HPEN old_pen = (HPEN)SelectObject(hdc, car_pen);

  for (const auto& mesh : car_meshes_) {
    size_t tri_count = mesh.indices.size() / 3;
    for (size_t i = 0; i < tri_count; ++i) {
      Vec3 v0 = mesh.vertices[mesh.indices[i * 3]];
      Vec3 v1 = mesh.vertices[mesh.indices[i * 3 + 1]];
      Vec3 v2 = mesh.vertices[mesh.indices[i * 3 + 2]];

      Vec4 p0_4d = model * Vec4(v0.x(), v0.y(), v0.z(), 1.0);
      Vec4 p1_4d = model * Vec4(v1.x(), v1.y(), v1.z(), 1.0);
      Vec4 p2_4d = model * Vec4(v2.x(), v2.y(), v2.z(), 1.0);

      Vec3 p0 = Vec3(p0_4d.x(), p0_4d.y(), p0_4d.z()) + car_pos;
      Vec3 p1 = Vec3(p1_4d.x(), p1_4d.y(), p1_4d.z()) + car_pos;
      Vec3 p2 = Vec3(p2_4d.x(), p2_4d.y(), p2_4d.z()) + car_pos;

      Vec3 s0 = project(p0);
      Vec3 s1 = project(p1);
      Vec3 s2 = project(p2);

      if (s0.z() > -1.0 || s1.z() > -1.0 || s2.z() > -1.0) {
        MoveToEx(hdc, static_cast<int>(s0.x()), static_cast<int>(s0.y()), nullptr);
        LineTo(hdc, static_cast<int>(s1.x()), static_cast<int>(s1.y()));
        MoveToEx(hdc, static_cast<int>(s1.x()), static_cast<int>(s1.y()), nullptr);
        LineTo(hdc, static_cast<int>(s2.x()), static_cast<int>(s2.y()));
        MoveToEx(hdc, static_cast<int>(s2.x()), static_cast<int>(s2.y()), nullptr);
        LineTo(hdc, static_cast<int>(s0.x()), static_cast<int>(s0.y()));
      }
    }
  }

  SelectObject(hdc, old_pen);
  DeleteObject(car_pen);
}

}
