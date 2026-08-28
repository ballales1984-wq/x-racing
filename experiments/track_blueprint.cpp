#include "track_blueprint.h"
#include <commctrl.h>
#include <commdlg.h>
#include <sstream>
#include <iomanip>
#include <cmath>`n#include <iostream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

namespace p0::track_blueprint {
constexpr COLORREF kGridColor = RGB(40, 40, 50);
constexpr COLORREF kMajorGridColor = RGB(60, 60, 80);
constexpr COLORREF kTrackFillColor = RGB(80, 80, 90);
constexpr COLORREF kTrackEdgeColor = RGB(200, 200, 200);
constexpr COLORREF kCenterlineColor = RGB(255, 255, 0);
constexpr COLORREF kVertexColor = RGB(0, 200, 255);
constexpr COLORREF kSelectedColor = RGB(255, 100, 100);
constexpr COLORREF kBoxLaneColor = RGB(255, 80, 80);
constexpr COLORREF kPitBoxColor = RGB(180, 180, 180);
constexpr COLORREF kBarrierColor = RGB(255, 140, 0);
constexpr COLORREF kStartFinishColor = RGB(255, 215, 0);
constexpr COLORREF kPreviewColor = RGB(100, 255, 100);
constexpr COLORREF kTextColor = RGB(220, 220, 220);
constexpr COLORREF kRulerBg = RGB(30, 30, 40);
constexpr COLORREF kRulerText = RGB(180, 180, 180);

BlueprintEditor::BlueprintEditor(const std::string& title) {
  track_.track_name = title;
  track_.track_id = "custom_track";
  track_.track_width = 12.0;
}

BlueprintEditor::~BlueprintEditor() {
  if (mem_bitmap_) DeleteObject(mem_bitmap_);
  if (mem_dc_) DeleteDC(mem_dc_);
  if (window_) DestroyWindow(window_);
}
bool BlueprintEditor::initialize() {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = CreateSolidBrush(RGB(20, 20, 25));
  wc.lpszClassName = "TrackBlueprintEditor";
  RegisterClassEx(&wc);

  RECT rect = {0, 0, config_.width, config_.height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
  int win_w = rect.right - rect.left;
  int win_h = rect.bottom - rect.top;

  window_ = CreateWindowEx(
    0, "TrackBlueprintEditor",
    ("X-Racing Blueprint: " + track_.track_name).c_str(),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, win_w, win_h,
    nullptr, nullptr, GetModuleHandle(nullptr), this);

  if (!window_) return false;

  ShowWindow(window_, SW_SHOW);
  UpdateWindow(window_);
  running_ = true;
  return true;
}
LRESULT CALLBACK BlueprintEditor::wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  BlueprintEditor* editor = nullptr;
  if (msg == WM_NCCREATE) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
    editor = reinterpret_cast<BlueprintEditor*>(cs->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(editor));
  } else {
    editor = reinterpret_cast<BlueprintEditor*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  }

