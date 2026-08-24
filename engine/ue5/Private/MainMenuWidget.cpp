#include "MainMenuWidget.h"
#include "Components/Button.h"

void UMainMenuWidget::OnStartRaceClicked()
{
    RemoveFromParent();
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->ConsoleCommand("open MainMenu");
        }
    }
}

void UMainMenuWidget::OnQuitClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->ConsoleCommand("quit");
        }
    }
}
