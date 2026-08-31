using System.Collections.Generic;
using UnityEngine;

namespace Project0.Unity
{
    // Data model consumed by TrackVisualizer. Populated from the generated
    // track (see XRTrackGenerator.BuildTrackData).

    [System.Serializable]
    public class Waypoint
    {
        public Vector3 position;
        public bool is_apex;
        public int corner_number;
        public bool brake_point;
    }

    [System.Serializable]
    public class TrackSegment
    {
        public string type = "straight"; // "straight" | "curve"
        public Vector3 start_pos;
        public Vector3 end_pos;
        public Vector2 center;            // .x = world X, .y = world Z
        public float radius_inner_m;
        public float radius_outer_m;
        public float arc_angle_deg;
    }

    [System.Serializable]
    public class PitBox
    {
        public Vector3 pos;
    }

    [System.Serializable]
    public class PitLane
    {
        public Vector3 entry_pos;
        public Vector3 exit_pos;
        public float width_m;
        public List<PitBox> pit_box_positions = new List<PitBox>();
    }

    [System.Serializable]
    public class MarshalZone
    {
        public int id;
        public Vector3 start_pos;
        public Vector3 end_pos;
    }

    [System.Serializable]
    public class Checkpoint
    {
        public int id;
        public Vector3 position;
        public float distance;
        public float width;
    }

    [System.Serializable]
    public class TrackData
    {
        public List<TrackSegment> segments = new List<TrackSegment>();
        public List<Waypoint> waypoints = new List<Waypoint>();
        public List<Checkpoint> checkpoints = new List<Checkpoint>();
        public PitLane pitLane = new PitLane();
        public List<MarshalZone> marshals = new List<MarshalZone>();
    }
}
