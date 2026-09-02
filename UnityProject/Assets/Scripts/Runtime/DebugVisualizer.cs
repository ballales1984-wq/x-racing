using System;
using System.Collections.Generic;
using UnityEngine;
using XRacing.Track;
using XRacing.Race;

namespace XRacing.Debug
{
    public enum DebugCategory
    {
        TrackDirection,
        StartFinish,
        Grid,
        Waypoints,
        RacingLine,
        Checkpoints,
        PitEntry,
        PitExit,
        PitSpeedStart,
        PitSpeedEnd,
        PitSpeedDetection,
        PitLanePath,
        PitBoxes,
        BoxServicePositions,
        MergeZone,
        CarStates,
        PitStates,
        SpeedViolations
    }

    public class DebugVisualizer : MonoBehaviour
    {
        public TrackData trackData;
        public RaceDefinition raceDefinition;
        public List<CarAssignment> assignments;

        [Flags]
        public enum VisibleFlags
        {
            None = 0,
            TrackDirection = 1 << 0,
            StartFinish = 1 << 1,
            Grid = 1 << 2,
            Waypoints = 1 << 3,
            RacingLine = 1 << 4,
            Checkpoints = 1 << 5,
            PitEntry = 1 << 6,
            PitExit = 1 << 7,
            PitSpeedStart = 1 << 8,
            PitSpeedEnd = 1 << 9,
            PitSpeedDetection = 1 << 10,
            PitLanePath = 1 << 11,
            PitBoxes = 1 << 12,
            BoxServicePositions = 1 << 13,
            MergeZone = 1 << 14,
            CarStates = 1 << 15,
            PitStates = 1 << 16,
            SpeedViolations = 1 << 17
        }

        public VisibleFlags visible = VisibleFlags.TrackDirection | VisibleFlags.StartFinish | VisibleFlags.Grid;

        private void OnDrawGizmos()
        {
            if (trackData.Equals(default(TrackData))) return;

            Gizmos.color = Color.white;

            if (visible.HasFlag(VisibleFlags.TrackDirection) && trackData.waypoints != null)
            {
                DrawArrows(trackData.waypoints);
            }

            if (visible.HasFlag(VisibleFlags.StartFinish))
            {
                DrawStartFinish(trackData.startFinish);
            }

            if (visible.HasFlag(VisibleFlags.Grid) && trackData.grid.slots != null)
            {
                DrawGrid(trackData.grid);
            }

            if (visible.HasFlag(VisibleFlags.Waypoints) && trackData.waypoints != null)
            {
                DrawWaypoints(trackData.waypoints);
            }

            if (visible.HasFlag(VisibleFlags.RacingLine) && trackData.racingLine != null)
            {
                DrawRacingLine(trackData.racingLine);
            }

            if (visible.HasFlag(VisibleFlags.PitLanePath) && trackData.pitLane.path != null)
            {
                DrawPitLanePath(trackData.pitLane.path);
            }

            if (visible.HasFlag(VisibleFlags.PitBoxes) && trackData.pitLane.boxes != null)
            {
                DrawPitBoxes(trackData.pitLane.boxes);
            }
        }

        private void DrawArrows(List<Waypoint> waypoints)
        {
            Gizmos.color = Color.yellow;
            foreach (var wp in waypoints)
            {
                Vector3 pos = new Vector3(wp.transform.position.x, 0.1f, wp.transform.position.y);
                Vector3 fwd = new Vector3(wp.transform.forward.x, 0f, wp.transform.forward.y);
                Gizmos.DrawLine(pos, pos + fwd * 3f);
                Gizmos.DrawSphere(pos, 0.3f);
            }
        }

        private void DrawStartFinish(StartFinishLine sf)
        {
            Gizmos.color = Color.red;
            Vector3 pos = new Vector3(sf.transform.position.x, 0.1f, sf.transform.position.y);
            Vector3 fwd = new Vector3(sf.transform.forward.x, 0f, sf.transform.forward.y);
            Vector3 right = Vector3.Cross(fwd, Vector3.up);
            Gizmos.DrawLine(pos - right * sf.width * 0.5f, pos + right * sf.width * 0.5f);
        }

        private void DrawGrid(GridDefinition grid)
        {
            Gizmos.color = Color.cyan;
            foreach (var slot in grid.slots)
            {
                Vector3 pos = new Vector3(slot.transform.position.x, 0.2f, slot.transform.position.y);
                Gizmos.DrawSphere(pos, 0.5f);
            }
        }

        private void DrawWaypoints(List<Waypoint> waypoints)
        {
            Gizmos.color = Color.green;
            for (int i = 0; i < waypoints.Count; i++)
            {
                Vector3 pos = new Vector3(waypoints[i].transform.position.x, 0.15f, waypoints[i].transform.position.y);
                Gizmos.DrawWireSphere(pos, waypoints[i].width * 0.5f);
            }
        }

        private void DrawRacingLine(List<RacingLineSample> line)
        {
            Gizmos.color = Color.gray;
            for (int i = 0; i < line.Count - 1; i++)
            {
                Vector3 a = new Vector3(line[i].transform.position.x, 0.05f, line[i].transform.position.y);
                Vector3 b = new Vector3(line[i + 1].transform.position.x, 0.05f, line[i + 1].transform.position.y);
                Gizmos.DrawLine(a, b);
            }
        }

        private void DrawPitLanePath(List<PitLanePathPoint> path)
        {
            Gizmos.color = Color.green;
            for (int i = 0; i < path.Count - 1; i++)
            {
                Vector3 a = new Vector3(path[i].transform.position.x, 0.05f, path[i].transform.position.y);
                Vector3 b = new Vector3(path[i + 1].transform.position.x, 0.05f, path[i + 1].transform.position.y);
                Gizmos.DrawLine(a, b);
            }
        }

        private void DrawPitBoxes(List<PitBox> boxes)
        {
            foreach (var box in boxes)
            {
                Gizmos.color = box.state == XRacing.Track.BoxState.Free ? Color.green : Color.red;
                Vector3 pos = new Vector3(box.position.position.x, 0.1f, box.position.position.y);
                Gizmos.DrawCube(pos, new Vector3(box.width, 0.5f, box.depth));
            }
        }
    }
}