  if (editor) {
    switch (msg) {
      case WM_CREATE: return editor->handle_create(hwnd);
      case WM_PAINT: return editor->handle_paint(hwnd);
      case WM_SIZE: return editor->handle_size(LOWORD(lp), HIWORD(lp));
      case WM_LBUTTONDOWN: return editor->handle_lbutton_down(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_LBUTTONUP: return editor->handle_lbutton_up(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_RBUTTONDOWN: return editor->handle_rbutton_down(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_MOUSEMOVE: return editor->handle_mouse_move(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_MOUSEWHEEL: return editor->handle_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wp));
      case WM_KEYDOWN: return editor->handle_key_down(hwnd, wp);
      case WM_COMMAND: return editor->handle_command(hwnd, wp);
      case WM_CLOSE: return editor->handle_close(hwnd);
      case WM_DESTROY: PostQuitMessage(0); return 0;
    }
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}
LRESULT BlueprintEditor::handle_create(HWND hwnd) {
  window_ = hwnd;
  HMENU menubar = CreateMenu();
  HMENU filemenu = CreatePopupMenu();
  AppendMenu(filemenu, MF_STRING, 1001, "New\tCtrl+N");
  AppendMenu(filemenu, MF_STRING, 1002, "Open\tCtrl+O");
  AppendMenu(filemenu, MF_STRING, 1003, "Save\tCtrl+S");
  AppendMenu(filemenu, MF_SEPARATOR, 0, nullptr);
  AppendMenu(filemenu, MF_STRING, 1004, "Export SVG");
  AppendMenu(filemenu, MF_STRING, 1005, "Export JSON");
  AppendMenu(filemenu, MF_SEPARATOR, 0, nullptr);
  AppendMenu(filemenu, MF_STRING, 1006, "Exit");
  AppendMenu(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(filemenu), "File");

  HMENU editmenu = CreatePopupMenu();
  AppendMenu(editmenu, MF_STRING, 2001, "Undo\tCtrl+Z");
  AppendMenu(editmenu, MF_STRING, 2002, "Delete Selected\tDel");
  AppendMenu(editmenu, MF_STRING, 2003, "Clear All");
  AppendMenu(editmenu, MF_STRING, 2004, "Close Loop");
  AppendMenu(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(editmenu), "Edit");

  HMENU toolmenu = CreatePopupMenu();
  AppendMenu(toolmenu, MF_STRING, 3001, "Select (V)");
  AppendMenu(toolmenu, MF_STRING, 3002, "Vertex (A)");
  AppendMenu(toolmenu, MF_STRING, 3003, "Straight (S)");
  AppendMenu(toolmenu, MF_STRING, 3004, "Left Curve (L)");
  AppendMenu(toolmenu, MF_STRING, 3005, "Right Curve (R)");
  AppendMenu(toolmenu, MF_STRING, 3006, "Pit Box (P)");
  AppendMenu(toolmenu, MF_STRING, 3007, "Start/Finish (F)");
  AppendMenu(toolmenu, MF_STRING, 3008, "Barrier (B)");
  AppendMenu(toolmenu, MF_STRING, 3009, "Eraser (E)");
  AppendMenu(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(toolmenu), "Tools");

  SetMenu(hwnd, menubar);
  mem_dc_ = CreateCompatibleDC(nullptr);
  return 0;
}

LRESULT BlueprintEditor::handle_size(HWND, int width, int height) {
  if (mem_bitmap_) { DeleteObject(mem_bitmap_); mem_bitmap_ = nullptr; }
  if (mem_dc_ && width > 0 && height > 0) {
    mem_bitmap_ = CreateCompatibleBitmap(GetDC(window_), width, height);
    SelectObject(mem_dc_, mem_bitmap_);
  }
  config_.width = width; config_.height = height;
  InvalidateRect(window_, nullptr, TRUE);
  return 0;
}
LRESULT BlueprintEditor::handle_paint(HWND hwnd) {
  PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
  if (mem_bitmap_ && mem_dc_) {
    PatBlt(mem_dc_, 0, 0, config_.width, config_.height, BLACKNESS);
    draw_grid(mem_dc_);
    if (config_.show_ruler) draw_ruler(mem_dc_);
    draw_track_preview(mem_dc_);
    draw_elements(mem_dc_);
    draw_selection(mem_dc_);
    draw_hud(mem_dc_);
    BitBlt(hdc, 0, 0, config_.width, config_.height, mem_dc_, 0, 0, SRCCOPY);
  }
  EndPaint(hwnd, &ps); return 0;
}

LRESULT BlueprintEditor::handle_lbutton_down(HWND hwnd, int x, int y) {
  SetCapture(hwnd); dragging_ = true;
  Vec2 world = screen_to_world(x, y); mouse_world_ = world;
  switch (current_tool_) {
    case Tool::Vertex: add_vertex(snap_to_grid(world)); break;
    case Tool::Straight: {
      if (selected_index_ >= 0 && selected_index_ < (int)track_.elements.size()) {
        auto& el = track_.elements[selected_index_];
        if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight)
          add_straight(el.position, snap_to_grid(world));
      } break;
    }
    case Tool::LeftCurve:
    case Tool::RightCurve: {
      if (selected_index_ >= 0 && selected_index_ < (int)track_.elements.size()) {
        auto& el = track_.elements[selected_index_];
        if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight) {
          Vec2 center = el.position;
          double radius = (world - center).norm();
          if (radius > kMinSegmentLength)
            add_curve(center, radius, kHalfPi, current_tool_ == Tool::LeftCurve);
        }
      } break;
    }
    case Tool::PitBox: add_pit_box(snap_to_grid(world)); break;
    case Tool::StartFinish: set_start_finish(snap_to_grid(world)); break;
    case Tool::Barrier: {
      if (selected_index_ >= 0 && selected_index_ < (int)track_.elements.size()) {
        auto& el = track_.elements[selected_index_];
        if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight)
          add_barrier(el.position, snap_to_grid(world));
      } break;
    }
    case Tool::Eraser: {
      double min_dist = kSnapDistance / config_.scale;
      for (int i = (int)track_.elements.size() - 1; i >= 0; --i) {
        if ((track_.elements[i].position - world).norm() < min_dist) {
          track_.elements.erase(track_.elements.begin() + i);
          selected_index_ = -1; recompute_track(); break;
        }
      } break;
    }
    case Tool::Select: {
      double min_dist = kSnapDistance / config_.scale;
      int closest = -1; double closest_dist = min_dist;
      for (int i = 0; i < (int)track_.elements.size(); ++i) {
        double d = (track_.elements[i].position - world).norm();
        if (d < closest_dist) { closest_dist = d; closest = i; }
      }
      selected_index_ = closest; break;
    }
    default: break;
  }
  InvalidateRect(hwnd, nullptr, FALSE); return 0;
}
LRESULT BlueprintEditor::handle_lbutton_up(HWND, int, int) {
  dragging_ = false; ReleaseCapture(); return 0;
}

