// Project 0 — unit tests for track geometry and interpolation
#include <gtest/gtest.h>
#include "track/track.h"

using namespace p0;

// Track should have positive length and width
TEST(TrackV2, BasicProperties) {
  track::Track track;
  EXPECT_GT(track.length(), 0.0);
  EXPECT_GT(track.at(0.0).width, 0.0);
}

// Track start position should be valid
TEST(TrackV2, StartPositionValid) {
  track::Track track;
  Vec2 start = track.get_start_position();
  EXPECT_TRUE(std::isfinite(start.x()));
  EXPECT_TRUE(std::isfinite(start.y()));
}

// Track start heading should be valid
TEST(TrackV2, StartHeadingValid) {
  track::Track track;
  double heading = track.get_start_heading();
  EXPECT_TRUE(std::isfinite(heading));
}

// Track should wrap around at length
TEST(TrackV2, WrapsAroundAtLength) {
  track::Track track;
  double len = track.length();
  const auto& p0 = track.at(0.0);
  const auto& p1 = track.at(len);
  EXPECT_NEAR(p0.position.x(), p1.position.x(), 1.0);
  EXPECT_NEAR(p0.position.y(), p1.position.y(), 1.0);
}

// Track should handle negative distances
TEST(TrackV2, HandlesNegativeDistance) {
  track::Track track;
  double len = track.length();
  const auto& p_pos = track.at(len * 0.5);
  const auto& p_neg = track.at(len * 0.5 - len);
  EXPECT_NEAR(p_pos.position.x(), p_neg.position.x(), 1.0);
  EXPECT_NEAR(p_pos.position.y(), p_neg.position.y(), 1.0);
}

// Track should have continuous tangent vectors
TEST(TrackV2, TangentContinuous) {
  track::Track track;
  for (double d = 0.0; d < track.length(); d += 10.0) {
    const auto& tp = track.at(d);
    EXPECT_NEAR(tp.tangent.norm(), 1.0, 1e-6);
    EXPECT_NEAR(tp.normal.norm(), 1.0, 1e-6);
  }
}

// Track normal should be perpendicular to tangent
TEST(TrackV2, NormalPerpendicularToTangent) {
  track::Track track;
  for (double d = 0.0; d < track.length(); d += 10.0) {
    const auto& tp = track.at(d);
    double dot = tp.tangent.dot(tp.normal);
    EXPECT_NEAR(dot, 0.0, 1e-6);
  }
}

// Box lane should exist on main straight
TEST(BoxLaneV2, ExistsOnStraight) {
  track::Track track;
  EXPECT_TRUE(track.has_box_lane_at(100.0));
  EXPECT_TRUE(track.has_box_lane_at(200.0));
  EXPECT_FALSE(track.has_box_lane_at(380.0));
}

// Box lane should not exist on corners
TEST(BoxLaneV2, AbsentOnCorners) {
  track::Track track;
  EXPECT_FALSE(track.has_box_lane_at(350.0));
  EXPECT_FALSE(track.has_box_lane_at(track.length() * 0.75 + 10.0));
}

// Box lane width should be positive where it exists
TEST(BoxLaneV2, WidthPositive) {
  track::Track track;
  if (track.has_box_lane_at(0.0)) {
    EXPECT_GT(track.box_lane_width_at(0.0), 0.0);
  }
}

// Surface friction values should be in [0, 1]
TEST(TrackSurfaceV2, FrictionInRange) {
  for (int i = 0; i < static_cast<int>(track::SurfaceType::Count); ++i) {
    double f = track::friction_for_surface(static_cast<track::SurfaceType>(i));
    EXPECT_GE(f, 0.0);
    EXPECT_LE(f, 1.0);
  }
}

// Surface friction should be distinct per type
TEST(TrackSurfaceV2, FrictionDistinct) {
  EXPECT_GT(track::friction_for_surface(track::SurfaceType::Asphalt),
            track::friction_for_surface(track::SurfaceType::OldAsphalt));
  EXPECT_GT(track::friction_for_surface(track::SurfaceType::OldAsphalt),
            track::friction_for_surface(track::SurfaceType::Kerb));
  EXPECT_GT(track::friction_for_surface(track::SurfaceType::Kerb),
            track::friction_for_surface(track::SurfaceType::Grass));
}

// set_surface_at should change surface type
TEST(TrackSurfaceV2, SetSurfaceAtModifies) {
  track::Track track;
  const double len = track.length();
  const double mid = len * 0.5;
  EXPECT_EQ(track.surface_type_at(mid), track::SurfaceType::OldAsphalt);
  track.set_surface_at(mid, track::SurfaceType::WetAsphalt);
  EXPECT_EQ(track.surface_type_at(mid), track::SurfaceType::WetAsphalt);
}

// set_surface_at should also change friction
TEST(TrackSurfaceV2, SetSurfaceAtUpdatesFriction) {
  track::Track track;
  const double len = track.length();
  const double mid = len * 0.5;
  track.set_surface_at(mid, track::SurfaceType::Sand);
  EXPECT_NEAR(track.at(mid).friction, 0.25, 1e-9);
}

