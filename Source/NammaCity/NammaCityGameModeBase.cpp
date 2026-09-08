#include "NammaCityGameModeBase.h"
#include "NammaPlayerCharacter.h"
#include "NammaSandboxHUD.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogNammaDelivery);

ANammaCityGameModeBase::ANammaCityGameModeBase()
{
    PrimaryActorTick.bCanEverTick = false;
    DefaultPawnClass = ANammaPlayerCharacter::StaticClass();
    HUDClass = ANammaSandboxHUD::StaticClass();
}

bool ANammaCityGameModeBase::TryPickup()
{
    if (!Delivery.PickUp()) return false;
    UE_LOG(LogNammaDelivery, Log, TEXT("Parcel collected"));
    OnObjectiveChanged.Broadcast(GetObjective());
    return true;
}

bool ANammaCityGameModeBase::TryDeliver()
{
    if (!Delivery.Deliver()) return false;
    UE_LOG(LogNammaDelivery, Log, TEXT("Delivery complete"));
    OnObjectiveChanged.Broadcast(GetObjective());
    return true;
}

void ANammaCityGameModeBase::RestartDelivery()
{
    Delivery.Reset();
    UE_LOG(LogNammaDelivery, Log, TEXT("Delivery restarted"));
    if (auto* Player = Cast<ANammaPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
        Player->RecoverToStart();
    OnObjectiveChanged.Broadcast(GetObjective());
}

FText ANammaCityGameModeBase::GetObjective() const
{
    switch (Delivery.Stage)
    {
    case ENammaDeliveryStage::Carrying:
        return FText::FromString(TEXT("Carry the parcel through Market Court to Corner Stores."));
    case ENammaDeliveryStage::Complete:
        return FText::FromString(TEXT("Delivery complete! Press R to walk the route again."));
    default:
        return FText::FromString(TEXT("Collect a parcel at Namma Tea. Follow the gold markers."));
    }
}
