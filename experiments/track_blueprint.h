#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

#define NOMINMAX
#include <windows.h>

namespace p0::track_blueprint {

enum class ElementType : uint8_t {
  TrackVertex = 0,
  Straight,
  LeftCurve,
  RightCurve,
  PitBox,
  StartFinish,
  Barrier,
  Kerb,
  SurfaceChange,
  Count
};

enum class SurfaceType : uint8_t {
  Asphalt = 0,
  WetAsphalt,
  OldAsphalt,
  Kerb,
  Grass,
  Gravel,
  Dirt,
  Sand,
  Count
};

inline double friction_for_surface(SurfaceType type) {
  switch (type) {
    case SurfaceType::Asphalt:    return 1.00;
    case SurfaceType::WetAsphalt: return 0.70;
    case SurfaceType::OldAsphalt: return 0.85;
    case SurfaceType::Kerb:       return 0.65;
    case SurfaceType::Grass:      return 0.30;
    case SurfaceType::Gravel:     return 0.40;
    case SurfaceType::Dirt:       return 0.50;
    case SurfaceType::Sand:       return 0.25;
    default:                      return 1.00;
  }
}

struct Vec2Color {
  Vec2 position;
  uint8_t r, g, b;
};

inline const Vec2Color& color_for_surface(SurfaceType type) {
  static const Vec2Color colors[] = {
    {{0,0}, 100, 100, 100},
    {{0,0},  78,  78, 106},
    {{0,0},  90,  74,  66},
    {{0,0}, 255, 136,   0},
    {{0,0},  74, 154,  74},
    {{0,0}, 210, 180, 140},
    {{0,0}, 139,  69,  19},
    {{0,0}, 255, 210, 127},
  };
  int idx = static_cast<int>(type);
  int count = static_cast<int>(sizeof(colors)/sizeof(colors[0]));
  if (idx < 0 || idx >= count) idx = 0;
  return colors[idx];
}

struct BlueprintElement {
  ElementType type = ElementType::TrackVertex;
  Vec2 position{0.0, 0.0};
  Vec2 tangent{1.0, 0.0};
  double radius = 50.0;
  double arc_angle = kHalfPi;
  double width = 12.0;
  double length = 100.0;
  SurfaceType surface = SurfaceType::Asphalt;
  int pit_box_index = 0;
  bool has_box_lane = false;
  double box_lane_width = 3.5;
};

class TrackBuilder {
 public:
  TrackBuilder();

  void set_position(const Vec2& pos) { current_pos_ = pos; }
  void set_heading(double heading) { current_heading_ = heading; }
  void set_width(double width) { track_width_ = width; }

  Vec2 position() const { return current_pos_; }
  double heading() const { return current_heading_; }
  Vec2 tangent() const { return Vec2(std::cos(current_heading_), std::sin(current_heading_)); }

  BlueprintElement add_straight(double length);
  BlueprintElement add_curve(double radius, double arc_angle, bool left);
  std::vector<BlueprintElement> add_curve_samples(double radius, double arc_angle, bool left, int steps);

  static Vec2 center_for_curve(const Vec2& pos, double heading, double radius, bool left);
  static Vec2 end_tangent_for_curve(double heading, double arc_angle, bool left);

 private:
  Vec2 current_pos_{0.0, 0.0};
  double current_heading_ = 0.0;
  double track_width_ = 12.0;
};

struct BlueprintTrack {
  std::string track_id;
  std::string track_name;
  double track_width = 12.0;
  std::vector<BlueprintElement> elements;
  std::vector<Vec2> pit_box_positions;
  Vec2 start_position{0.0, 0.0};
  double start_heading = 0.0;
};

class BlueprintEditor {
 public:
  explicit BlueprintEditor(const std::string& title = "X-Racing Track Blueprint Editor");
  ~BlueprintEditor();

  bool initialize();
  int run();

  void set_track(const BlueprintTrack& track) { track_ = track; }
  const BlueprintTrack& track() const { return track_; }
  bool export_json(const std::string& path);
  bool export_svg(const std::string& path);

 private:
  static constexpr int kDefaultWidth = 1400;
  static constexpr int kDefaultHeight = 900;
  static constexpr double kGridSize = 10.0;
  static constexpr double kMajorGrid = 50.0;
  static constexpr double kSnapDistance = 5.0;
  static constexpr double kVertexRadius = 6.0;
  static constexpr double kMinSegmentLength = 5.0;

  static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT handle_create(HWND hwnd);
  LRESULT handle_paint(HWND hwnd);
  LRESULT handle_size(HWND hwnd, int width, int height);
  LRESULT handle_lbutton_down(HWND hwnd, int x, int y);
  LRESULT handle_lbutton_up(HWND hwnd, int x, int y);
  LRESULT handle_rbutton_down(HWND hwnd, int x, int y);
  LRESULT handle_mouse_move(HWND hwnd, int x, int y);
  LRESULT handle_mouse_wheel(HWND hwnd, int delta);
  LRESULT handle_key_down(HWND hwnd, WPARAM vk);
  LRESULT handle_command(HWND hwnd, WPARAM wp);
  LRESULT handle_close(HWND hwnd);

  void draw_grid(HDC hdc);
  void draw_track_preview(HDC hdc);
  void draw_elements(HDC hdc);
  void draw_selection(HDC hdc);
  void draw_hud(HDC hdc);
  void draw_ruler(HDC hdc);

  Vec2 screen_to_world(int sx, int sy) const;
  Vec2 snap_to_grid(const Vec2& pos) const;
  int world_to_screen_x(double wx) const;
  int world_to_screen_y(double wy) const;

  void add_vertex(const Vec2& pos);
  void add_straight(const Vec2& from, const Vec2& to);
  void add_straight_from_last(double length);
  void add_curve(const Vec2& center, double radius, double arc_angle, bool left);
  void add_curve_from_last(double radius, double arc_angle, bool left);
  void add_pit_box(const Vec2& pos);
  void set_start_finish(const Vec2& pos);
  void add_barrier(const Vec2& from, const Vec2& to);
  void recompute_track();

  void close_loop();
  void close_loop_with_curve(bool left);
  void delete_selected();
  void clear_track();

  std::string element_type_name(ElementType type) const;

  Vec2 get_last_position() const;
  Vec2 get_last_tangent() const;
  double get_last_heading() const;

  struct EditorConfig {
    int width = kDefaultWidth;
    int height = kDefaultHeight;
    double scale = 5.0;
    Vec2 offset{0.0, 0.0};
    bool show_grid = true;
    bool show_ruler = true;
    bool snap_to_grid = true;
  };

  EditorConfig config_;
  BlueprintTrack track_;
  HWND window_ = nullptr;
  HDC mem_dc_ = nullptr;
  HBITMAP mem_bitmap_ = nullptr;
  bool running_ = false;
  bool dragging_ = false;
  bool panning_ = false;
  int selected_index_ = -1;
  int hover_index_ = -1;
  Vec2 last_mouse_world_;
  Vec2 drag_start_world_;
  Vec2 mouse_world_;
  std::string status_text_;
  std::string tool_name_;

  enum class Tool {
    Select,
    Vertex,
    Straight,
    LeftCurve,
    RightCurve,
    PitBox,
    StartFinish,
    Barrier,
    Eraser,
    Surface
  };
  Tool current_tool_ = Tool::Vertex;
  SurfaceType current_surface_ = SurfaceType::Asphalt;
  double curve_radius_ = 80.0;
  double curve_arc_angle_ = kHalfPi;
  double straight_length_ = 100.0;
};

}