LRESULT BlueprintEditor::handle_rbutton_down(HWND, int, int) {
  if (selected_index_ >= 0) { selected_index_ = -1; InvalidateRect(window_, nullptr, FALSE); }
  return 0;
}

LRESULT BlueprintEditor::handle_mouse_move(HWND hwnd, int x, int y) {
  mouse_world_ = screen_to_world(x, y);
  if (dragging_ && current_tool_ == Tool::Select && selected_index_ >= 0) {
    track_.elements[selected_index_].position = snap_to_grid(mouse_world_);
    recompute_track();
  }
  if (dragging_ && current_tool_ == Tool::Barrier && selected_index_ >= 0) {
    auto& el = track_.elements[selected_index_];
    if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight) {
      Vec2 end = snap_to_grid(mouse_world_);
      double len = (end - el.position).norm();
      if (len > kMinSegmentLength) {
        BlueprintElement barrier;
        barrier.type = ElementType::Barrier;
        barrier.position = el.position;
        barrier.tangent = (end - el.position).normalized();
        barrier.length = len;
        track_.elements.push_back(barrier);
        selected_index_ = (int)track_.elements.size() - 1;
        recompute_track();
      }
    }
  }
  InvalidateRect(hwnd, nullptr, FALSE); return 0;
}

LRESULT BlueprintEditor::handle_mouse_wheel(HWND, int delta) { {
  double factor = (delta > 0) ? 1.1 : 0.9;
  config_.scale = std::max(0.5, std::min(50.0, config_.scale * factor));
  InvalidateRect(window_, nullptr, FALSE); return 0;
}
LRESULT BlueprintEditor::handle_key_down(HWND, WPARAM vk) {
  switch (vk) {
    case 'V': current_tool_ = Tool::Select; tool_name_ = "Select"; break;
    case 'A': current_tool_ = Tool::Vertex; tool_name_ = "Vertex"; break;
    case 'S': current_tool_ = Tool::Straight; tool_name_ = "Straight"; break;
    case 'L': current_tool_ = Tool::LeftCurve; tool_name_ = "Left Curve"; break;
    case 'R': current_tool_ = Tool::RightCurve; tool_name_ = "Right Curve"; break;
    case 'P': current_tool_ = Tool::PitBox; tool_name_ = "Pit Box"; break;
    case 'F': current_tool_ = Tool::StartFinish; tool_name_ = "Start/Finish"; break;
    case 'B': current_tool_ = Tool::Barrier; tool_name_ = "Barrier"; break;
    case 'E': current_tool_ = Tool::Eraser; tool_name_ = "Eraser"; break;
    case VK_DELETE: delete_selected(); break;
    case VK_ESCAPE: selected_index_ = -1; break;
    case 'C': close_loop(); break;
    case 'G': config_.show_grid = !config_.show_grid; break;
    case 'N': clear_track(); break;
    case 'Z':
      if (!track_.elements.empty()) { track_.elements.pop_back(); recompute_track(); }
      break;
    default: break;
  }
  InvalidateRect(window_, nullptr, FALSE); return 0;
}

LRESULT BlueprintEditor::handle_command(HWND hwnd, WPARAM wp) {
  switch (LOWORD(wp)) {
    case 1001: clear_track(); break;
    case 1003: case 1005: {
      OPENFILENAME ofn = {}; char filename[MAX_PATH] = {};
      ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
      ofn.lpstrFilter = (LOWORD(wp) == 1003)
        ? "JSON Files\0*.json\0All Files\0*.*\0"
        : "SVG Files\0*.svg\0All Files\0*.*\0";
      ofn.lpstrFile = filename; ofn.nMaxFile = MAX_PATH;
      ofn.Flags = OFN_OVERWRITEPROMPT;
      if (GetSaveFileName(&ofn)) {
        if (LOWORD(wp) == 1003) export_json(filename);
        else export_svg(filename);
      }
      break;
    }
    case 1006: PostMessage(hwnd, WM_CLOSE, 0, 0); break;
    case 2002: delete_selected(); break;
    case 2003: clear_track(); break;
    case 2004: close_loop(); break;
  }
  InvalidateRect(hwnd, nullptr, FALSE); return 0;
}

LRESULT BlueprintEditor::handle_close(HWND) {
  running_ = false; DestroyWindow(window_); return 0;
}
void BlueprintEditor::draw_grid(HDC hdc) {
  int w = config_.width, h = config_.height;
  for (double gx = std::floor(config_.offset.x() / kGridSize) * kGridSize;
       gx < w / config_.scale + config_.offset.x(); gx += kGridSize) {
    int sx = world_to_screen_x(gx);
    bool major = (std::fmod(std::round(gx), kMajorGrid) < kEpsilon);
    HPEN pen = CreatePen(PS_SOLID, 1, major ? kMajorGridColor : kGridColor);
    HGDIOBJ old = SelectObject(hdc, pen);
    MoveToEx(hdc, sx, 0, nullptr); LineTo(hdc, sx, h);
    SelectObject(hdc, old); DeleteObject(pen);
  }
  for (double gy = std::floor(config_.offset.y() / kGridSize) * kGridSize;
       gy < h / config_.scale + config_.offset.y(); gy += kGridSize) {
    int sy = world_to_screen_y(gy);
    bool major = (std::fmod(std::round(gy), kMajorGrid) < kEpsilon);
    HPEN pen = CreatePen(PS_SOLID, 1, major ? kMajorGridColor : kGridColor);
    HGDIOBJ old = SelectObject(hdc, pen);
    MoveToEx(hdc, 0, sy, nullptr); LineTo(hdc, w, sy);
    SelectObject(hdc, old); DeleteObject(pen);
  }
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
  HPEN axis_pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 100));
  SelectObject(hdc, axis_pen);
  int origin_x = world_to_screen_x(0.0), origin_y = world_to_screen_y(0.0);
  MoveToEx(hdc, origin_x, 0, nullptr); LineTo(hdc, origin_x, h);
  MoveToEx(hdc, 0, origin_y, nullptr); LineTo(hdc, w, origin_y);
  SelectObject(hdc, old_brush); DeleteObject(axis_pen);
}

