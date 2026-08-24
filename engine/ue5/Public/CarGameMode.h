#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CarGameMode.generated.h"

UCLASS(minimalapi)
class ACarGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACarGameMode();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    void StartRace();

    UFUNCTION(BlueprintCallable)
    void ResetRace();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
    int32 TotalLaps = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    bool bRaceActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Race")
    float RaceTime = 0.0f;

private:
    FTimerHandle CountdownTimerHandle;
    float CountdownTime = 3.0f;
};
