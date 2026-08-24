#include "LapTracker.h"

ALapTracker::ALapTracker()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ALapTracker::BeginPlay()
{
    Super::BeginPlay();
    ResetLap();
}

void ALapTracker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bLapInProgress)
    {
        CurrentLapTime += DeltaTime;
    }
}

void ALapTracker::OnCheckpointTriggered(int32 CheckpointIndex)
{
    if (CheckpointIndex < 0 || CheckpointIndex >= TotalCheckpoints) return;

    OnCheckpointPassed.Broadcast(CheckpointIndex);

    if (CheckpointIndex == 0 && CurrentCheckpoint == TotalCheckpoints - 1)
    {
        if (bLapInProgress && CurrentLapTime > 5.0f)
        {
            if (BestLapTime <= 0.0f || CurrentLapTime < BestLapTime)
            {
                BestLapTime = CurrentLapTime;
            }
            OnLapCompleted.Broadcast(CurrentLap, CurrentLapTime);
            CurrentLap++;
        }
        ResetLap();
    }
    else if (CheckpointIndex == CurrentCheckpoint + 1)
    {
        CurrentCheckpoint = CheckpointIndex;
    }
}

void ALapTracker::ResetLap()
{
    CurrentCheckpoint = 0;
    CurrentLapTime = 0.0f;
    LapStartTime = 0.0f;
    bLapInProgress = true;
    PassedCheckpoints.Empty();
}
