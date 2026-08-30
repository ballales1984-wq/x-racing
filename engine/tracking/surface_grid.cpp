#include "tracking/surface_grid.h"

namespace p0::tracking {

SurfaceGrid::SurfaceGrid(double track_length, double track_width, double s_resolution, double l_resolution)
    : track_length_(track_length),
      track_width_(track_width),
      s_resolution_(s_resolution),
      l_resolution_(l_resolution) {
  s_count_ = static_cast<int>(std::ceil(track_length / s_resolution));
  l_count_ = static_cast<int>(std::ceil(track_width / l_resolution));
  cells_.resize(static_cast<size_t>(s_count_) * static_cast<size_t>(l_count_));
}

SurfaceCell& SurfaceGrid::cell(double s, double l) {
  const int si = s_index(s);
  const int li = l_index(l);
  return cells_[static_cast<size_t>(si) * static_cast<size_t>(l_count_) + li];
}

const SurfaceCell& SurfaceGrid::cell(double s, double l) const {
  const int si = s_index(s);
  const int li = l_index(l);
  return cells_[static_cast<size_t>(si) * static_cast<size_t>(l_count_) + li];
}

void SurfaceGrid::fill(const SurfaceCell& cell) {
  std::fill(cells_.begin(), cells_.end(), cell);
}

int SurfaceGrid::s_index(double s) const {
  if (s < 0.0) s += track_length_;
  if (s >= track_length_) s -= track_length_;
  return clamp(static_cast<int>(s / s_resolution_), 0, s_count_ - 1);
}

int SurfaceGrid::l_index(double l) const {
  const double half = track_width_ / 2.0;
  const double shifted = l + half;
  return clamp(static_cast<int>(shifted / l_resolution_), 0, l_count_ - 1);
}

}  // namespace p0::tracking
