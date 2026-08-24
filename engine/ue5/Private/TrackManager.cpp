#include "TrackManager.h"
#include "Math/UnrealMathUtility.h"

ATrackManager::ATrackManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ATrackManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateTrackPoints();
}

void ATrackManager::GenerateTrackPoints()
{
    TrackPoints.Empty();
    TrackLength = 0.0f;

    int32 SegmentsPerStraight = 150;
    int32 SegmentsPerCurve = 75;

    for (int32 i = 0; i <= SegmentsPerStraight; ++i)
    {
        float T = (float)i / SegmentsPerStraight;
        TrackPoints.Add(FVector(T * StraightLength, 0.0f, 0.0f));
    }

    for (int32 i = 1; i <= SegmentsPerCurve; ++i)
    {
        float T = (float)i / SegmentsPerCurve;
        float Angle = -PI / 2.0f + T * PI;
        float X = StraightLength + CurveRadius * FMath::Cos(Angle);
        float Z = CurveRadius + CurveRadius * FMath::Sin(Angle);
        TrackPoints.Add(FVector(X, 0.0f, Z));
    }

    for (int32 i = 1; i <= SegmentsPerStraight; ++i)
    {
        float T = (float)i / SegmentsPerStraight;
        TrackPoints.Add(FVector(StraightLength - T * StraightLength, 0.0f, 2.0f * CurveRadius));
    }

    for (int32 i = 1; i <= SegmentsPerCurve; ++i)
    {
        float T = (float)i / SegmentsPerCurve;
        float Angle = PI / 2.0f + T * PI;
        float X = CurveRadius * FMath::Cos(Angle);
        float Z = CurveRadius + CurveRadius * FMath::Sin(Angle);
        TrackPoints.Add(FVector(X, 0.0f, Z));
    }

    for (int32 i = 1; i < TrackPoints.Num(); ++i)
    {
        TrackLength += FVector::Distance(TrackPoints[i - 1], TrackPoints[i]);
    }
}

FVector ATrackManager::GetStartPosition() const
{
    if (TrackPoints.IsEmpty()) return FVector::ZeroVector;
    return TrackPoints[0];
}

FRotator ATrackManager::GetStartRotation() const
{
    if (TrackPoints.Num() < 2) return FRotator::ZeroRotator;
    FVector Dir = (TrackPoints[1] - TrackPoints[0]).GetSafeNormal();
    return Dir.Rotation();
}
