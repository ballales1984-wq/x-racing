import os

content = r'''#include "track_blueprint.h"
#include <commctrl.h>
#include <commdlg.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

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
      case WM_SIZE: return editor->handle_size(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_LBUTTONDOWN: return editor->handle_lbutton_down(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_LBUTTONUP: return editor->handle_lbutton_up(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_RBUTTONDOWN: return editor->handle_rbutton_down(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_MOUSEMOVE: return editor->handle_mouse_move(hwnd, LOWORD(lp), HIWORD(lp));
      case WM_MOUSEWHEEL: return editor->handle_mouse_wheel(hwnd, GET_WHEEL_DELTA_WPARAM(wp));
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

LRESULT BlueprintEditor::handle_mouse_wheel(HWND, int delta) {
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
'''

with open(r'D:\x-racing\experiments\track_blueprint_part1.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print('Part 1 written')
