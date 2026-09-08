#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NammaSandboxHUD.generated.h"

class ANammaCityGameModeBase;
class ANammaDeliveryStation;

UCLASS()
class NAMMACITY_API ANammaSandboxHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void DrawHUD() override;
private:
    UFUNCTION() void ObjectiveChanged(FText Objective);
    FText ObjectiveText;
    TWeakObjectPtr<ANammaCityGameModeBase> DeliveryMode;
    TArray<TWeakObjectPtr<ANammaDeliveryStation>> Stations;
};
