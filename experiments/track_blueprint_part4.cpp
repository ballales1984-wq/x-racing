
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
