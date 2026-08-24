#include "CarCameraComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

UCarCameraComponent::UCarCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCarCameraComponent::BeginPlay()
{
    Super::BeginPlay();
    if (TargetActor == nullptr)
    {
        TargetActor = GetOwner();
    }
}

void UCarCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (TargetActor == nullptr || FirstPersonCamera == nullptr || ChaseCamera == nullptr) return;

    if (bFirstPersonView)
    {
        FVector OwnerLoc = TargetActor->GetActorLocation();
        FVector Forward = TargetActor->GetActorForwardVector();
        FVector TargetPos = OwnerLoc + Forward * FirstPersonOffset.Z + FVector(0, 0, FirstPersonOffset.Y);
        FirstPersonCamera->SetWorldLocation(TargetPos);
        FirstPersonCamera->SetWorldRotation(TargetActor->GetActorRotation());
        FirstPersonCamera->SetActive(true);
        ChaseCamera->SetActive(false);
    }
    else
    {
        float HeadingRad = TargetActor->GetActorRotation().Yaw * PI / 180.0f;
        FVector Behind = FVector(FMath::Sin(HeadingRad + PI), FMath::Cos(HeadingRad + PI), 0.0f);
        FVector TargetPos = TargetActor->GetActorLocation() + Behind * FollowDistance + FVector(0, 0, FollowHeight);
        ChaseCamera->SetWorldLocation(FMath::VInterpTo(ChaseCamera->GetComponentLocation(), TargetPos, DeltaTime, SmoothTime));
        ChaseCamera->SetWorldRotation(FRotator(-10.0f, TargetActor->GetActorRotation().Yaw, 0.0f));
        FirstPersonCamera->SetActive(false);
        ChaseCamera->SetActive(true);
    }
}

void UCarCameraComponent::SetTarget(AActor* NewTarget)
{
    TargetActor = NewTarget;
    if (TargetActor == nullptr)
    {
        TargetActor = GetOwner();
    }
}

void UCarCameraComponent::ToggleView()
{
    bFirstPersonView = !bFirstPersonView;
}
