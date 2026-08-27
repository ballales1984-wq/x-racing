#include "track/track.h"
#include "track_diagram.h"
#include <iostream>

using namespace p0;

int main(int argc, char* argv[]) {
    track::TrackParams params;
    track::TrackType type = track::TrackType::Default;
    std::string output = "track_diagram.svg";
    bool show_chart = true;
    bool show_legend = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" && i + 1 < argc) {
            std::string val = argv[++i];
            if (val == "pit" || val == "PitCircuit") type = track::TrackType::PitCircuit;
            else type = track::TrackType::Default;
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--no-chart") {
            show_chart = false;
        } else if (arg == "--no-legend") {
            show_legend = false;
        }
    }

    track::Track track(type, params);

    track_diagram::TrackSvgExporter exporter(track, params);
    exporter.set_output_path(output);
    exporter.set_show_curvature_chart(show_chart);
    exporter.set_show_legend(show_legend);

    if (!exporter.export_svg()) {
        std::cerr << "Failed to export SVG\n";
        return 1;
    }

    std::cout << "Track SVG exported to: " << output << std::endl;
    std::cout << "Track type: " << (type == track::TrackType::PitCircuit ? "PitCircuit" : "Default") << std::endl;
    std::cout << "Track length: " << track.length() << " m" << std::endl;
    return 0;
}
