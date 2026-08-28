using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using XRacing.Race;

namespace XRacing.Track
{
    public static class ValidationEngine
    {
        public static List<ValidationIssue> ValidateAll(TrackData track, RaceDefinition race, List<CarAssignment> assignments)
        {
            var issues = new List<ValidationIssue>();
            issues.AddRange(ValidateGeometry(track));
            issues.AddRange(ValidateDirection(track));
            issues.AddRange(ValidateGrid(track, race));
            issues.AddRange(ValidatePitLane(track));
            issues.AddRange(ValidateRace(race));
            issues.AddRange(ValidateAssignments(race, assignments));
            return issues;
        }

        public static List<ValidationIssue> ValidateGeometry(TrackData track)
        {
            var issues = new List<ValidationIssue>();
            if (track.waypoints == null || track.waypoints.Count < 4)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GEO_001", message = "Track needs at least 4 waypoints", affectedComponent = "TrackGeometry" });
            if (track.lengthM < 100f)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GEO_002", message = "Track length too short (< 100 m)", affectedComponent = "TrackGeometry" });
            if (track.racingLine == null || track.racingLine.Count == 0)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GEO_003", message = "Racing line is empty", affectedComponent = "TrackGeometry" });
            return issues;
        }

        public static List<ValidationIssue> ValidateDirection(TrackData track)
        {
            var issues = new List<ValidationIssue>();
            if (track.direction != "CLOCKWISE" && track.direction != "COUNTER_CLOCKWISE")
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "DIR_001", message = "Track direction must be CLOCKWISE or COUNTER_CLOCKWISE", affectedComponent = "TrackDirection" });
            return issues;
        }

        public static List<ValidationIssue> ValidateGrid(TrackData track, RaceDefinition race)
        {
            var issues = new List<ValidationIssue>();
            if (race.maxCars > track.grid.maxSlots)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GRID_001", message = $"Race max_cars ({race.maxCars}) exceeds grid max_slots ({track.grid.maxSlots})", affectedComponent = "GridSystem" });
            if (track.grid.slots != null && track.grid.slots.Count < race.gridSlots)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GRID_002", message = $"Defined grid slots ({track.grid.slots.Count}) less than required ({race.gridSlots})", affectedComponent = "GridSystem" });
            if (track.grid.slots != null)
            {
                for (int i = 0; i < track.grid.slots.Count; i++)
                {
                    for (int j = i + 1; j < track.grid.slots.Count; j++)
                    {
                        float dist = Vector2.Distance(track.grid.slots[i].transform.position, track.grid.slots[j].transform.position);
                        if (dist < 3.5f)
                            issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "GRID_003", message = $"Grid slots {track.grid.slots[i].slotId} and {track.grid.slots[j].slotId} overlap (distance: {dist:F2} m)", affectedComponent = "GridSystem" });
                    }
                }
            }
            return issues;
        }

        public static List<ValidationIssue> ValidatePitLane(TrackData track)
        {
            var issues = new List<ValidationIssue>();
            if (track.pitLane.path == null || track.pitLane.path.Count == 0)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "PIT_001", message = "Pit lane path is empty", affectedComponent = "PitLane" });
            if (track.pitLane.boxes == null || track.pitLane.boxes.Count == 0)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "PIT_002", message = "No pit boxes defined", affectedComponent = "PitLane" });
            if (track.pitLane.speedZone.speedLimitMps <= 0f)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "PIT_003", message = "Speed limit must be > 0", affectedComponent = "PitLane" });
            return issues;
        }

        public static List<ValidationIssue> ValidateRace(RaceDefinition race)
        {
            var issues = new List<ValidationIssue>();
            if (race.maxCars > race.gridSlots)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "RACE_001", message = "max_cars exceeds grid_slots", affectedComponent = "RaceRules" });
            if (race.laps < 1 && race.raceDistanceM <= 0f)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "RACE_002", message = "Race must have at least 1 lap or positive distance", affectedComponent = "RaceRules" });
            if (race.pitMinStops > race.laps && race.laps > 0)
                issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "RACE_003", message = "pit_min_stops exceeds total laps", affectedComponent = "RaceRules" });
            return issues;
        }

        public static List<ValidationIssue> ValidateAssignments(RaceDefinition race, List<CarAssignment> assignments)
        {
            var issues = new List<ValidationIssue>();
            var usedSlots = new HashSet<int>();
            var usedBoxes = new HashSet<int>();
            foreach (var a in assignments)
            {
                if (!usedSlots.Add(a.gridSlot))
                    issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "ASGN_001", message = $"Duplicate grid slot {a.gridSlot} for car {a.carId}", affectedComponent = "CarAssignments" });
                if (a.pitBoxId >= 0 && !usedBoxes.Add(a.pitBoxId))
                    issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "ASGN_002", message = $"Duplicate pit box {a.pitBoxId} for car {a.carId}", affectedComponent = "CarAssignments" });
                if (a.startFuelL > race.fuelCapacityL)
                    issues.Add(new ValidationIssue { severity = ValidationSeverity.Error, code = "ASGN_003", message = $"Start fuel exceeds capacity for car {a.carId}", affectedComponent = "CarAssignments" });
            }
            return issues;
        }
    }
}
