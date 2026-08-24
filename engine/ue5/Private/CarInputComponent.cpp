#include "CarInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

UCarInputComponent::UCarInputComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCarInputComponent::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->AddMappingContext();
        }
    }

    if (EnhancedInputComponent == nullptr)
    {
        EnhancedInputComponent = NewObject<UEnhancedInputComponent>(this, TEXT("EnhancedInputComponent"));
    }
}

void UCarInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (EnhancedInputComponent == nullptr) return;

    Throttle = 0.0f;
    Brake = 0.0f;
    Steer = 0.0f;

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
}
