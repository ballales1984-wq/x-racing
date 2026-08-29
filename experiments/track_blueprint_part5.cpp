
bool BlueprintEditor::export_json(const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << "{\n";
  file << "  \"track_id\": \"" << track_.track_id << "\",\n";
  file << "  \"track_name\": \"" << track_.track_name << "\",\n";
  file << "  \"width_m\": " << track_.track_width << ",\n";
  file << "  \"start_position\": {\"x\": " << std::fixed << std::setprecision(2)
       << track_.start_position.x() << ", \"y\": " << track_.start_position.y() << "},\n";
  file << "  \"start_heading_deg\": " << track_.start_heading << ",\n";
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
  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"800\">\n";
  file << "  <rect width=\"100%\" height=\"100%\" fill=\"#1a1a1a\"/>\n";
  file << "  <text x=\"20\" y=\"30\" fill=\"#888\" font-family=\"monospace\" font-size=\"14\">";
  file << track_.track_name << " - Blueprint</text>\n";
  std::vector<Vec2> centerline;
  for (const auto& el : track_.elements)
    if (el.type == ElementType::TrackVertex || el.type == ElementType::Straight ||
        el.type == ElementType::LeftCurve || el.type == ElementType::RightCurve)
      centerline.push_back(el.position);
  if (centerline.size() >= 2) {
    file << "  <polyline points=\"";
    for (size_t i = 0; i < centerline.size(); ++i) {
      double sx = centerline[i].x() * 3.0 + 600.0;
      double sy = -centerline[i].y() * 3.0 + 400.0;
      file << sx << "," << sy << " ";
    }
    file << "\" fill=\"none\" stroke=\"#ffff00\" stroke-width=\"3\"/>\n";
  }
  for (const auto& el : track_.elements) {
    double sx = el.position.x() * 3.0 + 600.0;
    double sy = -el.position.y() * 3.0 + 400.0;
    if (el.type == ElementType::PitBox) {
      file << "  <rect x=\"" << sx - 12 << "\" y=\"" << sy - 6
           << "\" width=\"24\" height=\"12\" fill=\"#888\" stroke=\"#666\"/>\n";
    } else if (el.type == ElementType::StartFinish) {
      file << "  <line x1=\"" << sx - 20 << "\" y1=\"" << sy - 10
           << "\" x2=\"" << sx + 20 << "\" y2=\"" << sy + 10
           << "\" stroke=\"#FFD700\" stroke-width=\"3\"/>\n";
    } else if (el.type == ElementType::Barrier) {
      Vec2 end = el.position + el.tangent * el.length;
      double ex = end.x() * 3.0 + 600.0, ey = -end.y() * 3.0 + 400.0;
      file << "  <line x1=\"" << sx << "\" y1=\"" << sy << "\" x2=\"" << ex
           << "\" y2=\"" << ey << "\" stroke=\"#ff8c00\" stroke-width=\"3\"/>\n";
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
