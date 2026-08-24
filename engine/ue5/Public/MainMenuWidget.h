#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class XRACING_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void OnStartRaceClicked();

    UFUNCTION(BlueprintCallable)
    void OnQuitClicked();

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> StartRaceButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;
};
