#pragma once

#include "CoreMinimal.h"

// World-space targets for the riding pose. The bicycle fills this from its own moving
// parts, so the rider's feet follow the pedals the drivetrain is actually turning
// rather than a looping animation. Consumed by the character's animation proxy.
struct FNammaRidePose
{
    FVector PedalLeft = FVector::ZeroVector;
    FVector PedalRight = FVector::ZeroVector;
    FVector GripLeft = FVector::ZeroVector;
    FVector GripRight = FVector::ZeroVector;
    FVector Saddle = FVector::ZeroVector;
    float TorsoPitchDegrees = 0.f;
    bool bFootDown = false;
};
