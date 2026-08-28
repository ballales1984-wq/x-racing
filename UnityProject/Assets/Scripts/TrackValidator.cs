using UnityEngine;
using System.Collections.Generic;
using System.Linq;
using Project0.Unity;

public class TrackValidator : MonoBehaviour
{
    [SerializeField] private float toleranceMeters = 5.0f;
    private TrackData trackData;
    private List<string> validationErrors = new List<string>();
    private List<string> validationWarnings = new List<string>();

    public bool ValidateTrack(TrackData data)
    {
        trackData = data;
        validationErrors.Clear();
        validationWarnings.Clear();

        Debug.Log("=== TRACK VALIDATION START ===\n");

        ValidateMetadata();
        ValidateSegmentContinuity();
        ValidateTrackClosure();
        ValidateWaypoints();
        ValidatePitLane();
        ValidateCornerData();

        PrintValidationResults();

        return validationErrors.Count == 0;
    }

    private void ValidateMetadata()
    {
        if (trackData.segments == null || trackData.segments.Count == 0)
            AddError("No track segments defined");

        if (trackData.waypoints == null || trackData.waypoints.Count == 0)
            AddWarning("No waypoints defined");

        if (trackData.pitLane == null)
            AddWarning("Pit lane data is null");
    }

    private void ValidateSegmentContinuity()
    {
        Debug.Log("→ Checking segment continuity...\n");

        for (int i = 0; i < trackData.segments.Count - 1; i++)
        {
            TrackSegment current = trackData.segments[i];
            TrackSegment next = trackData.segments[i + 1];

            float gap = Vector3.Distance(current.end_pos, next.start_pos);

            if (gap > toleranceMeters)
            {
                AddError($"Segment gap: [{i}] -> [{i + 1}] = {gap:F2}m");
                Debug.Log($"  ✗ Gap {gap:F2}m between segment {i} and {i + 1}");
                Debug.Log($"    End of [{i}]: {current.end_pos}");
                Debug.Log($"    Start of [{i + 1}]: {next.start_pos}\n");
            }
            else
            {
                Debug.Log($"  ✓ Segment {i} → {i + 1}: gap {gap:F2}m (OK)\n");
            }
        }
    }

    private void ValidateTrackClosure()
    {
        if (trackData.segments.Count < 2) return;

        Debug.Log("→ Checking track closure (circuit loop)...\n");

        TrackSegment firstSegment = trackData.segments[0];
        TrackSegment lastSegment = trackData.segments[trackData.segments.Count - 1];

        float closureGap = Vector3.Distance(lastSegment.end_pos, firstSegment.start_pos);

        if (closureGap > toleranceMeters)
        {
            AddError($"Track not closed: gap of {closureGap:F2}m between last and first segment");
            Debug.Log($"  ✗ TRACK NOT CLOSED!");
            Debug.Log($"    Last segment end: {lastSegment.end_pos}");
            Debug.Log($"    First segment start: {firstSegment.start_pos}");
            Debug.Log($"    Gap: {closureGap:F2}m\n");
        }
        else
        {
            Debug.Log($"  ✓ Track is closed: gap {closureGap:F2}m (OK)\n");
        }
    }

    private void ValidateWaypoints()
    {
        Debug.Log("→ Checking waypoints...\n");

        if (trackData.waypoints.Count == 0)
        {
            AddWarning("No waypoints defined");
            return;
        }

        for (int w = 0; w < trackData.waypoints.Count; w++)
        {
            Waypoint wp = trackData.waypoints[w];

            if (wp.corner_number < 0)
                AddWarning($"Waypoint {w}: negative corner_number {wp.corner_number}");
        }

        var cornerGroups = trackData.waypoints
            .Where(wp => wp.corner_number > 0)
            .GroupBy(wp => wp.corner_number);

        Debug.Log($"  Waypoints by corner:");
        foreach (var group in cornerGroups)
        {
            var apices = group.Where(wp => wp.is_apex).ToList();
            var brakes = group.Where(wp => wp.brake_point).ToList();

            if (apices.Count != 1)
                AddWarning($"Corner {group.Key}: {apices.Count} apices (should be 1)");

            Debug.Log($"    Corner {group.Key}: {group.Count()} waypoints, {apices.Count} apex(es), {brakes.Count} brake point(s)");
        }

        Debug.Log($"  ✓ Total: {trackData.waypoints.Count} waypoints\n");
    }

