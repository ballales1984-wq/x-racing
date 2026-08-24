#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "CarInputComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class XRACING_API UCarInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCarInputComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    float GetThrottle() const { return Throttle; }

    UFUNCTION(BlueprintCallable)
    float GetBrake() const { return Brake; }

    UFUNCTION(BlueprintCallable)
    float GetSteer() const { return Steer; }

    UFUNCTION(BlueprintCallable)
    bool GetToggleCameraPressed() const { return bToggleCameraPressed; }

    UFUNCTION(BlueprintCallable)
    void ClearToggleCamera() { bToggleCameraPressed = false; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float SteerSensitivity = 1.0f;

private:
    float Throttle = 0.0f;
    float Brake = 0.0f;
    float Steer = 0.0f;
    bool bToggleCameraPressed = false;

    TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent;
};
