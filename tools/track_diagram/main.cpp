// Project 0 — SVG track diagram CLI tool
// Generates scalable vector graphics depicting the race track layout.
// Supports both Default (oval) and PitCircuit (road course) track types.
#include "track_diagram.h"
#include <iostream>
#include <string>

using namespace p0;

// Print command-line usage information.
static void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  -o <file>       Output SVG file path (default: track_diagram.svg)\n"
              << "  -t <type>       Track type: 'default' or 'pit' (default: default)\n"
              << "  -w <width>      Canvas width in pixels (default: 1000)\n"
              << "  -h <height>     Canvas height in pixels (default: 800)\n"
              << "  --no-chart      Disable curvature profile chart\n"
              << "  --no-legend     Disable legend\n"
              << "  -? | --help     Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string output_path = "track_diagram.svg";
    std::string track_type = "default";
    int canvas_width = 1000;
    int canvas_height = 800;
    bool show_chart = true;
    bool show_legend = true;

    // Parse command-line arguments.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-?" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            track_type = argv[++i];
        } else if (arg == "-w" && i + 1 < argc) {
            canvas_width = std::stoi(argv[++i]);
        } else if (arg == "-h" && i + 1 < argc) {
            canvas_height = std::stoi(argv[++i]);
        } else if (arg == "--no-chart") {
            show_chart = false;
        } else if (arg == "--no-legend") {
            show_legend = false;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate track type.
    if (track_type != "default" && track_type != "pit") {
        std::cerr << "Error: track type must be 'default' or 'pit'\n";
        return 1;
    }

    // Build the track and export it as SVG.
    track::TrackParams params;
    track::TrackType type = (track_type == "pit") ? track::TrackType::PitCircuit : track::TrackType::Default;
    track::Track track(type, params);

    track_diagram::TrackSvgExporter exporter(track, params);
    exporter.set_canvas_size(canvas_width, canvas_height);
    exporter.set_output_path(output_path);
    exporter.set_show_curvature_chart(show_chart);
    exporter.set_show_legend(show_legend);

    if (!exporter.export_svg()) {
        std::cerr << "Failed to generate track diagram\n";
        return 1;
    }

    std::cout << "Track type: " << track_type << " | Length: " << track.length()
              << "m | Output: " << output_path << "\n";
    return 0;
}
