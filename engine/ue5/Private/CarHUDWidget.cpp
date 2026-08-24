#include "CarHUDWidget.h"
#include "Components/TextBlock.h"

void UCarHUDWidget::UpdateHUD(float Speed, float RPM, int32 Gear, float LapTime, float BestLapTime)
{
    if (SpeedText)
    {
        SpeedText->SetText(FText::Format(FText::FromString("{0} km/h"), FMath::RoundToInt(Speed)));
    }

    if (RPMText)
    {
        RPMText->SetText(FText::Format(FText::FromString("{0} RPM"), FMath::RoundToInt(RPM)));
    }

    if (GearText)
    {
        FString GearString = Gear > 0 ? FString::FromInt(Gear) : TEXT("N");
        GearText->SetText(FText::FromString(GearString));
    }

    if (LapTimeText)
    {
        LapTimeText->SetText(GetTimeText(LapTime));
    }

    if (BestLapText)
    {
        BestLapText->SetText(GetTimeText(BestLapTime));
    }
}

void UCarHUDWidget::ResetLap()
{
    if (bLapStarted && CurrentLapTime > 5.0f)
    {
        if (CurrentLapTime < BestLapTime || BestLapTime <= 0.0f)
        {
            BestLapTime = CurrentLapTime;
        }
    }
    CurrentLapTime = 0.0f;
    bLapStarted = true;
}

FText UCarHUDWidget::GetTimeText(float Time) const
{
    int32 Minutes = FMath::FloorToInt(Time / 60.0f);
    float Seconds = Time - Minutes * 60.0f;
    return FText::Format(FText::FromString("{0:00}:{1:00.00}"), Minutes, Seconds);
}
