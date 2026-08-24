#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarCameraComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class XRACING_API UCarCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCarCameraComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UFUNCTION(BlueprintCallable)
    void SetTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable)
    void ToggleView();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bFirstPersonView = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FollowDistance = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FollowHeight = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float SmoothTime = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector FirstPersonOffset = FVector(0.0f, 0.9f, 0.3f);

private:
    TObjectPtr<AActor> TargetActor;
    TObjectPtr<UCameraComponent> FirstPersonCamera;
    TObjectPtr<UCameraComponent> ChaseCamera;
    FVector CameraVelocity;
};
