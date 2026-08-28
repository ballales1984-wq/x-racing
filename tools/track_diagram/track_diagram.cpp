// Project 0 — SVG track diagram generator implementation
// Generates scalable vector graphics depicting the race track layout,
// including color-coded surface sections, centerline, box lane, pit boxes,
// start/finish line, curvature profile chart, and surface legend.
#include "track_diagram.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace p0::track_diagram {

// Construct the SVG exporter for a given track.
// Immediately computes the bounding box of the track geometry
// so that the SVG viewBox and scale factor can be derived later.
TrackSvgExporter::TrackSvgExporter(const track::Track& track, const track::TrackParams& params)
    : params_(params), track_(track) {
    compute_bounds();
}

// Compute the world-space bounding box of the track including margins.
// Samples the centerline at regular intervals and expands the bounds
// by half the track width plus box lane width on each side.
// A fixed extra margin ensures elements near the edge remain visible.
void TrackSvgExporter::compute_bounds() {
    double length = track_.length();
    if (length <= 0) {
        bounds_ = {0, 1, 0, 1};
        return;
    }

    // Sample at most 500 points along the centerline for bounds estimation.
    double step = std::max(length / 500.0, 1.0);
    double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    bool first = true;

    for (double d = 0.0; d < length; d += step) {
        const auto tp = track_.at(d);
        double half_w = tp.width * 0.5 + tp.box_lane_width * 0.5;
        double x = tp.position.x();
        double y = tp.position.y();
        if (first) {
            min_x = x; max_x = x; min_y = y; max_y = y;
            first = false;
        }
        min_x = std::min(min_x, x - half_w);
        max_x = std::max(max_x, x + half_w);
        min_y = std::min(min_y, y - half_w);
        max_y = std::max(max_y, y + half_w);
    }

    // Expand bounds by a fixed margin to keep edges clear of the canvas border.
    double extra = 60.0;
    bounds_.min_x = min_x - extra;
    bounds_.max_x = max_x + extra;
    bounds_.min_y = min_y - extra;
    bounds_.max_y = max_y + extra;
}