void BlueprintEditor::draw_ruler(HDC hdc) {
  int ruler_h = 24, ruler_w = 60;
  HBRUSH bg = CreateSolidBrush(kRulerBg);
  HBRUSH old_brush = SelectObject(hdc, bg);
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 60));
  HGDIOBJ old_pen = SelectObject(hdc, pen);
  Rectangle(hdc, 0, 0, config_.width, ruler_h);
  Rectangle(hdc, 0, 0, ruler_w, config_.height);
  SelectObject(hdc, old_pen); DeleteObject(pen);
  DeleteObject(bg); SelectObject(hdc, old_brush);

  SetTextColor(hdc, kRulerText); SetBkMode(hdc, TRANSPARENT);
  HFONT font = CreateFontA(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, FIXED_PITCH, "Consolas");
  HGDIOBJ old_font = SelectObject(hdc, font);

  double step = 50.0;
  for (double wx = std::floor(config_.offset.x() / step) * step;
       wx < config_.width / config_.scale + config_.offset.x(); wx += step) {
    int sx = world_to_screen_x(wx);
    if (sx > ruler_w && sx < config_.width) {
      char buf[32]; std::snprintf(buf, sizeof(buf), "%.0f", wx);
      TextOutA(hdc, sx - 15, 4, buf, (int)std::strlen(buf));
      MoveToEx(hdc, sx, ruler_h - 4, nullptr); LineTo(hdc, sx, ruler_h);
    }
  }
  for (double wy = std::floor(config_.offset.y() / step) * step;
       wy < config_.height / config_.scale + config_.offset.y(); wy += step) {
    int sy = world_to_screen_y(wy);
    if (sy > ruler_h && sy < config_.height) {
      char buf[32]; std::snprintf(buf, sizeof(buf), "%.0f", wy);
      TextOutA(hdc, 4, sy - 6, buf, (int)std::strlen(buf));
      MoveToEx(hdc, ruler_w - 4, sy, nullptr); LineTo(hdc, ruler_w, sy);
    }
  }
  SelectObject(hdc, old_font); DeleteObject(font);
}
void BlueprintEditor::draw_track_preview(HDC hdc) {
  std::vector<Vec2> centerline;
  for (const auto& el : track_.elements) {
    if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight ||
        el.type == ElementType::LeftCurve || el.type == ElementType::RightCurve)
      centerline.push_back(el.position);
  }
  if (centerline.empty() || centerline.size() < 2) return;

  std::vector<Vec2> left_edge, right_edge;
  left_edge.reserve(centerline.size());
  right_edge.reserve(centerline.size());

  for (size_t i = 0; i < centerline.size(); ++i) {
    Vec2 tangent;
    if (i < centerline.size() - 1) tangent = (centerline[i + 1] - centerline[i]).normalized();
    else if (centerline.size() > 1) tangent = (centerline[i] - centerline[i - 1]).normalized();
    else tangent = Vec2(1.0, 0.0);
    Vec2 normal(-tangent.y(), tangent.x());
    double w = track_.track_width * 0.5;
    left_edge.push_back(centerline[i] + normal * w);
    right_edge.push_back(centerline[i] - normal * w);
  }

  HBRUSH fill_brush = CreateSolidBrush(kTrackFillColor);
  HGDIOBJ old_brush = SelectObject(hdc, fill_brush);
  HPEN edge_pen = CreatePen(PS_SOLID, 2, kTrackEdgeColor);
  HGDIOBJ old_pen = SelectObject(hdc, edge_pen);

  BeginPath(hdc);
  for (size_t i = 0; i < left_edge.size(); ++i) {
    int sx = world_to_screen_x(left_edge[i].x()), sy = world_to_screen_y(left_edge[i].y());
    if (i == 0) MoveToEx(hdc, sx, sy, nullptr); else LineTo(hdc, sx, sy);
  }
  for (int i = (int)right_edge.size() - 1; i >= 0; --i) {
    int sx = world_to_screen_x(right_edge[i].x()), sy = world_to_screen_y(right_edge[i].y());
    LineTo(hdc, sx, sy);
  }
  CloseFigure(hdc); EndPath(hdc); FillPath(hdc);

  BeginPath(hdc);
  for (size_t i = 0; i < left_edge.size(); ++i) {
    int sx = world_to_screen_x(left_edge[i].x()), sy = world_to_screen_y(left_edge[i].y());
    if (i == 0) MoveToEx(hdc, sx, sy, nullptr); else LineTo(hdc, sx, sy);
  }
  EndPath(hdc); StrokePath(hdc);

  BeginPath(hdc);
  for (int i = (int)right_edge.size() - 1; i >= 0; --i) {
    int sx = world_to_screen_x(right_edge[i].x()), sy = world_to_screen_y(right_edge[i].y());
    if (i == (int)right_edge.size() - 1) MoveToEx(hdc, sx, sy, nullptr);
    else LineTo(hdc, sx, sy);
  }
  EndPath(hdc); StrokePath(hdc);

  HPEN center_pen = CreatePen(PS_SOLID, 2, kCenterlineColor);
  SelectObject(hdc, center_pen);
  BeginPath(hdc);
  for (size_t i = 0; i < centerline.size(); ++i) {
    int sx = world_to_screen_x(centerline[i].x()), sy = world_to_screen_y(centerline[i].y());
    if (i == 0) MoveToEx(hdc, sx, sy, nullptr); else LineTo(hdc, sx, sy);
  }
  EndPath(hdc); StrokePath(hdc);

  SelectObject(hdc, old_pen);
  DeleteObject(edge_pen); DeleteObject(center_pen);
  SelectObject(hdc, old_brush); DeleteObject(fill_brush);
}
void BlueprintEditor::draw_elements(HDC hdc) {
  for (const auto& el : track_.elements) {
    int sx = world_to_screen_x(el.position.x()), sy = world_to_screen_y(el.position.y());
    switch (el.type) {
      case ElementType::PitBox: {
        HPEN pen = CreatePen(PS_SOLID, 1, kPitBoxColor);
        HBRUSH brush = CreateSolidBrush(RGB(160, 160, 160));
        HGDIOBJ old_pen = SelectObject(hdc, pen), old_brush = SelectObject(hdc, brush);
        Rectangle(hdc, sx - 12, sy - 6, sx + 12, sy + 6);
        SelectObject(hdc, old_pen); SelectObject(hdc, old_brush);
        DeleteObject(pen); DeleteObject(brush);
        char buf[32]; std::snprintf(buf, sizeof(buf), "P%d", el.pit_box_index);
        SetTextColor(hdc, kTextColor); SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, sx - 8, sy - 4, buf, (int)std::strlen(buf));
        break;
      }
      case ElementType::StartFinish: {
        HPEN pen = CreatePen(PS_SOLID, 3, kStartFinishColor);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, sx - 20, sy - 10, nullptr); LineTo(hdc, sx + 20, sy + 10);
        MoveToEx(hdc, sx - 20, sy + 10, nullptr); LineTo(hdc, sx + 20, sy - 10);
        SelectObject(hdc, old_pen); DeleteObject(pen);
        SetTextColor(hdc, kStartFinishColor); SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, sx - 20, sy - 20, "S/F", 3);
        break;
      }
      case ElementType::Barrier: {
        Vec2 end = el.position + el.tangent * el.length;
        int ex = world_to_screen_x(end.x()), ey = world_to_screen_y(end.y());
        HPEN pen = CreatePen(PS_SOLID, 3, kBarrierColor);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, sx, sy, nullptr); LineTo(hdc, ex, ey);
        SelectObject(hdc, old_pen); DeleteObject(pen);
        break;
      }
      default: break;
    }
  }
}
void BlueprintEditor::draw_selection(HDC hdc) {
  if (selected_index_ >= 0 && selected_index_ < (int)track_.elements.size()) {
    const auto& el = track_.elements[selected_index_];
    int sx = world_to_screen_x(el.position.x()), sy = world_to_screen_y(el.position.y());
    HPEN pen = CreatePen(PS_SOLID, 2, kSelectedColor);
    HGDIOBJ old_pen = SelectObject(hdc, pen), old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, sx - kVertexRadius - 3, sy - kVertexRadius - 3,
            sx + kVertexRadius + 3, sy + kVertexRadius + 3);
    SelectObject(hdc, old_pen); SelectObject(hdc, old_brush); DeleteObject(pen);
    char buf[256];
    std::snprintf(buf, sizeof(buf), "X: %.1f  Y: %.1f  W: %.1f",
      el.position.x(), el.position.y(), el.width);
    SetTextColor(hdc, kSelectedColor); SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, sx + 12, sy - 8, buf, (int)std::strlen(buf));
  }
  if (current_tool_ == Tool::Vertex || current_tool_ == Tool::Select) {
    int sx = world_to_screen_x(mouse_world_.x()), sy = world_to_screen_y(mouse_world_.y());
    HPEN pen = CreatePen(PS_DOT, 1, RGB(100, 100, 120));
    HGDIOBJ old_pen = SelectObject(hdc, pen), old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, sx - kVertexRadius, sy - kVertexRadius, sx + kVertexRadius, sy + kVertexRadius);
    SelectObject(hdc, old_pen); SelectObject(hdc, old_brush); DeleteObject(pen);
  }
}

