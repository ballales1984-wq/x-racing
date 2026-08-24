#include "CarGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ACarGameMode::ACarGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACarGameMode::BeginPlay()
{
    Super::BeginPlay();
    ResetRace();
}

void ACarGameMode::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bRaceActive)
    {
        RaceTime += DeltaTime;
    }
}

void ACarGameMode::StartRace()
{
    bRaceActive = true;
    RaceTime = 0.0f;
}

void ACarGameMode::ResetRace()
{
    bRaceActive = false;
    RaceTime = 0.0f;
}
