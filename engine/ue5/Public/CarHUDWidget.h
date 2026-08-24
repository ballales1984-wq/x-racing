#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CarHUDWidget.generated.h"

UCLASS()
class XRACING_API UCarHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void UpdateHUD(float Speed, float RPM, int32 Gear, float LapTime, float BestLapTime);

    UFUNCTION(BlueprintCallable)
    void ResetLap();

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SpeedText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> RPMText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GearText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> LapTimeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BestLapText;

    float CurrentLapTime = 0.0f;
    float BestLapTime = 0.0f;
    bool bLapStarted = false;
};