void BlueprintEditor::draw_hud(HDC hdc) {
  HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, FIXED_PITCH, "Consolas");
  HGDIOBJ old_font = SelectObject(hdc, font);
  SetTextColor(hdc, kTextColor); SetBkMode(hdc, TRANSPARENT);
  int y = config_.height - 90; char buf[256];
  std::snprintf(buf, sizeof(buf), "Tool: %s", tool_name_.c_str());
  TextOutA(hdc, 70, y, buf, (int)std::strlen(buf)); y += 18;
  std::snprintf(buf, sizeof(buf), "Zoom: %.1fx  Grid: %sm",
    config_.scale, config_.snap_to_grid ? "10m" : "off");
  TextOutA(hdc, 70, y, buf, (int)std::strlen(buf)); y += 18;
  std::snprintf(buf, sizeof(buf), "Mouse: (%.1f, %.1f)", mouse_world_.x(), mouse_world_.y());
  TextOutA(hdc, 70, y, buf, (int)std::strlen(buf)); y += 18;
  std::snprintf(buf, sizeof(buf), "Elements: %d  Selected: %d",
    (int)track_.elements.size(), selected_index_);
  TextOutA(hdc, 70, y, buf, (int)std::strlen(buf)); y += 18;
  if (!status_text_.empty()) {
    std::snprintf(buf, sizeof(buf), "%s", status_text_.c_str());
    TextOutA(hdc, 70, y, buf, (int)std::strlen(buf));
  }
  std::snprintf(buf, sizeof(buf), "%s - %s", track_.track_id.c_str(), track_.track_name.c_str());
  TextOutA(hdc, config_.width - 250, config_.height - 20, buf, (int)std::strlen(buf));
  SelectObject(hdc, old_font); DeleteObject(font);
}
Vec2 BlueprintEditor::screen_to_world(int sx, int sy) const {
  return Vec2((sx - config_.offset.x()) / config_.scale,
              -(sy - config_.offset.y()) / config_.scale);
}