// Escape XML special characters so user-provided strings can be safely
// embedded in SVG text elements and attribute values.
std::string TrackSvgExporter::escape_xml(const std::string& s) const {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Draw the track surface as a series of colored quads (two triangles per segment).
// Each quad spans from the previous sample point to the current one, offset
// along the track normal by half the track width on each side.
// The fill color is determined by the surface type at each sample, with low opacity
// to allow overlapping elements to remain visible.
void TrackSvgExporter::write_track_sections(std::ostringstream& oss) {
    double length = track_.length();
    if (length <= 0) return;

    // Sample at ~800 points for smooth surface rendering.
    double step = std::max(length / 800.0, 1.0);
    double prev_left_x = 0, prev_left_y = 0, prev_right_x = 0, prev_right_y = 0;
    bool first = true;

    oss << "  <g id=\"track-fill\" stroke=\"none\">\n";

    for (double d = 0.0; d <= length + step; d += step) {
        // Wrap distance into [0, length) range for closed-loop tracks.
        double dd = std::fmod(d, length);
        if (dd < 0) dd += length;
        const auto tp = track_.at(dd);

        double nx = tp.normal.x();
        double ny = tp.normal.y();

        // Convert world coordinates to SVG screen space (y is inverted).
        double left_x = (tp.position.x() + nx * tp.width * 0.5) * scale_ + offset_x_;
        double left_y = -(tp.position.y() + ny * tp.width * 0.5) * scale_ + offset_y_;
        double right_x = (tp.position.x() - nx * tp.width * 0.5) * scale_ + offset_x_;
        double right_y = -(tp.position.y() - ny * tp.width * 0.5) * scale_ + offset_y_;

        const auto& sc = color_for_surface(tp.surface_type);

        if (!first) {
            // Emit a quad connecting the previous and current sample on both sides.
            oss << "    <polygon points=\""
                << std::fixed << std::setprecision(2)
                << prev_left_x << "," << prev_left_y << " "
                << left_x << "," << left_y << " "
                << right_x << "," << right_y << " "
                << prev_right_x << "," << prev_right_y
                << "\" fill=\"" << format_color(sc.r, sc.g, sc.b, 0.20) << "\" />\n";
        } else {
            first = false;
        }

        prev_left_x = left_x;
        prev_left_y = left_y;
        prev_right_x = right_x;
        prev_right_y = right_y;
    }

    oss << "  </g>\n";
}

// Draw the track centerline as a single continuous SVG path.
// Samples at ~400 points provides a smooth curve without excessive path data.
void TrackSvgExporter::write_centerline(std::ostringstream& oss) {
    double length = track_.length();
    if (length <= 0) return;

    double step = std::max(length / 400.0, 1.0);

    oss << "  <g id=\"centerline\" stroke=\"#333333\" stroke-width=\"2\" fill=\"none\">\n";
    oss << "    <path d=\"";

    bool first = true;
    for (double d = 0.0; d <= length + step; d += step) {
        double dd = std::fmod(d, length);
        if (dd < 0) dd += length;
        const auto tp = track_.at(dd);
        double sx = tp.position.x() * scale_ + offset_x_;
        double sy = -tp.position.y() * scale_ + offset_y_;
        if (first) {
            oss << "M " << std::fixed << std::setprecision(2) << sx << " " << sy;
            first = false;
        } else {
            oss << " L " << sx << " " << sy;
        }
    }

    oss << "\" />\n";
    oss << "  </g>\n";
}

// Draw the pit box lane as a series of quads offset outward from the track edge.
// The box lane is rendered in red with transparency. Only segments where
// has_box_lane is true are drawn; the lane appears intermittently along the track.
void TrackSvgExporter::write_box_lane(std::ostringstream& oss) {
    double length = track_.length();
    if (length <= 0) return;

    double step = std::max(length / 800.0, 1.0);
    double prev_outer_x = 0, prev_outer_y = 0, prev_inner_x = 0, prev_inner_y = 0;
    bool in_box = false;
    bool first = true;

    oss << "  <g id=\"box-lane\" stroke=\"none\">\n";

    for (double d = 0.0; d <= length + step; d += step) {
        double dd = std::fmod(d, length);
        if (dd < 0) dd += length;
        const auto tp = track_.at(dd);

        bool box_here = tp.has_box_lane;
        double nx = tp.normal.x();
        double ny = tp.normal.y();

        // Outer edge is further from centerline; inner edge is at half box lane width.
        double outer_offset = tp.width * 0.5 + tp.box_lane_width * 0.5;
        double inner_offset = tp.width * 0.5 + tp.box_lane_width * 0.25;

        double outer_x = (tp.position.x() + nx * outer_offset) * scale_ + offset_x_;
        double outer_y = -(tp.position.y() + ny * outer_offset) * scale_ + offset_y_;
        double inner_x = (tp.position.x() + nx * inner_offset) * scale_ + offset_x_;
        double inner_y = -(tp.position.y() + ny * inner_offset) * scale_ + offset_y_;

        if (box_here) {
            if (!in_box) {
                // Entering a new box lane segment; reset the first-point flag.
                in_box = true;
                first = true;
            }
            if (!first) {
                oss << "    <polygon points=\""
                    << std::fixed << std::setprecision(2)
                    << prev_outer_x << "," << prev_outer_y << " "
                    << outer_x << "," << outer_y << " "
                    << inner_x << "," << inner_y << " "
                    << prev_inner_x << "," << prev_inner_y
                    << "\" fill=\"#ff5050\" fill-opacity=\"0.35\" stroke=\"#ff3030\" stroke-width=\"1\" />\n";
            } else {
                first = false;
            }
            prev_outer_x = outer_x;
            prev_outer_y = outer_y;
            prev_inner_x = inner_x;
            prev_inner_y = inner_y;
        } else {
            in_box = false;
        }
    }

    oss << "  </g>\n";
}

// Draw individual pit boxes as rotated rectangles positioned alongside the box lane.
// Each box is oriented along the track tangent/normal frame and labeled with
// its index and arc-length position in meters.
void TrackSvgExporter::write_pit_boxes(std::ostringstream& oss) {
    const auto& boxes = track_.pit_box_positions();
    if (boxes.empty()) return;

    oss << "  <g id=\"pit-boxes\" stroke=\"none\">\n";
    int idx = 0;
    for (double pos : boxes) {
        const auto tp = track_.at(pos);
        double nx = tp.normal.x();
        double ny = tp.normal.y();
        double tx = tp.tangent.x();
        double ty = tp.tangent.y();

        // Position the box center just outside the box lane outer edge.
        double box_cx = (tp.position.x() + nx * (tp.width * 0.5 + tp.box_lane_width * 0.5 + 2.0)) * scale_ + offset_x_;
        double base_y = -(tp.position.y() + ny * (tp.width * 0.5 + tp.box_lane_width * 0.5 + 2.0)) * scale_ + offset_y_;

        // Box dimensions scaled to the SVG zoom level.
        double hw = 2.0 * scale_;
        double hl = 3.0 * scale_;

        // Compute the four corners of the rotated rectangle using tangent/normal basis.
        double rx1 = tx * hl;
        double ry1 = ty * hl;
        double rx2 = -ty * hw;
        double ry2 = tx * hw;

        double x0 = box_cx + rx1 + rx2;
        double y0 = base_y + ry1 + ry2;
        double x1 = box_cx + rx1 - rx2;
        double y1 = base_y + ry1 - ry2;
        double x2 = box_cx - rx1 - rx2;
        double y2 = base_y - ry1 + ry2;
        double x3 = box_cx - rx1 + rx2;
        double y3 = base_y - ry1 - ry2;

        // Convert the rotated rect to an axis-aligned bounding rect for SVG.
        double min_x = std::min({x0, x1, x2, x3});
        double max_x = std::max({x0, x1, x2, x3});
        double min_y = std::min({y0, y1, y2, y3});
        double max_y = std::max({y0, y1, y2, y3});

        double rect_x = min_x;
        double rect_y = min_y;
        double rect_w = max_x - min_x;
        double rect_h = max_y - min_y;

        oss << "    <rect x=\""
            << std::fixed << std::setprecision(2)
            << rect_x << "\" y=\"" << rect_y
            << "\" width=\"" << rect_w << "\" height=\"" << rect_h
            << "\" fill=\"#cccccc\" fill-opacity=\"0.8\" stroke=\"#666666\" stroke-width=\"1\" />\n";

        // Label each pit box with its index and track distance.
        double text_x = (min_x + max_x) * 0.5;
        double text_y = min_y - 4;
        oss << "    <text x=\"" << text_x << "\" y=\"" << text_y
            << "\" font-size=\"10\" text-anchor=\"middle\" fill=\"#333333\">Box " << idx << " (" << pos << "m)</text>\n";
        idx++;
    }

    oss << "  </g>\n";
}

// Draw the start/finish line as a horizontal gold line across the track width
// at distance 0, with a text label above it.
void TrackSvgExporter::write_start_finish(std::ostringstream& oss) {
    const auto tp = track_.at(0.0);
    double nx = tp.normal.x();
    double ny = tp.normal.y();

    // Compute left and right endpoints at the track edges.
    double left_x = (tp.position.x() + nx * tp.width * 0.5) * scale_ + offset_x_;
    double left_y = -(tp.position.y() + ny * tp.width * 0.5) * scale_ + offset_y_;
    double right_x = (tp.position.x() - nx * tp.width * 0.5) * scale_ + offset_x_;
    double right_y = -(tp.position.y() - ny * tp.width * 0.5) * scale_ + offset_y_;

    double mid_x = (left_x + right_x) * 0.5;
    double mid_y = (left_y + right_y) * 0.5;

    oss << "  <g id=\"start-finish\" stroke=\"#FFD700\" stroke-width=\"3\">\n";
    oss << "    <line x1=\"" << std::fixed << std::setprecision(2)
        << left_x << "\" y1=\"" << left_y
        << "\" x2=\"" << right_x << "\" y2=\"" << right_y << "\" />\n";
    oss << "    <text x=\"" << mid_x << "\" y=\"" << (mid_y - 10)
        << "\" font-size=\"12\" text-anchor=\"middle\" fill=\"#FFD700\">START/FINISH</text>\n";
    oss << "  </g>\n";
}

// Draw a curvature profile chart at the bottom of the canvas.
// Shows track curvature (1/r) as a function of arc-length distance.
// Positive curvature indicates left turns; negative indicates right turns.
void TrackSvgExporter::write_curvature_chart(std::ostringstream& oss) {
    double length = track_.length();
    if (length <= 0) return;

    // Chart geometry: positioned in the lower-left area, leaving room for the legend.
    double chart_w = canvas_width_ - 2 * margin_ - kLegendWidth - 20.0;
    double chart_h = kChartHeight;
    double chart_y = canvas_height_ - margin_ - chart_h;
    double chart_x = margin_;

    double step = std::max(length / 300.0, 1.0);

    // First pass: find maximum absolute curvature for vertical scaling.
    double max_curv = 0.0;
    for (double d = 0.0; d < length; d += step) {
        const auto tp = track_.at(d);
        if (std::abs(tp.curvature) > max_curv) max_curv = std::abs(tp.curvature);
    }
    if (max_curv < kEpsilon) max_curv = 0.01;

    double plot_h = chart_h - 30;
    double baseline_y = chart_y + chart_h - 15;

    oss << "  <g id=\"curvature-chart\">\n";
    // Chart background with rounded corners.
    oss << "    <rect x=\"" << std::fixed << std::setprecision(2) << chart_x << "\" y=\"" << chart_y
        << "\" width=\"" << chart_w << "\" height=\"" << chart_h
        << "\" fill=\"#1a1a1a\" fill-opacity=\"0.9\" stroke=\"#444\" stroke-width=\"1\" rx=\"4\" />\n";

    // Chart title.
    oss << "    <text x=\"" << (chart_x + chart_w * 0.5) << "\" y=\"" << (chart_y + 18)
        << "\" font-size=\"13\" text-anchor=\"middle\" fill=\"#cccccc\" font-weight=\"bold\">Curvature Profile</text>\n";

    // Zero baseline.
    oss << "    <line x1=\"" << chart_x << "\" y1=\"" << baseline_y
        << "\" x2=\"" << (chart_x + chart_w) << "\" y2=\"" << baseline_y
        << "\" stroke=\"#555\" stroke-width=\"1\" />\n";

    // Second pass: draw the curvature polyline.
    oss << "    <path d=\"";
    bool first = true;
    for (double d = 0.0; d <= length + step; d += step) {
        double dd = std::fmod(d, length);
        if (dd < 0) dd += length;
        const auto tp = track_.at(dd);
        // Map distance to x, curvature to y (inverted so positive = up).
        double px = chart_x + (dd / length) * chart_w;
        double py = baseline_y - (tp.curvature / max_curv) * (plot_h * 0.45);
        if (first) {
            oss << "M " << std::fixed << std::setprecision(2) << px << " " << py;
            first = false;
        } else {
            oss << " L " << px << " " << py;
        }
    }
    oss << "\" fill=\"none\" stroke=\"#ffaa00\" stroke-width=\"2\" />\n";

    // Footer with track length and max curvature values.
    oss << "    <text x=\"" << (chart_x + 5) << "\" y=\"" << (chart_y + chart_h - 5)
        << "\" font-size=\"9\" fill=\"#888\">L=" << std::fixed << std::setprecision(1)
        << length << "m  max|" << (max_curv * 1000.0) << "|/km</text>\n";

    oss << "  </g>\n";
}

// Draw the surface legend in the upper-right corner.
// Lists all surface types with their representative colors and friction coefficients.
// Also includes entries for box lane, pit box, and start/finish line.
void TrackSvgExporter::write_legend(std::ostringstream& oss) {
    double lx = canvas_width_ - margin_ - kLegendWidth;
    double ly = margin_;
    double item_h = 18.0;
    double box_size = 12.0;

    int count = static_cast<int>(track::SurfaceType::Count);

    oss << "  <g id=\"legend\">\n";
    // Legend background panel.
    oss << "    <rect x=\"" << lx << "\" y=\"" << ly
        << "\" width=\"" << kLegendWidth << "\" height=\"" << (item_h * count + 24)
        << "\" fill=\"#1a1a1a\" fill-opacity=\"0.92\" stroke=\"#444\" stroke-width=\"1\" rx=\"4\" />\n";

    // Legend title.
    oss << "    <text x=\"" << (lx + kLegendWidth * 0.5) << "\" y=\"" << (ly + 14)
        << "\" font-size=\"12\" text-anchor=\"middle\" fill=\"#cccccc\" font-weight=\"bold\">Surface Legend</text>\n";

    // One row per surface type: color swatch + name + friction coefficient.
    double ty = ly + 28;
    for (int i = 0; i < count; ++i) {
        auto st = static_cast<track::SurfaceType>(i);
        const auto& sc = color_for_surface(st);
        oss << "    <rect x=\"" << (lx + 8) << "\" y=\"" << ty
            << "\" width=\"" << box_size << "\" height=\"" << box_size
            << "\" fill=\"" << format_color(sc.r, sc.g, sc.b) << "\" stroke=\"#333\" stroke-width=\"0.5\" />\n";
        oss << "    <text x=\"" << (lx + 8 + box_size + 6) << "\" y=\"" << (ty + box_size - 1)
            << "\" font-size=\"10\" fill=\"#cccccc\">";
        switch (st) {
            case track::SurfaceType::Asphalt:      oss << "asphalt"; break;
            case track::SurfaceType::WetAsphalt:   oss << "wet asphalt"; break;
            case track::SurfaceType::OldAsphalt:   oss << "old asphalt"; break;
            case track::SurfaceType::Kerb:         oss << "kerb"; break;
            case track::SurfaceType::Grass:        oss << "grass"; break;
            case track::SurfaceType::Gravel:       oss << "gravel"; break;
            case track::SurfaceType::Dirt:         oss << "dirt"; break;
            case track::SurfaceType::Sand:         oss << "sand"; break;
            default: break;
        }
        oss << " (" << std::fixed << std::setprecision(2)
            << track::friction_for_surface(st) << ")</text>\n";
        ty += item_h;
    }

    // Additional legend entries for non-surface track features.
    ty += 8;
    oss << "    <rect x=\"" << (lx + 8) << "\" y=\"" << ty
        << "\" width=\"" << box_size << "\" height=\"" << box_size
        << "\" fill=\"#ff5050\" fill-opacity=\"0.35\" stroke=\"#ff3030\" stroke-width=\"0.5\" />\n";
    oss << "    <text x=\"" << (lx + 8 + box_size + 6) << "\" y=\"" << (ty + box_size - 1)
        << "\" font-size=\"10\" fill=\"#cccccc\">box lane</text>\n";
    ty += item_h;

    oss << "    <rect x=\"" << (lx + 8) << "\" y=\"" << ty
        << "\" width=\"" << box_size << "\" height=\"" << box_size
        << "\" fill=\"#cccccc\" fill-opacity=\"0.8\" stroke=\"#666666\" stroke-width=\"0.5\" />\n";
    oss << "    <text x=\"" << (lx + 8 + box_size + 6) << "\" y=\"" << (ty + box_size - 1)
        << "\" font-size=\"10\" fill=\"#cccccc\">pit box</text>\n";
    ty += item_h;

    oss << "    <line x1=\"" << (lx + 8) << "\" y1=\"" << (ty + 6)
        << "\" x2=\"" << (lx + 8 + box_size) << "\" y2=\"" << (ty + 6)
        << "\" stroke=\"#FFD700\" stroke-width=\"3\" />\n";
    oss << "    <text x=\"" << (lx + 8 + box_size + 6) << "\" y=\"" << (ty + 11)
        << "\" font-size=\"10\" fill=\"#cccccc\">start/finish</text>\n";

    oss << "  </g>\n";
}

// Compute the scale factor and offsets to fit the track within the canvas.
// Accounts for margins, legend width, and optional curvature chart height.
// The track is centered in the available area after scaling.
void TrackSvgExporter::build_svg() {
    double width = std::max(bounds_.max_x - bounds_.min_x, 1.0);
    double height = std::max(bounds_.max_y - bounds_.min_y, 1.0);
    double available_h = canvas_height_ - 2 * margin_;
    if (show_curvature_chart_) available_h -= kChartHeight + 20.0;
    double available_w = canvas_width_ - 2 * margin_ - kLegendWidth - 20.0;

    // Uniform scale to fit both dimensions (preserve aspect ratio).
    scale_ = std::min(available_w / width, available_h / height);
    if (scale_ < 0.01) scale_ = 0.01;

    // Center the track in the available area and account for bounds offset.
    offset_x_ = margin_ + (available_w - width * scale_) * 0.5 - bounds_.min_x * scale_;
    offset_y_ = margin_ + (available_h - height * scale_) * 0.5 + bounds_.max_y * scale_;
}

// Draw the track boundary edges (left and right) as continuous polylines.
// These lines define the outer limits of the drivable surface.
void TrackSvgExporter::write_boundary_edges(std::ostringstream& oss) {
    double length = track_.length();
    if (length <= 0) return;

    double step = std::max(length / 800.0, 1.0);

    oss << "  <g id=\"track-boundaries\" stroke=\"#333333\" stroke-width=\"1.5\" fill=\"none\">\n";
    for (int side = 0; side < 2; ++side) {
        // side=0: left edge (positive normal direction); side=1: right edge (negative normal).
        double sign = (side == 0) ? 1.0 : -1.0;
        oss << "    <path d=\"";
        bool first = true;
        for (double d = 0.0; d <= length + step; d += step) {
            double dd = std::fmod(d, length);
            if (dd < 0) dd += length;
            const auto tp = track_.at(dd);
            double bx = (tp.position.x() + sign * tp.normal.x() * tp.width * 0.5) * scale_ + offset_x_;
            double by = -(tp.position.y() + sign * tp.normal.y() * tp.width * 0.5) * scale_ + offset_y_;
            if (first) {
                oss << "M " << std::fixed << std::setprecision(2) << bx << " " << by;
                first = false;
            } else {
                oss << " L " << bx << " " << by;
            }
        }
        oss << "\" />\n";
    }
    oss << "  </g>\n";
}

// Main entry point: build the complete SVG document and write to file.
// Layers are written back-to-front (fill first, then details on top):
// track sections -> pit boxes -> box lane -> boundaries -> centerline
// -> start/finish -> optional chart -> optional legend.
bool TrackSvgExporter::export_svg() {
    if (track_.length() <= 0) {
        svg_content_ = "";
        return false;
    }

    build_svg();

    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    oss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    oss << "width=\"" << canvas_width_ << "\" height=\"" << canvas_height_ << "\" ";
    oss << "viewBox=\"0 0 " << canvas_width_ << " " << canvas_height_ << "\" ";
    oss << "font-family=\"monospace\">\n";

    // Dark background rect filling the entire canvas.
    oss << "  <rect x=\"0\" y=\"0\" width=\"" << canvas_width_ << "\" height=\"" << canvas_height_
        << "\" fill=\"#0d0d0d\" />\n";

    // Draw track layers in z-order (bottom to top).
    write_track_sections(oss);
    write_pit_boxes(oss);
    write_box_lane(oss);
    write_boundary_edges(oss);
    write_centerline(oss);
    write_start_finish(oss);

    // Optional overlays controlled by flags.
    if (show_curvature_chart_) write_curvature_chart(oss);
    if (show_legend_) write_legend(oss);

    // Footer with track name and total length.
    double track_name_y = canvas_height_ - 12;
    oss << "  <text x=\"" << (canvas_width_ * 0.5) << "\" y=\"" << track_name_y
        << "\" font-size=\"12\" text-anchor=\"middle\" fill=\"#888\">";
    if (track_.track_type() == track::TrackType::Default) {
        oss << "Track: Default (Oval)";
    } else if (track_.track_type() == track::TrackType::PitCircuit) {
        oss << "Track: PitCircuit (Road Course)";
    } else {
        oss << "Track: CustomCircuit (Road Course)";
    }
    oss << " | Length: " << std::fixed << std::setprecision(1) << track_.length() << "m</text>\n";

    oss << "</svg>\n";

    svg_content_ = oss.str();

    // Write to disk if an output path was specified.
    if (!output_path_.empty()) {
        std::ofstream file(output_path_);
        if (!file.is_open()) {
            std::cerr << "Failed to open output file: " << output_path_ << std::endl;
            return false;
        }
        file << svg_content_;
        file.close();
        std::cout << "Track diagram saved to: " << output_path_ << std::endl;
    }

    return true;
}

}
