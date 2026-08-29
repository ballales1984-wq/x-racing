
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