Vec2 BlueprintEditor::snap_to_grid(const Vec2& pos) const {
  if (!config_.snap_to_grid) return pos;
  double snap = kGridSize;
  return Vec2(std::round(pos.x() / snap) * snap, std::round(pos.y() / snap) * snap);
}

int BlueprintEditor::world_to_screen_x(double wx) const {
  return (int)(wx * config_.scale + config_.offset.x());
}

int BlueprintEditor::world_to_screen_y(double wy) const {
  return (int)(-wy * config_.scale + config_.offset.y());
}

void BlueprintEditor::add_vertex(const Vec2& pos) {
  BlueprintElement el; el.type = ElementType::TrackVertex; el.position = pos;
  if (track_.elements.empty()) track_.start_position = pos;
  track_.elements.push_back(el);
  status_text_ = "Added vertex at (" + std::to_string((int)pos.x()) +
                 ", " + std::to_string((int)pos.y()) + ")";
}

void BlueprintEditor::add_straight(const Vec2& from, const Vec2& to) {
  BlueprintElement start_el; start_el.type = ElementType::Straight;
  start_el.position = from; start_el.tangent = (to - from).normalized();
  start_el.length = (to - from).norm(); start_el.width = track_.track_width;
  BlueprintElement end_el; end_el.type = ElementType::TrackVertex; end_el.position = to;
  track_.elements.push_back(start_el); track_.elements.push_back(end_el);
  selected_index_ = (int)track_.elements.size() - 1;
  status_text_ = "Added straight: " + std::to_string((int)start_el.length) + "m";
}
void BlueprintEditor::add_curve(const Vec2& center, double radius, double arc_angle, bool left) {
  Vec2 start = track_.elements.empty() ? center + Vec2(0.0, -radius)
                                       : track_.elements.back().position;
  double start_angle = std::atan2(start.y() - center.y(), start.x() - center.x());
  int steps = std::max(4, (int)(std::abs(arc_angle) / (kPi / 36.0)));
  for (int i = 1; i <= steps; ++i) {
    double t = (double)i / steps;
    double angle = start_angle + t * (left ? arc_angle : -arc_angle);
    Vec2 p = center + Vec2(radius * std::cos(angle), radius * std::sin(angle));
    BlueprintElement vert; vert.type = ElementType::TrackVertex; vert.position = p;
    track_.elements.push_back(vert);
  }
  selected_index_ = (int)track_.elements.size() - 1;
  status_text_ = "Added " + std::string(left ? "left" : "right") +
                 " curve: R=" + std::to_string((int)radius) + "m";
}

