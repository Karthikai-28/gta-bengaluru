#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NammaInteractable.generated.h"

class ANammaPlayerCharacter;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UNammaInteractable : public UInterface
{
    GENERATED_BODY()
};

class NAMMACITY_API INammaInteractable
{
    GENERATED_BODY()
public:
    virtual FText GetInteractionPrompt() const = 0;
    virtual bool CanInteract(const ANammaPlayerCharacter* Player) const = 0;
    virtual void Interact(ANammaPlayerCharacter* Player) = 0;
};
