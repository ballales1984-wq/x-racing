#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaceManager.generated.h"

UENUM(BlueprintType)
enum class ERaceState : uint8
{
    Countdown,
    Racing,
    Finished
};

UCLASS(BlueprintType)
class XRACING_API ARaceManager : public AActor
{
    GENERATED_BODY()

public:
    ARaceManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UFUNCTION(BlueprintCallable)
    void StartCountdown();

    UFUNCTION(BlueprintCallable)
    void StartRace();

    UFUNCTION(BlueprintCallable)
    void FinishRace();

    UFUNCTION(BlueprintCallable)
    void ResetRace();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
    int32 TotalLaps = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    ERaceState CurrentState = ERaceState::Countdown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    int32 CurrentLap = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    float RaceTime = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    float CountdownTime = 3.0f;

private:
    float CountdownTimer = 0.0f;
};
