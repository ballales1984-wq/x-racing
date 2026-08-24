#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class XRACING_API UCarMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCarMovementComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UFUNCTION(BlueprintCallable)
    void ResetCar();

    UFUNCTION(BlueprintCallable)
    float GetSpeed() const { return CurrentSpeed; }

    UFUNCTION(BlueprintCallable)
    int32 GetGear() const { return CurrentGear; }

    UFUNCTION(BlueprintCallable)
    float GetRPM() const { return CurrentRPM; }

    UFUNCTION(BlueprintCallable)
    float GetHeading() const { return CurrentHeading; }

    UFUNCTION(BlueprintCallable)
    void SetFirstPersonCamera(UCameraComponent* Camera) { FirstPersonCamera = Camera; }

    UFUNCTION(BlueprintCallable)
    void SetChaseCamera(UCameraComponent* Camera) { ChaseCamera = Camera; }

    UFUNCTION(BlueprintCallable)
    void ToggleCameraView();

protected:
    void UpdateDirectControl(float DeltaTime);
    void UpdateCamera(float DeltaTime);

    // Physics parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float MaxSpeed = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Acceleration = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float BrakeForce = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float SteerSpeed = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float MaxSteerAngle = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float NaturalDeceleration = 5.0f;

    // Camera settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bFirstPersonView = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraFollowDistance = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraFollowHeight = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraSmoothTime = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector FirstPersonOffset = FVector(0.0f, 0.9f, 0.3f);

    // State
    float CurrentSpeed = 0.0f;
    float CurrentHeading = 0.0f;
    int32 CurrentGear = 1;
    float CurrentRPM = 800.0f;

    // Camera references
    UPROPERTY(Transient)
    UCameraComponent* FirstPersonCamera = nullptr;

    UPROPERTY(Transient)
    UCameraComponent* ChaseCamera = nullptr;

    FVector CameraVelocity;
    float CameraRotationVelocity;
};
