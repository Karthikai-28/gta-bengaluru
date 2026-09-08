#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NammaInstancedProp.generated.h"

class UInstancedStaticMeshComponent;

// Generator-owned batches; serialized instances are also visible in the editor.
UCLASS()
class NAMMACITY_API ANammaInstancedProp : public AActor
{
    GENERATED_BODY()
public:
    ANammaInstancedProp();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Geometry")
    TObjectPtr<UInstancedStaticMeshComponent> Instances;
};
