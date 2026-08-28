#pragma once

#include "race_config.h"
#include "track_data.h"
#include <string>
#include <vector>

namespace p0::track {

using p0::race::RaceDefinition;
using p0::race::CarAssignment;

// ---------------------------------------------------------------------------
// ValidationEngine — validates track, race, and pit lane consistency
// ---------------------------------------------------------------------------
class ValidationEngine {
 public:
  explicit ValidationEngine(const TrackData& track,
                            const p0::race::RaceDefinition& race,
                            const std::vector<p0::race::CarAssignment>& assignments);

  std::vector<p0::race::ValidationIssue> validate_all();
  std::vector<p0::race::ValidationIssue> validate_geometry() const;
  std::vector<p0::race::ValidationIssue> validate_direction() const;
  std::vector<p0::race::ValidationIssue> validate_grid() const;
  std::vector<p0::race::ValidationIssue> validate_pit_lane() const;
  std::vector<p0::race::ValidationIssue> validate_race() const;
  std::vector<p0::race::ValidationIssue> validate_assignments() const;

  static std::string severity_name(p0::race::ValidationSeverity s);

 private:
  const TrackData& track_;
  const p0::race::RaceDefinition& race_;
  const std::vector<p0::race::CarAssignment>& assignments_;
};

}
