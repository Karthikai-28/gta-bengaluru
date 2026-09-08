#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NammaHumanAnimInstance.generated.h"

// Post-processes the authored locomotion/foot IK pose without replacing it.
UCLASS()
class NAMMACITY_API UNammaHumanAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
