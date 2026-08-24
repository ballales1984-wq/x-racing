#include "CarMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

UCarMovementComponent::UCarMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCarMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHeading = GetOwner()->GetActorRotation().Yaw * PI / 180.0f;
}

void UCarMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisToggleComponentTickFunction);

    if (GetOwner() == nullptr) return;

    UpdateDirectControl(DeltaTime);
    UpdateCamera(DeltaTime);
}

void UCarMovementComponent::ResetCar()
{
    CurrentSpeed = 0.0f;
    CurrentGear = 1;
    CurrentRPM = 800.0f;
    CurrentHeading = GetOwner()->GetActorRotation().Yaw * PI / 180.0f;
}

void UCarMovementComponent::UpdateDirectControl(float DeltaTime)
{
    float Throttle = 0.0f;
    float Brake = 0.0f;
    float Steer = 0.0f;

    if (GetWorld() == nullptr) return;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))
        {
            Throttle = 1.0f;
        }
        if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down))
        {
            Brake = 1.0f;
        }
        if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))
        {
            Steer = 1.0f;
        }
        if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right))
        {
            Steer = -1.0f;
        }
    }

    if (Throttle > 0.0f)
    {
        CurrentSpeed += Acceleration * DeltaTime;
    }
    else if (Brake > 0.0f)
    {
        CurrentSpeed -= BrakeForce * DeltaTime;
    }
    else
    {
        if (CurrentSpeed > 0.0f)
        {
            CurrentSpeed -= NaturalDeceleration * DeltaTime;
            if (CurrentSpeed < 0.0f) CurrentSpeed = 0.0f;
        }
        else if (CurrentSpeed < 0.0f)
        {
            CurrentSpeed += NaturalDeceleration * DeltaTime;
            if (CurrentSpeed > 0.0f) CurrentSpeed = 0.0f;
        }
    }

    CurrentSpeed = FMath::Clamp(CurrentSpeed, -MaxSpeed * 0.3f, MaxSpeed);

    if (FMath::Abs(CurrentSpeed) > 0.1f)
    {
        float SteerFactor = FMath::Clamp01(FMath::Abs(CurrentSpeed) / (MaxSpeed * 0.5f));
        float EffectiveSteer = Steer * MaxSteerAngle * (1.0f - SteerFactor * 0.7f);
        CurrentHeading -= EffectiveSteer * DeltaTime * FMath::Sign(CurrentSpeed);
    }

    CurrentHeading = FMath::NormalizeAxis(CurrentHeading);

    FVector Forward = FVector(FMath::Sin(CurrentHeading), FMath::Cos(CurrentHeading), 0.0f);
    FVector Move = Forward * CurrentSpeed * DeltaTime;

    GetOwner()->AddActorWorldOffset(Move);
    GetOwner()->SetActorRotation(FRotator(0.0f, CurrentHeading * 180.0f / PI, 0.0f));
}

void UCarMovementComponent::UpdateCamera(float DeltaTime)
{
    if (FirstPersonCamera == nullptr && ChaseCamera == nullptr) return;

    if (bFirstPersonView && FirstPersonCamera != nullptr)
    {
        FVector OwnerLoc = GetOwner()->GetActorLocation();
        FVector Forward = GetOwner()->GetActorForwardVector();
        FVector TargetPos = OwnerLoc + Forward * FirstPersonOffset.Z + FVector(0, 0, FirstPersonOffset.Y);
        FirstPersonCamera->SetWorldLocation(TargetPos);
        FirstPersonCamera->SetWorldRotation(GetOwner()->GetActorRotation());
    }
    else if (ChaseCamera != nullptr)
    {
        float HeadingRad = GetOwner()->GetActorRotation().Yaw * PI / 180.0f;
        FVector Behind = FVector(FMath::Sin(HeadingRad + PI), FMath::Cos(HeadingRad + PI), 0.0f);
        FVector TargetPos = GetOwner()->GetActorLocation() + Behind * CameraFollowDistance + FVector(0, 0, CameraFollowHeight);
        ChaseCamera->SetWorldLocation(FMath::VInterpTo(ChaseCamera->GetComponentLocation(), TargetPos, DeltaTime, CameraSmoothTime));
        ChaseCamera->SetWorldRotation(FRotator(-10.0f, GetOwner()->GetActorRotation().Yaw, 0.0f));
    }
}

void UCarMovementComponent::ToggleCameraView()
{
    bFirstPersonView = !bFirstPersonView;
}
