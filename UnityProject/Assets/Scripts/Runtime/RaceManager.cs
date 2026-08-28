using System;
using System.Collections.Generic;
using UnityEngine;
using XRacing.Track;

namespace XRacing.Race
{
    public class RaceManager : MonoBehaviour
    {
        public TrackData trackData;
        public RaceDefinition raceDefinition;
        public List<CarAssignment> assignments;
        public List<TeamDefinition> teams;

        public RaceSessionState sessionState = RaceSessionState.Pregame;
        public int currentLap;
        public float raceTime;

        private readonly Dictionary<int, PitStopFSM> pitFsmMap_ = new();
        private readonly Dictionary<int, float> carLastLapPos_ = new();
        private readonly Dictionary<int, int> carLapCount_ = new();
        private bool initialized_ = false;

        public bool Initialize()
        {
            if (trackData.Equals(default(TrackData)) || raceDefinition.Equals(default(RaceDefinition)) || assignments == null)
                return false;

            foreach (var a in assignments)
            {
                pitFsmMap_[a.carId] = new PitStopFSM(a.carId);
                carLapCount_[a.carId] = 0;
            }

            var issues = ValidationEngine.ValidateAll(trackData, raceDefinition, assignments);
            bool hasErrors = false;
            foreach (var issue in issues)
            {
                if (issue.severity == ValidationSeverity.Error)
                {
                    UnityEngine.Debug.LogError($"[VALIDATION] {issue.code}: {issue.message}");
                    hasErrors = true;
                }
                else
                {
                    UnityEngine.Debug.LogWarning($"[VALIDATION] {issue.code}: {issue.message}");
                }
            }

            initialized_ = !hasErrors;
            return initialized_;
        }

        public void StartRace()
        {
            if (!initialized_) return;
            sessionState = RaceSessionState.Formation;
            currentLap = 0;
            raceTime = 0f;
        }

        public void UpdateRace(float timestamp,
                               Dictionary<int, Vector2> carPositions,
                               Dictionary<int, float> carSpeeds)
        {
            if (!initialized_) return;
            raceTime = timestamp;

            switch (sessionState)
            {
                case RaceSessionState.Formation:
                    if (timestamp > raceDefinition.formationLap * 90f)
                        sessionState = RaceSessionState.GreenFlag;
                    break;
                case RaceSessionState.GreenFlag:
                    sessionState = RaceSessionState.GreenFlagRunning;
                    currentLap = 1;
                    break;
                case RaceSessionState.GreenFlagRunning:
                    if (currentLap >= raceDefinition.laps)
                        sessionState = RaceSessionState.CheckeredFlag;
                    break;
            }

            if (trackData.checkpoints != null && trackData.checkpoints.Count > 0)
            {
                foreach (var kvp in carPositions)
                {
                    int carId = kvp.Key;
                    float currPos = kvp.Value.x;
                    if (carLastLapPos_.TryGetValue(carId, out float prevPos))
                    {
                        if (prevPos > 0f && currPos < prevPos - trackData.lengthM * 0.5f)
                        {
                            carLapCount_[carId] = carLapCount_.GetValueOrDefault(carId) + 1;
                        }
                    }
                    carLastLapPos_[carId] = currPos;
                }
            }

            foreach (var kvp in pitFsmMap_)
            {
                float pos = 0f;
                float speed = 0f;
                if (carPositions != null && carPositions.TryGetValue(kvp.Key, out Vector2 p)) pos = p.x;
                if (carSpeeds != null && carSpeeds.TryGetValue(kvp.Key, out float s)) speed = s;
                kvp.Value.Update(timestamp, pos, speed);
            }
        }

        public bool RequestPitStop(int carId, TireCompound newTire, bool refuel, bool changeTires, bool repair)
        {
            if (sessionState != RaceSessionState.GreenFlagRunning && sessionState != RaceSessionState.SafetyCar)
                return false;

            if (!pitFsmMap_.TryGetValue(carId, out var fsm)) return false;
            if (fsm.IsActive) return false;

            int boxId = -1;
            foreach (var a in assignments)
            {
                if (a.carId == carId) { boxId = a.pitBoxId; break; }
            }

            if (boxId >= 0 && boxId < trackData.pitLane.BoxCount)
            {
                var box = trackData.pitLane.boxes[boxId];
                if (box.state != XRacing.Track.BoxState.Free) return false;
            }

            fsm.RequestStop(newTire, refuel, changeTires, repair);
            fsm.AssignBox(boxId);
            return true;
        }

        public PitStopState CarPitState(int carId)
        {
            return pitFsmMap_.TryGetValue(carId, out var fsm) ? fsm.CarState.state : PitStopState.None;
        }

        public bool CarHasPitViolation(int carId)
        {
            return pitFsmMap_.TryGetValue(carId, out var fsm) && fsm.CarState.violations != null && fsm.CarState.violations.Count > 0;
        }

        public string DebugReport()
        {
            var report = $"=== Race Manager Report ===\nSession: {(int)sessionState}\nLap: {currentLap}/{raceDefinition.laps}\nTime: {raceTime:F1}s\n";
            foreach (var a in assignments)
            {
                report += $"  Car {a.carId} pit_state={(int)CarPitState(a.carId)} laps={carLapCount_.GetValueOrDefault(a.carId)}\n";
            }
            return report;
        }
    }
}
