#include "NammaDeliveryStation.h"
#include "NammaCityGameModeBase.h"
#include "NammaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ANammaDeliveryStation::ANammaDeliveryStation()
{
    PrimaryActorTick.bCanEverTick = false;
    Counter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Counter"));
    SetRootComponent(Counter);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Counter->SetStaticMesh(Cube.Object);
    Counter->SetCollisionProfileName(TEXT("BlockAll"));
    Counter->SetRelativeScale3D(FVector(1.4, 1.4, 0.9));
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Counter);
    Label->SetAbsolute(false, false, true);
    Label->SetRelativeLocation(FVector(0, 0, 180));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(36);
    Label->SetTextRenderColor(FColor(255, 210, 95));
}

void ANammaDeliveryStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Label->SetText(FText::FromString(bPickup ? TEXT("NAMMA TEA | PICKUP") : TEXT("CORNER STORES | DELIVERY")));
}

FText ANammaDeliveryStation::GetInteractionPrompt() const
{
    return FText::FromString(bPickup ? TEXT("E  Collect parcel") : TEXT("E  Deliver parcel"));
}

bool ANammaDeliveryStation::CanInteract(const ANammaPlayerCharacter* Player) const
{
    const auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    if (!Player || !Mode || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(260.f))
        return false;
    return Mode->GetDeliveryStage() == (bPickup ? ENammaDeliveryStage::AwaitingPickup : ENammaDeliveryStage::Carrying);
}

void ANammaDeliveryStation::Interact(ANammaPlayerCharacter* Player)
{
    if (!CanInteract(Player)) return;
    auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    if (bPickup) Mode->TryPickup();
    else Mode->TryDeliver();
}

void ANammaDeliveryStation::ConfigureStation(bool bIsPickup)
{
    bPickup = bIsPickup;
    Label->SetText(FText::FromString(bPickup ? TEXT("NAMMA TEA | PICKUP") : TEXT("CORNER STORES | DELIVERY")));
}
