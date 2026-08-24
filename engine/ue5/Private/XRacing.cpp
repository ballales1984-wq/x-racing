#include "XRacing.h"
#include "Interfaces/IPluginManager.h"

void FXRacingModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("XRacing module started"));
}

void FXRacingModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("XRacing module shutdown"));
}

IMPLEMENT_MODULE(FXRacingModule, XRacing)
