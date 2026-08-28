#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;
using System.Collections.Generic;

namespace Project0.Unity
{
    // Editor-only gizmo overlay that makes the circuit readable: direction arrows,
    // segment endpoints, apex/brake waypoints, pit lane entry/exit and marshal zones.
    // Attach to a GameObject; it auto-reads data from the scene's XRTrackGenerator.
    public class TrackVisualizer : MonoBehaviour
    {
        private TrackData trackData;
        [SerializeField] private bool showDirectionArrows = true;
        [SerializeField] private bool showWaypoints = true;
        [SerializeField] private bool showSegmentEndpoints = true;
        [SerializeField] private bool showPitLane = true;
        [SerializeField] private bool showMarshalZones = true;
        [SerializeField] private float arrowScale = 1.0f;

        private Color colorStartFinish = Color.green;
        private Color colorSegmentStart = Color.blue;
        private Color colorSegmentEnd = Color.red;
        private Color colorWaypoint = Color.yellow;
        private Color colorApex = Color.cyan;
        private Color colorBrakePoint = Color.magenta;
        private Color colorPitEntry = Color.yellow;
        private Color colorPitExit = new Color(1.0f, 0.5f, 0.0f);
        private Color colorArrow = Color.white;

        public void Initialize(TrackData data)
        {
            trackData = data;
        }

        private void OnDrawGizmos()
        {
            if (trackData == null)
            {
                var gen = FindObjectOfType<XRTrackGenerator>();
                if (gen != null) trackData = gen.GetTrackData();
            }
            if (trackData == null) return;

            if (showDirectionArrows) DrawDirectionArrows();
            if (showSegmentEndpoints) DrawSegmentEndpoints();
            if (showWaypoints) DrawWaypoints();
            if (showPitLane) DrawPitLaneVisualization();
            if (showMarshalZones) DrawMarshalZones();
        }

        private void DrawDirectionArrows()
        {
            List<Vector3> trackPath = BuildTrackPath();
            if (trackPath.Count < 2) return;

            int arrowSpacing = Mathf.Max(1, trackPath.Count / 20);
            for (int i = 0; i + arrowSpacing < trackPath.Count; i += arrowSpacing)
            {
                Vector3 from = trackPath[i];
                Vector3 to = trackPath[i + arrowSpacing];
                Vector3 direction = (to - from).normalized;
                float distance = Vector3.Distance(from, to);
                Vector3 arrowPos = from + direction * (distance / 2f);
                DrawArrow(arrowPos, direction * arrowScale, colorArrow);
            }
        }

        private void DrawSegmentEndpoints()
        {
            for (int i = 0; i < trackData.segments.Count; i++)
            {
                TrackSegment segment = trackData.segments[i];

                Gizmos.color = (i == 0) ? colorStartFinish : colorSegmentStart;
                Gizmos.DrawWireSphere(segment.start_pos, 2.0f);
                if (i == 0) DebugLabel(segment.start_pos, "START/FINISH", colorStartFinish);

                Gizmos.color = colorSegmentEnd;
                Gizmos.DrawWireSphere(segment.end_pos, 1.5f);

                Gizmos.color = new Color(1, 1, 1, 0.3f);
                Gizmos.DrawLine(segment.start_pos, segment.end_pos);
            }
        }

        private void DrawWaypoints()
        {
            foreach (Waypoint wp in trackData.waypoints)
            {
                if (wp.is_apex)
                {
                    Gizmos.color = colorApex;
                    Gizmos.DrawWireSphere(wp.position, 3.0f);
                    DebugLabel(wp.position + Vector3.up * 3, $"APEX {wp.corner_number}", colorApex);
                }
                else if (wp.brake_point)
                {
                    Gizmos.color = colorBrakePoint;
                    Gizmos.DrawWireCube(wp.position, Vector3.one * 2.0f);
                    DebugLabel(wp.position + Vector3.up * 2, "BRAKE", colorBrakePoint);
                }
                else
                {
                    Gizmos.color = colorWaypoint;
                    Gizmos.DrawSphere(wp.position, 1.0f);
                }
            }

            Gizmos.color = new Color(1, 1, 0, 0.5f);
            for (int i = 0; i < trackData.waypoints.Count - 1; i++)
            {
                Gizmos.DrawLine(trackData.waypoints[i].position, trackData.waypoints[i + 1].position);
            }
        }

