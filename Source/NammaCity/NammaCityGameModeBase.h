#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NammaDeliveryState.h"
#include "NammaCityGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNammaDelivery, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNammaObjectiveChanged, FText, Objective);

UCLASS()
class NAMMACITY_API ANammaCityGameModeBase : public AGameModeBase
{
    GENERATED_BODY()
public:
    ANammaCityGameModeBase();
    bool TryPickup();
    bool TryDeliver();
    void RestartDelivery();
    ENammaDeliveryStage GetDeliveryStage() const { return Delivery.Stage; }
    FText GetObjective() const;
    UPROPERTY(BlueprintAssignable, Category="Delivery")
    FNammaObjectiveChanged OnObjectiveChanged;
private:
    FNammaDeliveryState Delivery;
};
