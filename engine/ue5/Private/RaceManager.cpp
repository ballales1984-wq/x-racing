#include "RaceManager.h"
#include "TimerManager.h"

ARaceManager::ARaceManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARaceManager::BeginPlay()
{
    Super::BeginPlay();
    StartCountdown();
}

void ARaceManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    switch (CurrentState)
    {
        case ERaceState::Countdown:
            CountdownTimer += DeltaTime;
            if (CountdownTimer >= CountdownTime)
            {
                CurrentState = ERaceState::Racing;
                CountdownTimer = 0.0f;
            }
            break;

        case ERaceState::Racing:
            RaceTime += DeltaTime;
            break;

        case ERaceState::Finished:
            break;
    }
}

void ARaceManager::StartCountdown()
{
    CurrentState = ERaceState::Countdown;
    CountdownTimer = 0.0f;
    RaceTime = 0.0f;
    CurrentLap = 0;
}

void ARaceManager::StartRace()
{
    CurrentState = ERaceState::Racing;
    RaceTime = 0.0f;
    CurrentLap = 0;
}

void ARaceManager::FinishRace()
{
    CurrentState = ERaceState::Finished;
}

void ARaceManager::ResetRace()
{
    CurrentState = ERaceState::Countdown;
    CountdownTimer = 0.0f;
    RaceTime = 0.0f;
    CurrentLap = 0;
}