void BlueprintEditor::add_pit_box(const Vec2& pos) {
  BlueprintElement el; el.type = ElementType::PitBox; el.position = pos;
  el.pit_box_index = (int)track_.pit_box_positions.size();
  track_.elements.push_back(el); track_.pit_box_positions.push_back(pos);
  status_text_ = "Added pit box " + std::to_string(el.pit_box_index);
}

void BlueprintEditor::set_start_finish(const Vec2& pos) {
  for (auto& el : track_.elements) {
    if (el.type == ElementType::StartFinish) {
      el.position = pos; track_.start_position = pos;
      status_text_ = "Moved start/finish"; return;
    }
  }
  BlueprintElement el; el.type = ElementType::StartFinish; el.position = pos;
  track_.elements.push_back(el); track_.start_position = pos;
  status_text_ = "Set start/finish line";
}

void BlueprintEditor::add_barrier(const Vec2& from, const Vec2& to) {
  BlueprintElement el; el.type = ElementType::Barrier; el.position = from;
  el.tangent = (to - from).normalized(); el.length = (to - from).norm();
  track_.elements.push_back(el);
  selected_index_ = (int)track_.elements.size() - 1;
  status_text_ = "Added barrier: " + std::to_string((int)el.length) + "m";
}

void BlueprintEditor::recompute_track() {
  track_.pit_box_positions.clear();
  for (const auto& el : track_.elements)
    if (el.type == ElementType::PitBox) track_.pit_box_positions.push_back(el.position);
  if (!track_.elements.empty()) track_.start_position = track_.elements[0].position;
}
void BlueprintEditor::close_loop() {
  if (track_.elements.size() >= 2) {
    const auto& first = track_.elements[0];
    const auto& last = track_.elements.back();
    if ((last.position - first.position).norm() > kMinSegmentLength) {
      BlueprintElement el; el.type = ElementType::Straight; el.position = last.position;
      el.tangent = (first.position - last.position).normalized();
      el.length = (first.position - last.position).norm(); el.width = track_.track_width;
      track_.elements.push_back(el);
      status_text_ = "Loop closed";
    } else { status_text_ = "Already closed"; }
  }
  InvalidateRect(window_, nullptr, FALSE);
}

void BlueprintEditor::delete_selected() {
  if (selected_index_ >= 0 && selected_index_ < (int)track_.elements.size()) {
    track_.elements.erase(track_.elements.begin() + selected_index_);
    selected_index_ = -1; recompute_track();
    InvalidateRect(window_, nullptr, FALSE);
  }
}

void BlueprintEditor::clear_track() {
  track_.elements.clear(); track_.pit_box_positions.clear();
  selected_index_ = -1; status_text_ = "Track cleared";
  InvalidateRect(window_, nullptr, FALSE);
}