    private void ValidatePitLane()
    {
        PitLane pit = trackData.pitLane;

        Debug.Log("→ Checking pit lane...\n");

        if (pit == null)
        {
            AddWarning("Pit lane data is null");
            return;
        }

        float pitLength = Vector3.Distance(pit.entry_pos, pit.exit_pos);
        if (pitLength <= 0)
            AddError("Pit lane entry and exit are at same position");

        Debug.Log($"  Pit entry: {pit.entry_pos}");
        Debug.Log($"  Pit exit: {pit.exit_pos}");
        Debug.Log($"  Pit length: {pitLength:F2}m");
        Debug.Log($"  Pit width: {pit.width_m}m");

        if (pit.pit_box_positions.Count == 0)
            AddWarning("No pit boxes defined");
        else
            Debug.Log($"  Pit boxes: {pit.pit_box_positions.Count}");
    }

    private void ValidateCornerData()
    {
        Debug.Log("→ Checking corner data...\n");

        var curves = trackData.segments
            .Where(s => s.type == "curve")
            .ToList();

        Debug.Log($"  Curve segments: {curves.Count}");

        foreach (var seg in curves)
        {
            float radius = (seg.radius_inner_m + seg.radius_outer_m) * 0.5f;
            Debug.Log($"    Type: {seg.type}, Arc: {seg.arc_angle_deg}°, Radius: {radius:F2}m");
        }
    }

    public TrackDirection DetermineTrackDirection()
    {
        if (trackData.segments.Count < 3) return TrackDirection.Unknown;

        Debug.Log("→ Determining track direction...\n");

        float area = 0;

        for (int i = 0; i < trackData.segments.Count; i++)
        {
            TrackSegment current = trackData.segments[i];
            TrackSegment next = trackData.segments[(i + 1) % trackData.segments.Count];

            Vector3 p1 = current.start_pos;
            Vector3 p2 = next.start_pos;

            area += (p2.x - p1.x) * (p2.z + p1.z);
        }

        TrackDirection direction = area > 0 ? TrackDirection.Clockwise : TrackDirection.CounterClockwise;

        Debug.Log($"  Area integral: {area:F2}");
        Debug.Log($"  Direction: {direction}\n");

        return direction;
    }

    public string GenerateValidationReport()
    {
        var report = new System.Text.StringBuilder();

        report.AppendLine("=== TRACK VALIDATION REPORT ===\n");

        report.AppendLine($"Segments: {trackData.segments.Count}");
        report.AppendLine($"Waypoints: {trackData.waypoints.Count}");
        report.AppendLine($"Direction: {DetermineTrackDirection()}\n");

        report.AppendLine($"Errors ({validationErrors.Count}):");
        foreach (var err in validationErrors)
            report.AppendLine($"  ✗ {err}");

        report.AppendLine($"\nWarnings ({validationWarnings.Count}):");
        foreach (var warn in validationWarnings)
            report.AppendLine($"  ⚠ {warn}");

        if (validationErrors.Count == 0)
            report.AppendLine("\n✓ Track validation PASSED");
        else
            report.AppendLine($"\n✗ Track validation FAILED ({validationErrors.Count} errors)");

        return report.ToString();
    }

    private void AddError(string message)
    {
        validationErrors.Add(message);
        Debug.LogError($"[VALIDATION] {message}");
    }

    private void AddWarning(string message)
    {
        validationWarnings.Add(message);
        Debug.LogWarning($"[VALIDATION] {message}");
    }

    private void PrintValidationResults()
    {
        Debug.Log(GenerateValidationReport());
    }
}

public enum TrackDirection
{
    Clockwise,
    CounterClockwise,
    Unknown
}
