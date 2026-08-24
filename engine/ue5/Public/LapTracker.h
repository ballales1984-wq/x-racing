#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LapTracker.generated.h"

UENUM(BlueprintType)
enum class ELapValidationResult : uint8
{
    Invalid,
    Valid,
    Skip
};

UCLASS(BlueprintType)
class XRACING_API ALapTracker : public AActor
{
    GENERATED_BODY()

public:
    ALapTracker();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    void OnCheckpointTriggered(int32 CheckpointIndex);

    UFUNCTION(BlueprintCallable)
    void ResetLap();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lap")
    int32 TotalCheckpoints = 10;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lap")
    int32 CurrentCheckpoint = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lap")
    int32 CurrentLap = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lap")
    float CurrentLapTime = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lap")
    float BestLapTime = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lap")
    bool bLapInProgress = false;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLapCompleted, int32, LapNumber, float, LapTime);
    UPROPERTY(BlueprintAssignable)
    FOnLapCompleted OnLapCompleted;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckpointPassed, int32, CheckpointIndex);
    UPROPERTY(BlueprintAssignable)
    FOnCheckpointPassed OnCheckpointPassed;

private:
    float LapStartTime = 0.0f;
    TArray<int32> PassedCheckpoints;
};
