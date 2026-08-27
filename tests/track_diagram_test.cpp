#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "track/track.h"
#include "track_diagram.h"

using namespace p0;

TEST(TrackDiagram, DefaultTrackGeneratesSvg) {
    track::Track track(track::TrackType::Default);
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("<svg"), std::string::npos);
    EXPECT_NE(svg.find("</svg>"), std::string::npos);
}

TEST(TrackDiagram, PitCircuitTrackGeneratesSvg) {
    track::TrackParams params;
    track::Track track(track::TrackType::PitCircuit, params);
    track_diagram::TrackSvgExporter exporter(track, params);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("<svg"), std::string::npos);
    EXPECT_NE(svg.find("</svg>"), std::string::npos);
}

TEST(TrackDiagram, SvgContainsTrackFill) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"track-fill\""), std::string::npos);
    EXPECT_NE(svg.find("id=\"centerline\""), std::string::npos);
}

TEST(TrackDiagram, SvgContainsBoundaries) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"track-boundaries\""), std::string::npos);
}

TEST(TrackDiagram, SvgContainsStartFinish) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("START/FINISH"), std::string::npos);
    EXPECT_NE(svg.find("id=\"start-finish\""), std::string::npos);
}

TEST(TrackDiagram, SvgContainsCurvatureChart) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"curvature-chart\""), std::string::npos);
    EXPECT_NE(svg.find("Curvature Profile"), std::string::npos);
}

TEST(TrackDiagram, SvgContainsLegend) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"legend\""), std::string::npos);
    EXPECT_NE(svg.find("Surface Legend"), std::string::npos);
    EXPECT_NE(svg.find("asphalt"), std::string::npos);
    EXPECT_NE(svg.find("grass"), std::string::npos);
}

TEST(TrackDiagram, SvgContainsSurfaceColors) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("rgb(100,100,100)"), std::string::npos);
    EXPECT_NE(svg.find("rgb(74,154,74)"), std::string::npos);
}

TEST(TrackDiagram, PitBoxesRenderedForPitCircuit) {
    track::Track track(track::TrackType::PitCircuit);
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"pit-boxes\""), std::string::npos);
    EXPECT_NE(svg.find("Box 0"), std::string::npos);
    EXPECT_NE(svg.find("Box 1"), std::string::npos);
    EXPECT_NE(svg.find("Box 2"), std::string::npos);
    EXPECT_NE(svg.find("Box 3"), std::string::npos);
}

TEST(TrackDiagram, NoPitBoxesForDefaultTrack) {
    track::Track track(track::TrackType::Default);
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_EQ(svg.find("id=\"pit-boxes\""), std::string::npos);
}

TEST(TrackDiagram, BoxLaneRendered) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("id=\"box-lane\""), std::string::npos);
}

TEST(TrackDiagram, DisableCurvatureChart) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    exporter.set_show_curvature_chart(false);
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_EQ(svg.find("id=\"curvature-chart\""), std::string::npos);
}

TEST(TrackDiagram, DisableLegend) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    exporter.set_show_legend(false);
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_EQ(svg.find("id=\"legend\""), std::string::npos);
}

TEST(TrackDiagram, SvgContainsTrackLength) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("Length:"), std::string::npos);
    EXPECT_NE(svg.find("m</text>"), std::string::npos);
}

TEST(TrackDiagram, SvgContainsTrackType) {
    track::Track track(track::TrackType::Default);
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("Track: Default"), std::string::npos);
}

TEST(TrackDiagram, FileExportWritesValidSvg) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    std::string tmp_path = "test_track_diagram_tmp.svg";
    exporter.set_output_path(tmp_path);
    ASSERT_TRUE(exporter.export_svg());

    std::ifstream file(tmp_path);
    ASSERT_TRUE(file.is_open());
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    std::remove(tmp_path.c_str());

    std::string content = ss.str();
    EXPECT_NE(content.find("<svg"), std::string::npos);
    EXPECT_NE(content.find("</svg>"), std::string::npos);
}

TEST(TrackDiagram, SvgContainsPolygonElements) {
    track::Track track;
    track_diagram::TrackSvgExporter exporter(track);
    exporter.set_output_path("");
    ASSERT_TRUE(exporter.export_svg());
    const std::string& svg = exporter.svg_content();
    EXPECT_NE(svg.find("<polygon"), std::string::npos);
    EXPECT_NE(svg.find("<path"), std::string::npos);
}

TEST(TrackDiagram, SurfaceColorForType) {
    const auto& asphalt = track_diagram::color_for_surface(track::SurfaceType::Asphalt);
    EXPECT_EQ(asphalt.r, 100);
    EXPECT_EQ(asphalt.g, 100);
    EXPECT_EQ(asphalt.b, 100);

    const auto& grass = track_diagram::color_for_surface(track::SurfaceType::Grass);
    EXPECT_EQ(grass.r, 74);
    EXPECT_EQ(grass.g, 154);
    EXPECT_EQ(grass.b, 74);
}

TEST(TrackDiagram, FormatColorRgb) {
    EXPECT_EQ(track_diagram::format_color(255, 0, 0), "rgb(255,0,0)");
}

TEST(TrackDiagram, FormatColorRgba) {
    std::string result = track_diagram::format_color(255, 0, 0, 0.5);
    EXPECT_TRUE(result.find("rgba(255,0,0") != std::string::npos);
}