// Custom track params should be respected
TEST(TrackV2, CustomParams) {
  track::TrackParams params;
  params.total_length = 2000.0;
  params.default_width = 15.0;
  params.default_friction = 0.9;
  track::Track track(params);
  EXPECT_GT(track.length(), 1000.0);
  EXPECT_NEAR(track.at(0.0).width, 15.0, 1e-9);
  EXPECT_NEAR(track.at(0.0).friction, 0.9, 1e-9);
}

// Track should have curvature at corners
TEST(TrackV2, CurvatureAtCorners) {
  track::Track track;
  const double straight_length = 300.0;
  const auto& corner_tp = track.at(straight_length + 50.0);
  const auto& straight_tp = track.at(100.0);
  EXPECT_GT(std::abs(corner_tp.curvature), 0.0);
  EXPECT_NEAR(straight_tp.curvature, 0.0, 0.001);
}

// Track interpolation should be smooth
TEST(TrackV2, InterpolationSmooth) {
  track::Track track;
  double d1 = track.length() * 0.49;
  double d2 = track.length() * 0.51;
  const auto& p1 = track.at(d1);
  const auto& p2 = track.at(d2);
  double dx = p2.position.x() - p1.position.x();
  double dy = p2.position.y() - p1.position.y();
  EXPECT_GT(std::sqrt(dx*dx + dy*dy), 0.0);
}

// Default track should have pit box positions on the box lane straight
TEST(TrackV2, DefaultTrackHasPitBoxes) {
  track::Track track;
  const auto& boxes = track.pit_box_positions();
  EXPECT_GT(boxes.size(), 0u);
  for (double pos : boxes) {
    EXPECT_TRUE(track.has_box_lane_at(pos));
  }
}

// PitCircuit track type should be PitCircuit
TEST(PitCircuitV2, TrackTypeIsPitCircuit) {
  track::Track track(track::TrackType::PitCircuit);
  EXPECT_EQ(track.track_type(), track::TrackType::PitCircuit);
}

// PitCircuit should have positive length and width
TEST(PitCircuitV2, BasicProperties) {
  track::Track track(track::TrackType::PitCircuit);
  EXPECT_GT(track.length(), 0.0);
  EXPECT_GT(track.at(0.0).width, 0.0);
}

// PitCircuit should have box lane on the pit straight
TEST(PitCircuitV2, BoxLaneOnPitStraight) {
  track::Track track(track::TrackType::PitCircuit);
  EXPECT_TRUE(track.has_box_lane_at(900.0));
  EXPECT_TRUE(track.has_box_lane_at(1000.0));
  EXPECT_TRUE(track.has_box_lane_at(1100.0));
  EXPECT_TRUE(track.has_box_lane_at(1200.0));
}

// PitCircuit should not have box lane on main straight or corners
TEST(PitCircuitV2, BoxLaneAbsentElsewhere) {
  track::Track track(track::TrackType::PitCircuit);
  EXPECT_FALSE(track.has_box_lane_at(100.0));
  EXPECT_FALSE(track.has_box_lane_at(700.0));
  EXPECT_FALSE(track.has_box_lane_at(1600.0));
}

// PitCircuit pit box positions should be reported
TEST(PitCircuitV2, PitBoxPositionsReturned) {
  track::Track track(track::TrackType::PitCircuit);
  const auto& boxes = track.pit_box_positions();
  EXPECT_EQ(boxes.size(), 4u);
  EXPECT_NEAR(boxes[0], 850.0, 1.0);
  EXPECT_NEAR(boxes[1], 950.0, 1.0);
  EXPECT_NEAR(boxes[2], 1050.0, 1.0);
  EXPECT_NEAR(boxes[3], 1150.0, 1.0);
}

// PitCircuit start position should be valid
TEST(PitCircuitV2, StartPositionValid) {
  track::Track track(track::TrackType::PitCircuit);
  Vec2 start = track.get_start_position();
  EXPECT_TRUE(std::isfinite(start.x()));
  EXPECT_TRUE(std::isfinite(start.y()));
}

// PitCircuit start heading should be valid
TEST(PitCircuitV2, StartHeadingValid) {
  track::Track track(track::TrackType::PitCircuit);
  double heading = track.get_start_heading();
  EXPECT_TRUE(std::isfinite(heading));
}

// Mesh collider: start position should be inside track
TEST(TrackMesh, StartPositionInsideTrack) {
  track::Track track;
  Vec2 start = track.get_start_position();
  EXPECT_TRUE(track.collides_with_mesh(start));
}

// Mesh collider: point far off track should not collide
TEST(TrackMesh, FarOffTrackDoesNotCollide) {
  track::Track track;
  Vec2 far_away(10000.0, 10000.0);
  EXPECT_FALSE(track.collides_with_mesh(far_away));
}

// Mesh collider: point on track centerline should collide
TEST(TrackMesh, CenterlineCollides) {
  track::Track track;
  for (double d = 0.0; d < track.length(); d += 50.0) {
    const auto& tp = track.at(d);
    EXPECT_TRUE(track.collides_with_mesh(tp.position));
  }
}

// Mesh collider: generate_mesh should have valid vertices and indices
TEST(TrackMesh, GenerateMeshHasValidData) {
  track::Track track;
  auto mesh = track.generate_mesh();
  EXPECT_GT(mesh.vertices.size(), 0u);
  EXPECT_GT(mesh.indices.size(), 0u);
  EXPECT_EQ(mesh.indices.size() % 3, 0u);
}
