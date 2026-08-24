#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackManager.generated.h"

UCLASS(BlueprintType)
class XRACING_API ATrackManager : public AActor
{
    GENERATED_BODY()

public:
    ATrackManager();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable)
    FVector GetStartPosition() const;

    UFUNCTION(BlueprintCallable)
    FRotator GetStartRotation() const;

    UFUNCTION(BlueprintCallable)
    float GetTrackLength() const { return TrackLength; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
    float TrackWidth = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
    float MainStraightWidth = 16.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
    float StraightLength = 765.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
    float CurveRadius = 75.0f;

private:
    UPROPERTY(VisibleAnywhere)
    TArray<FVector> TrackPoints;

    UPROPERTY(VisibleAnywhere)
    float TrackLength = 0.0f;

    void GenerateTrackPoints();
};
