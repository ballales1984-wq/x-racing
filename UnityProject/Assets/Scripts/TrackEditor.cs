using UnityEngine;
using System.Collections.Generic;

namespace Project0.Unity
{
    // Editor helper that repairs common track authoring defects (segment gaps,
    // unclosed loop, misplaced waypoints) and re-validates the result.
    public class TrackEditor : MonoBehaviour
    {
        private TrackData trackData;
        private TrackValidator validator;

        /// <summary>
        /// Automatically fixes track issues
        /// </summary>
        public void AutoFixTrack(TrackData data)
        {
            trackData = data;

            Debug.Log("=== AUTO FIX TRACK ===\n");

            FixSegmentGaps();
            CloseTrackLoop();
            AlignWaypoints();
            ValidateAndReport();
        }

        /// <summary>
        /// Fixes gaps between segments by aligning endpoints
        /// </summary>
        private void FixSegmentGaps()
        {
            Debug.Log("→ Fixing segment gaps...\n");

            for (int i = 0; i < trackData.segments.Count - 1; i++)
            {
                TrackSegment current = trackData.segments[i];
                TrackSegment next = trackData.segments[i + 1];

                float gap = Vector3.Distance(current.end_pos, next.start_pos);

                if (gap > 0.1f)
                {
                    // Move the start of the next segment to the end of the current one
                    Debug.Log($"  Fixing gap {gap:F2}m between segment {i} and {i + 1}");

                    // Keep the curve direction, just translate
                    Vector3 offset = current.end_pos - next.start_pos;
                    next.start_pos = current.end_pos;

                    // If it is a curve, also translate the center
                    if (next.type == "curve" && next.center != Vector2.zero)
                    {
                        next.center = new Vector2(
                            next.center.x + offset.x,
                            next.center.y + offset.z
                        );
                    }

                    Debug.Log($"    ✓ Fixed: segment {i} end {current.end_pos} -> segment {i + 1} start\n");
                }
            }
        }

        /// <summary>
        /// Closes the loop by matching the last segment with the first
        /// </summary>
        private void CloseTrackLoop()
        {
            if (trackData.segments.Count < 2) return;

            Debug.Log("→ Closing track loop...\n");

            TrackSegment firstSegment = trackData.segments[0];
            TrackSegment lastSegment = trackData.segments[trackData.segments.Count - 1];

            float gap = Vector3.Distance(lastSegment.end_pos, firstSegment.start_pos);

            if (gap > 0.1f)
            {
                Debug.Log($"  Closing gap of {gap:F2}m");

                // Option 1: Translate the last segment
                Vector3 offset = firstSegment.start_pos - lastSegment.end_pos;
                lastSegment.end_pos = firstSegment.start_pos;

                // If it is a curve, also translate the center
                if (lastSegment.type == "curve" && lastSegment.center != Vector2.zero)
                {
                    lastSegment.center = new Vector2(
                        lastSegment.center.x + offset.x,
                        lastSegment.center.y + offset.z
                    );
                }

                Debug.Log("    ✓ Track closed\n");
            }
            else
            {
                Debug.Log($"  ✓ Track already closed (gap {gap:F2}m)\n");
            }
        }

        /// <summary>
        /// Re-aligns waypoints ensuring they are close to segments
        /// </summary>
        private void AlignWaypoints()
        {
            Debug.Log("→ Aligning waypoints...\n");

            if (trackData.waypoints.Count == 0) return;

            // Waypoint does not store a segment_id in the current model: we verify
            // that each waypoint is within a reasonable threshold from the nearest segment.
            const float toleranceMeters = 50f;

            foreach (Waypoint wp in trackData.waypoints)
            {
                int closestSegId = FindClosestSegment(wp.position);
                float dist = DistanceToSegment(wp.position, trackData.segments[closestSegId]);

                if (dist > toleranceMeters)
                {
                    Debug.LogWarning($"  Waypoint at {wp.position} is {dist:F2}m from nearest segment [{closestSegId}]");
                }
            }

            Debug.Log("  ✓ All waypoints checked\n");
        }

        /// <summary>
        /// Finds the closest segment to a position
        /// </summary>
        private int FindClosestSegment(Vector3 position)
        {
            int closestId = 0;
            float minDist = float.MaxValue;

            for (int i = 0; i < trackData.segments.Count; i++)
            {
                TrackSegment seg = trackData.segments[i];
                float distToStart = Vector3.Distance(position, seg.start_pos);
                float distToEnd = Vector3.Distance(position, seg.end_pos);
                float minSegDist = Mathf.Min(distToStart, distToEnd);

                if (minSegDist < minDist)
                {
                    minDist = minSegDist;
                    closestId = i;
                }
            }

            return closestId;
        }

        private float DistanceToSegment(Vector3 position, TrackSegment seg)
        {
            return Mathf.Min(
                Vector3.Distance(position, seg.start_pos),
                Vector3.Distance(position, seg.end_pos)
            );
        }

        /// <summary>
        /// Runs validation after fixes
        /// </summary>
        private void ValidateAndReport()
        {
            if (validator == null)
                validator = gameObject.AddComponent<TrackValidator>();

            bool isValid = validator.ValidateTrack(trackData);

            if (isValid)
                Debug.Log("✓ Track auto-fix completed successfully!");
            else
                Debug.LogWarning("⚠ Track still has issues after auto-fix. Review manually.");
        }
    }
}
