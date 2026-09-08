#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NammaInteractable.h"
#include "NammaDeliveryStation.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class NAMMACITY_API ANammaDeliveryStation : public AActor, public INammaInteractable
{
    GENERATED_BODY()
public:
    ANammaDeliveryStation();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual FText GetInteractionPrompt() const override;
    virtual bool CanInteract(const ANammaPlayerCharacter* Player) const override;
    virtual void Interact(ANammaPlayerCharacter* Player) override;
    UFUNCTION(BlueprintCallable, Category="Delivery") void ConfigureStation(bool bIsPickup);
    UPROPERTY(EditAnywhere, Category="Delivery") bool bPickup = true;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Counter;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
