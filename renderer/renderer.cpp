#include "renderer/renderer.h"
#include "telemetry/telemetry.h"
#include "input/input.h"
#include <windows.h>
#include <cmath>

namespace p0::renderer {

static const char* kWindowClass = "X-Racing";
static const char* kWindowTitle = "X-Racing Simulator";

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
  telemetry::Telemetry tel;
  LARGE_INTEGER freq, prev, curr;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&prev);

  MSG msg = {};
  input::InputState input;

  while (running_) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) running_ = false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    QueryPerformanceCounter(&curr);
    const double dt = static_cast<double>(curr.QuadPart - prev.QuadPart) / freq.QuadPart;
    prev = curr;

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
      draw_car(mem_dc, result.state);
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

  for (int i = 0; i < static_cast<int>(length); i += step) {
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

void Renderer::draw_car(HDC hdc, const vehicle::VehicleState& state) {
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
}

}
