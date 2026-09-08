#include "NammaInstancedProp.h"
#include "Components/InstancedStaticMeshComponent.h"

ANammaInstancedProp::ANammaInstancedProp()
{
    PrimaryActorTick.bCanEverTick = false;
    Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instances"));
    SetRootComponent(Instances);
    Instances->SetMobility(EComponentMobility::Static);
    Instances->SetCollisionProfileName(TEXT("BlockAll"));
}