std::string BlueprintEditor::element_type_name(ElementType type) const {
  switch (type) {
    case ElementType::TrackVertex: return "vertex";
    case ElementType::Straight: return "straight";
    case ElementType::LeftCurve: return "left_curve";
    case ElementType::RightCurve: return "right_curve";
    case ElementType::PitBox: return "pit_box";
    case ElementType::StartFinish: return "start_finish";
    case ElementType::Barrier: return "barrier";
    case ElementType::Kerb: return "kerb";
    case ElementType::SurfaceChange: return "surface_change";
    default: return "unknown";
  }
}
bool BlueprintEditor::export_json(const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << "{\n";
  file << "  \"track_id\": \"" << track_.track_id << "\",\n";
  file << "  \"track_name\": \"" << track_.track_name << "\",\n";
  file << "  \"width_m\": " << track_.track_width << ",\n";
  file << "  \"start_position\": {\"x\": " << std::fixed << std::setprecision(2)
       << track_.start_position.x() << ", \"y\": " << track_.start_position.y() << "},\n";
  file << "  \"elements\": [\n";
  for (size_t i = 0; i < track_.elements.size(); ++i) {
    const auto& el = track_.elements[i];
    file << "    {\"type\": \"" << element_type_name(el.type) << "\", ";
    file << "\"position\": {\"x\": " << std::fixed << std::setprecision(2)
         << el.position.x() << ", \"y\": " << el.position.y() << "}";
    if (el.type == ElementType::Straight || el.type == ElementType::Barrier) {
      file << ", \"tangent\": {\"x\": " << el.tangent.x() << ", \"y\": " << el.tangent.y() << "}";
      file << ", \"length\": " << el.length;
    }
    if (el.type == ElementType::Barrier) file << ", \"width\": " << el.width;
    if (el.type == ElementType::PitBox) file << ", \"index\": " << el.pit_box_index;
    file << "}";
    if (i + 1 < track_.elements.size()) file << ",";
    file << "\n";
  }
  file << "  ]\n}\n";
  file.close();
  std::cout << "Blueprint exported to: " << path << std::endl;
  return true;
}

bool BlueprintEditor::export_svg(const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << "<?xml version=""1.0"" encoding=""UTF-8""?>\n";
  file << "<svg xmlns=""http://www.w3.org/2000/svg"" width=""1200"" height=""800"">\n";
  file << "  <rect width=""100%"" height=""100%"" fill=""#1a1a1a""/>\n";
  file << "  <text x=""20"" y=""30"" fill=""#888"" font-family=""monospace"" font-size=""14"">";
  file << track_.track_name << " - Blueprint</text>\n";
  std::vector<Vec2> centerline;
  for (const auto& el : track_.elements)
    if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight ||
        el.type == ElementType::LeftCurve || el.type == ElementType::RightCurve)
      centerline.push_back(el.position);
  if (centerline.size() >= 2) {
    file << "  <polyline points=""";
    for (size_t i = 0; i < centerline.size(); ++i) {
      double sx = centerline[i].x() * 3.0 + 600.0;
      double sy = -centerline[i].y() * 3.0 + 400.0;
      file << sx << "," << sy << " ";
    }
    file << """ fill=""none"" stroke=""#ffff00"" stroke-width=""3""/>\n";
  }
  for (const auto& el : track_.elements) {
    double sx = el.position.x() * 3.0 + 600.0;
    double sy = -el.position.y() * 3.0 + 400.0;
    if (el.type == ElementType::PitBox) {
      file << "  <rect x=""" << sx - 12 << """ y=""" << sy - 6
           << """ width=""24"" height=""12"" fill=""#888"" stroke=""#666""/>\n";
    } else if (el.type == ElementType::StartFinish) {
      file << "  <line x1=""" << sx - 20 << """ y1=""" << sy - 10
           << """ x2=""" << sx + 20 << """ y2=""" << sy + 10
           << """ stroke=""#FFD700"" stroke-width=""3""/>\n";
    } else if (el.type == ElementType::Barrier) {
      Vec2 end = el.position + el.tangent * el.length;
      double ex = end.x() * 3.0 + 600.0, ey = -end.y() * 3.0 + 400.0;
      file << "  <line x1=""" << sx << """ y1=""" << sy << """ x2=""" << ex
           << """ y2=""" << ey << """ stroke=""#ff8c00"" stroke-width=""3""/>\n";
    }
  }
  file << "</svg>\n";
  file.close();
  std::cout << "SVG blueprint exported to: " << path << std::endl;
  return true;
}
int BlueprintEditor::run() {
  MSG msg;
  while (running_ && GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return (int)msg.wParam;
}

}
