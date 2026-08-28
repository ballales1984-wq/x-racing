using System;
using System.Collections.Generic;
using UnityEngine;

namespace XRacing.Race
{
    [Serializable]
    public struct PitServiceResult
    {
        public bool refueled;
        public float fuelAddedL;
        public float refuelTimeS;
        public bool tiresChanged;
        public TireCompound newCompound;
        public float tireChangeTimeS;
        public bool repaired;
        public float repairTimeS;
        public float totalServiceTimeS;
    }

    [Serializable]
    public struct SpeedViolation
    {
        public int carId;
        public float recordedSpeedMps;
        public float limitMps;
        public double timestamp;
        public float trackPositionM;
        public ViolationType type;
        public PenaltyType penalty;
        public bool served;
    }

    [Serializable]
    public struct CarPitState
    {
        public int carId;
        public PitStopState state;
        public int assignedBoxId;
        public double pitEntryTime;
        public double boxStopTime;
        public double serviceStartTime;
        public double serviceEndTime;
        public double pitExitTime;
        public int pitStopsCompleted;
        public List<SpeedViolation> violations;
        public PitServiceResult lastService;

        public double speedZoneEntryTime;
        public double speedZoneEntryPositionM;
        public bool speedZoneEntered;
    }

    public class PitStopFSM
    {
        private readonly int carId_;
        private PitStopState state_ = PitStopState.None;
        public CarPitState CarState;
        private double stateEntryTime_ = 0.0;
        private bool serviceRequested_ = false;
        private TireCompound requestedTire_ = TireCompound.Medium;
        private bool requestRefuel_ = false;
        private bool requestTires_ = false;
        private bool requestRepair_ = false;
        private double serviceStarted_ = 0.0;
        private double serviceDuration_ = 0.0;

        public PitStopFSM(int carId)
        {
            carId_ = carId;
            CarState = new CarPitState { carId = carId, state = PitStopState.None };
        }

        public void RequestStop(TireCompound newTire, bool refuel, bool tires, bool repair)
        {
            if (state_ == PitStopState.None)
            {
                state_ = PitStopState.Requested;
                stateEntryTime_ = 0.0;
                CarState.state = state_;
                requestedTire_ = newTire;
                requestRefuel_ = refuel;
                requestTires_ = tires;
                requestRepair_ = repair;
                serviceRequested_ = true;
            }
        }

        public void Update(double timestamp, float carTrackPosM, float carSpeedMps)
        {
            if (state_ == PitStopState.None || state_ == PitStopState.Complete || state_ == PitStopState.Abandoned)
                return;

            switch (state_)
            {
                case PitStopState.Requested:
                    TransitionTo(PitStopState.ApproachingPitLane, timestamp);
                    break;
                case PitStopState.BoxAssigned:
                    TransitionTo(PitStopState.AligningBox, timestamp);
                    break;
                case PitStopState.StoppedAtBox:
                    if (serviceRequested_ && serviceDuration_ > 0.0)
                    {
                        TransitionTo(PitStopState.Servicing, timestamp);
                        serviceStarted_ = timestamp;
                    }
                    else
                    {
                        TransitionTo(PitStopState.ReleaseAuthorized, timestamp);
                    }
                    break;
                case PitStopState.Servicing:
                    if (timestamp - serviceStarted_ >= serviceDuration_)
                    {
                        CarState.lastService.totalServiceTimeS = (float)(timestamp - serviceStarted_);
                        TransitionTo(PitStopState.ReleaseAuthorized, timestamp);
                    }
                    break;
                case PitStopState.ReleaseAuthorized:
                    TransitionTo(PitStopState.ExitingBox, timestamp);
                    break;
                case PitStopState.ExitingBox:
                    TransitionTo(PitStopState.TrackReentry, timestamp);
                    break;
                case PitStopState.TrackReentry:
                    TransitionTo(PitStopState.Complete, timestamp);
                    CarState.pitStopsCompleted++;
                    break;
            }
        }

        public void Abandon()
        {
            if (state_ != PitStopState.Complete && state_ != PitStopState.Abandoned)
                TransitionTo(PitStopState.Abandoned, 0.0);
        }

        public bool IsComplete => state_ == PitStopState.Complete || state_ == PitStopState.Abandoned;
        public bool IsActive => state_ != PitStopState.None && !IsComplete;

        public void AssignBox(int boxId) => CarState.assignedBoxId = boxId;
        public void SetServiceDuration(double duration) => serviceDuration_ = duration;

        private void TransitionTo(PitStopState newState, double timestamp)
        {
            state_ = newState;
            stateEntryTime_ = timestamp;
            CarState.state = newState;
        }
    }
}
