using System;
using System.Collections.Generic;
using UnityEngine;

namespace XRacing.Track
{
    [Serializable]
    public struct Transform2D
    {
        public Vector2 position;
        public Vector2 forward;

        public Transform2D(Vector2 pos, Vector2 fwd)
        {
            position = pos;
            forward = fwd.normalized;
        }
    }

    [Serializable]
    public struct GridSlot
    {
        public int slotId;
        public Transform2D transform;
        public float width;
        public float depth;
    }

    public enum GridLayout { SingleColumn, TwoColumn, Custom }

    [Serializable]
    public struct GridDefinition
    {
        public List<GridSlot> slots;
        public int maxSlots;
        public GridLayout layout;
        public float rowSpacing;
        public float columnSpacing;

        public int SlotCount => slots?.Count ?? 0;
        public bool HasSlot(int id)
        {
            if (slots == null) return false;
            return slots.Exists(s => s.slotId == id);
        }
    }

    [Serializable]
    public struct Waypoint
    {
        public int id;
        public Transform2D transform;
        public float width;
        public float triggerRadius;
    }

    [Serializable]
    public struct RacingLineSample
    {
        public Transform2D transform;
        public float speedMps;
        public float throttle;
        public float brake;
        public float gear;
    }

    [Serializable]
    public struct Checkpoint
    {
        public int id;
        public Transform2D transform;
        public float width;
        public bool isSectorGate;
        public int sectorIndex;
    }

    [Serializable]
    public struct StartFinishLine
    {
        public Transform2D transform;
        public float width;
    }

    [Serializable]
    public struct PitEntryPoint
    {
        public Transform2D transform;
        public float width;
    }

    [Serializable]
    public struct PitExitPoint
    {
        public Transform2D transform;
        public float width;
    }

    public enum SpeedDetectionMode { AverageSpeed, Instantaneous, MaxSpeed }

    [Serializable]
    public struct SpeedDetectionZone
    {
        public Transform2D startLine;
        public Transform2D endLine;
        public float speedLimitMps;
        public float toleranceMps;
        public SpeedDetectionMode detectionMode;
        public int violationType;
        public int penalty;

        public float EffectiveLimitMps => speedLimitMps + toleranceMps;
    }

    [Serializable]
    public struct PitLanePathPoint
    {
        public Transform2D transform;
        public float width;
    }

    [Serializable]
    public struct MergeZone
    {
        public Transform2D start;
        public Transform2D end;

        public float Length => Vector2.Distance(start.position, end.position);
    }

    public enum BoxState { Free, Occupied, Servicing, Releasing }

    [Serializable]
    public struct PitBox
    {
        public int boxId;
        public int teamId;
        public Transform2D position;
        public Transform2D servicePosition;
        public Vector2 entryDirection;
        public Vector2 exitDirection;
        public float width;
        public float depth;
        public BoxState state;
        public int assignedCarId;
        public double occupiedSince;
    }

    [Serializable]
    public struct PitLaneDefinition
    {
        public PitEntryPoint entry;
        public SpeedDetectionZone speedZone;
        public List<PitLanePathPoint> path;
        public List<PitBox> boxes;
        public PitExitPoint exit;
        public MergeZone mergeZone;
        public float speedLimitMps;
        public float pitLaneLengthM;

        public int BoxCount => boxes?.Count ?? 0;
        public bool IsValid => (path != null && path.Count > 0) && (boxes != null && boxes.Count > 0);
    }

    [Serializable]
    public struct TrackData
    {
        public string trackId;
        public string trackName;
        public float lengthM;
        public float widthM;
        public float surfaceGrip;
        public string direction;

        public StartFinishLine startFinish;
        public GridDefinition grid;
        public List<Waypoint> waypoints;
        public List<RacingLineSample> racingLine;
        public List<Checkpoint> checkpoints;

        public PitLaneDefinition pitLane;
        public List<Transform2D> safetyZones;

        public bool IsValid => !string.IsNullOrEmpty(trackId) && lengthM > 0f;
        public int MaxGridSlots => grid.maxSlots;
    }
}
