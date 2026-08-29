content = r'''
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
'''

with open(r'D:\x-racing\experiments\track_blueprint_part2.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print('Part 2 written')
