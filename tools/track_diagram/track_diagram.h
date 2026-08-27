#pragma once

#include "common.h"
#include "track/track.h"
#include <string>
#include <sstream>
#include <cstdint>

namespace p0::track_diagram {

struct SurfaceColor {
    uint8_t r, g, b;
    const char* name;
};

inline const SurfaceColor& color_for_surface(track::SurfaceType type) {
    static const SurfaceColor colors[] = {
        {100, 100, 100, "Asphalt"},
        { 78,  78, 106, "Wet Asphalt"},
        { 90,  74,  66, "Old Asphalt"},
        {255, 136,   0, "Kerb"},
        { 74, 154,  74, "Grass"},
        {210, 180, 140, "Gravel"},
        {139,  69,  19, "Dirt"},
        {255, 210, 127, "Sand"},
    };
    int idx = static_cast<int>(type);
    int count = static_cast<int>(sizeof(colors)/sizeof(colors[0]));
    if (idx < 0 || idx >= count) idx = 0;
    return colors[idx];
}

inline std::string format_color(uint8_t r, uint8_t g, uint8_t b, double alpha = 1.0) {
    char buf[48];
    if (alpha >= 1.0) {
        std::snprintf(buf, sizeof(buf), "rgb(%d,%d,%d)", r, g, b);
    } else {
        std::snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%.2f)", r, g, b, alpha);
    }
    return buf;
}

class TrackSvgExporter {
public:
    explicit TrackSvgExporter(const track::Track& track, const track::TrackParams& params = {});

    void set_canvas_size(int width, int height) { canvas_width_ = width; canvas_height_ = height; }
    void set_margin(double margin) { margin_ = margin; }
    void set_show_curvature_chart(bool show) { show_curvature_chart_ = show; }
    void set_show_legend(bool show) { show_legend_ = show; }
    void set_output_path(const std::string& path) { output_path_ = path; }

    bool export_svg();

    const std::string& svg_content() const { return svg_content_; }

private:
    void compute_bounds();
    void build_svg();
    std::string escape_xml(const std::string& s) const;
    void write_track_sections(std::ostringstream& oss);
    void write_centerline(std::ostringstream& oss);
    void write_box_lane(std::ostringstream& oss);
    void write_boundary_edges(std::ostringstream& oss);
    void write_pit_boxes(std::ostringstream& oss);
    void write_start_finish(std::ostringstream& oss);
    void write_curvature_chart(std::ostringstream& oss);
    void write_legend(std::ostringstream& oss);

    track::TrackParams params_;
    const track::Track& track_;
    int canvas_width_ = 1000;
    int canvas_height_ = 800;
    double margin_ = 80.0;
    double scale_ = 1.0;
    double offset_x_ = 0.0;
    double offset_y_ = 0.0;
    bool show_curvature_chart_ = true;
    bool show_legend_ = true;
    std::string output_path_ = "track_diagram.svg";
    std::string svg_content_;

    struct Bounds {
        double min_x, max_x, min_y, max_y;
    };
    Bounds bounds_;

    static constexpr double kChartHeight = 120.0;
    static constexpr double kLegendWidth = 200.0;
};

}
