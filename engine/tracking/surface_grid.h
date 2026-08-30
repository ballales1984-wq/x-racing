#pragma once

#include "common.h"
#include "tracking/surface_cell.h"
#include <vector>

// Project 0 — tracking / surface grid
// Namespace: p0::tracking
namespace p0::tracking {

// 2D grid over track distance (S) and lateral offset (L).
class SurfaceGrid {
 public:
  SurfaceGrid(double track_length, double track_width, double s_resolution, double l_resolution);

  const SurfaceCell& cell(double s, double l) const;
  SurfaceCell& cell(double s, double l);

  double track_length() const { return track_length_; }
  double track_width() const { return track_width_; }
  int s_count() const { return s_count_; }
  int l_count() const { return l_count_; }

  void fill(const SurfaceCell& cell);

  SurfaceCell* data() { return cells_.data(); }
  const SurfaceCell* data() const { return cells_.data(); }
  size_t size() const { return cells_.size(); }
  SurfaceCell* begin() { return cells_.data(); }
  SurfaceCell* end() { return cells_.data() + cells_.size(); }
  const SurfaceCell* begin() const { return cells_.data(); }
  const SurfaceCell* end() const { return cells_.data() + cells_.size(); }

 private:
  int s_index(double s) const;
  int l_index(double l) const;

  double track_length_;
  double track_width_;
  double s_resolution_;
  double l_resolution_;
  int s_count_ = 0;
  int l_count_ = 0;
  std::vector<SurfaceCell> cells_;
};

}  // namespace p0::tracking
