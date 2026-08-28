using System;
using System.Collections.Generic;

namespace XRacing.Race
{
    public enum RaceType { Sprint, Endurance, Custom }

    public enum TireCompound { Soft = 0, Medium, Hard, Wet, Intermediate, Count }

    public enum ViolationType { None, PitSpeedExceeded, TrackLimits, JumpStart, UnsafeRelease, Count }

    public enum PenaltyType { None, DriveThrough, StopAndGo, TimePenalty, Disqualification }

    public enum PitEntryPolicy { FreeForAll, Fifo, Priority }

    public enum SpeedDetectionMode { AverageSpeed, Instantaneous, MaxSpeed }

    public enum CarRaceStatus
    {
        InGarage, Spawned, FormationLap, GridPosition, Racing,
        PitApproach, PitEntry, PitLane, BoxApproach, BoxStop,
        BoxService, BoxRelease, PitExit, TrackReentry, Finished, Dnf, PenaltyServed
    }

    public enum PitStopState
    {
        None, Requested, ApproachingPitLane, EnteringPitLane, PitLaneNavigation,
        BoxAssigned, AligningBox, StoppedAtBox, Servicing, ReleaseAuthorized,
        ExitingBox, PitExitNavigation, TrackReentry, Complete, Abandoned
    }

    public enum BoxState { Free, Occupied, Servicing, Releasing }

    public enum RaceSessionState { Pregame, Formation, Grid, GreenFlag, GreenFlagRunning, SafetyCar, CheckeredFlag, PostRace }

    [Serializable]
    public struct ServiceFlags
    {
        public bool refueling;
        public bool tireChange;
        public bool repair;
    }

    [Serializable]
    public struct CarFuelState
    {
        public float currentFuelL;
        public float fuelCapacityL;
        public float consumptionPerLapL;
        public float consumptionPerML;
    }

    [Serializable]
    public struct CarTireState
    {
        public TireCompound currentCompound;
        public int currentStintLaps;
        public float wearPercent;
    }

    public enum ValidationSeverity { Error, Warning, Info }

    [Serializable]
    public struct ValidationIssue
    {
        public ValidationSeverity severity;
        public string code;
        public string message;
        public string affectedComponent;
    }

    [Serializable]
    public struct RaceDefinition
    {
        public string raceId;
        public string trackId;
        public RaceType raceType;
        public int maxCars;
        public int gridSlots;
        public int laps;
        public float raceDistanceM;
        public int formationLap;

        public ServiceFlags services;
        public PitEntryPolicy pitEntryPolicy;

        public int pitMaxCars;
        public int pitMaxStopped;
        public int pitMinStops;
        public float speedToleranceMps;
        public PenaltyType violationPenalty;
        public float pitMinTimeS;
        public float pitMaxTimeS;

        public float refuelRateLs;
        public float tireChangeTimeS;
        public float repairRateHpS;
        public int pitQueueLimit;

        public float fuelCapacityL;
        public float fuelConsumptionBaseLM;
        public float fuelConsumptionSlopeLM2;

        public bool IsPitRequired => pitMinStops > 0;
        public bool UsesLaps => raceDistanceM <= 0f;
    }

    [Serializable]
    public struct CarAssignment
    {
        public int carId;
        public int teamId;
        public int gridSlot;
        public int pitBoxId;
        public float startFuelL;
        public TireCompound startTire;
        public int carNumber;
        public string driverName;
    }

    [Serializable]
    public struct TeamDefinition
    {
        public int teamId;
        public string teamName;
        public int carCount;
        public List<int> carIds;
        public int primaryBoxId;
        public int secondaryBoxId;
    }
}