        private void DrawPitLaneVisualization()
        {
            PitLane pit = trackData.pitLane;
            if (pit.width_m <= 0.01f) return;

            Gizmos.color = colorPitEntry;
            Gizmos.DrawWireSphere(pit.entry_pos, 3.0f);
            DebugLabel(pit.entry_pos + Vector3.up * 3, "PIT ENTRY", colorPitEntry);

            Gizmos.color = colorPitExit;
            Gizmos.DrawWireSphere(pit.exit_pos, 3.0f);
            DebugLabel(pit.exit_pos + Vector3.up * 3, "PIT EXIT", colorPitExit);

            Vector3 pitDir = (pit.exit_pos - pit.entry_pos).normalized;
            Vector3 pitPerp = new Vector3(-pitDir.z, 0, pitDir.x) * (pit.width_m / 2f);

            Gizmos.color = new Color(1, 1, 0, 0.3f);
            Gizmos.DrawLine(pit.entry_pos - pitPerp, pit.entry_pos + pitPerp);
            Gizmos.DrawLine(pit.exit_pos - pitPerp, pit.exit_pos + pitPerp);
            Gizmos.DrawLine(pit.entry_pos - pitPerp, pit.exit_pos - pitPerp);
            Gizmos.DrawLine(pit.entry_pos + pitPerp, pit.exit_pos + pitPerp);

            foreach (PitBox box in pit.pit_box_positions)
            {
                Gizmos.color = new Color(1, 0.5f, 0, 0.5f);
                Gizmos.DrawWireCube(box.pos, Vector3.one * 5.0f);
            }
        }

        private void DrawMarshalZones()
        {
            foreach (MarshalZone zone in trackData.marshals)
            {
                Gizmos.color = new Color(0, 1, 1, 0.2f);
                Gizmos.DrawLine(zone.start_pos, zone.end_pos);
                Vector3 midpoint = (zone.start_pos + zone.end_pos) / 2;
                DebugLabel(midpoint + Vector3.up * 2, $"Marshal {zone.id}", Color.cyan);
            }
        }

        private List<Vector3> BuildTrackPath()
        {
            List<Vector3> path = new List<Vector3>();
            for (int s = 0; s < trackData.segments.Count; s++)
            {
                TrackSegment segment = trackData.segments[s];
                if (segment.type == "curve")
                {
                    Vector2 c = segment.center;
                    float radius = (segment.radius_inner_m + segment.radius_outer_m) * 0.5f;
                    // Sweep from the segment's actual start angle to its end angle so the
                    // arc passes through start_pos and end_pos (the original code swept
                    // from 0deg and ignored the real geometry).
                    float startAng = Mathf.Atan2(segment.start_pos.z - c.y, segment.start_pos.x - c.x);
                    float endAng = Mathf.Atan2(segment.end_pos.z - c.y, segment.end_pos.x - c.x);
                    float sweep = endAng - startAng;
                    while (sweep > Mathf.PI) sweep -= 2f * Mathf.PI;
                    while (sweep < -Mathf.PI) sweep += 2f * Mathf.PI;
                    float arcRad = Mathf.Deg2Rad * segment.arc_angle_deg;
                    // Prefer the authored arc length, keep the sweep direction consistent.
                    float dir = (sweep >= 0f) ? 1f : -1f;
                    int steps = Mathf.Max(4, Mathf.RoundToInt(segment.arc_angle_deg / 10f));
                    for (int i = 0; i <= steps; i++)
                    {
                        float t = (float)i / steps;
                        float ang = startAng + dir * arcRad * t;
                        float x = c.x + radius * Mathf.Cos(ang);
                        float z = c.y + radius * Mathf.Sin(ang);
                        float y = Mathf.Lerp(segment.start_pos.y, segment.end_pos.y, t);
                        path.Add(new Vector3(x, y, z));
                    }
                }
                else // straight
                {
                    if (path.Count == 0 || Vector3.Distance(path[path.Count - 1], segment.start_pos) > 0.01f)
                        path.Add(segment.start_pos);
                    if (Vector3.Distance(path[path.Count - 1], segment.end_pos) > 0.01f)
                        path.Add(segment.end_pos);
                }
            }
            return path;
        }

        private void DrawArrow(Vector3 position, Vector3 direction, Color color)
        {
            Gizmos.color = color;
            Gizmos.DrawLine(position, position + direction);

            Vector3 right = Quaternion.LookRotation(direction) * Quaternion.Euler(0, -20, 0) * Vector3.forward;
            Vector3 left = Quaternion.LookRotation(direction) * Quaternion.Euler(0, 20, 0) * Vector3.forward;

            float arrowHeadSize = direction.magnitude * 0.3f;
            Gizmos.DrawLine(position + direction, position + direction - right * arrowHeadSize);
            Gizmos.DrawLine(position + direction, position + direction - left * arrowHeadSize);
        }

        private void DebugLabel(Vector3 position, string text, Color color)
        {
#if UNITY_EDITOR
            var style = new GUIStyle();
            style.normal.textColor = color;
            Handles.Label(position, text, style);
#else
            Gizmos.color = color;
            Gizmos.DrawWireSphere(position, 0.5f);
#endif
        }
    }
}
