#include "NammaCityGameModeBase.h"
#include "NammaPlayerCharacter.h"
#include "NammaSandboxHUD.h"
#include "Kismet/GameplayStatics.h"
#include "NammaBicycle.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

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

void ANammaCityGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    // Map regeneration can remove optional actors. Keep the sandbox playable
    // without duplicating a cycle already parked near the actual PlayerStart.
    TActorIterator<APlayerStart> StartIt(GetWorld());
    APlayerStart* Start = StartIt ? *StartIt : nullptr;
    if (!Start) return;
    for (TActorIterator<ANammaBicycle> It(GetWorld()); It; ++It)
        if (FVector::Dist2D(It->GetActorLocation(), Start->GetActorLocation()) < 800.f
            && FMath::Abs(It->GetActorLocation().Z - Start->GetActorLocation().Z) < 300.f) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NammaStarterCycle), false, Start);
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0)) Query.AddIgnoredActor(Player);
    for (float Side : {-150.f, -200.f, 250.f, -300.f})
    {
        const FVector Candidate = Start->GetActorLocation() + Start->GetActorForwardVector() * 400.f
            + Start->GetActorRightVector() * Side;
        FHitResult Ground;
        if (!GetWorld()->LineTraceSingleByChannel(Ground, Candidate + FVector(0,0,200),
            Candidate - FVector(0,0,400), ECC_Visibility, Query) || Ground.ImpactNormal.Z < .9f) continue;
        bool bBothWheelsSupported = true;
        for (float AxleOffset : {-52.4f, 52.4f})
        {
            const FVector Axle = Candidate + Start->GetActorForwardVector() * AxleOffset;
            FHitResult Contact;
            if (!GetWorld()->LineTraceSingleByChannel(Contact, Axle + FVector(0,0,200),
                Axle - FVector(0,0,400), ECC_Visibility, Query) || Contact.ImpactNormal.Z < .9f
                || FMath::Abs(Contact.ImpactPoint.Z - Ground.ImpactPoint.Z) > 6.f)
                bBothWheelsSupported = false;
        }
        if (!bBothWheelsSupported) continue;
        const FVector Position = Ground.ImpactPoint + FVector(0,0,65);
        const FRotator Rotation(0, Start->GetActorRotation().Yaw, 0);
        if (GetWorld()->OverlapBlockingTestByChannel(Position, Rotation.Quaternion(), ECC_Pawn,
            FCollisionShape::MakeBox(FVector(65,35,45)), Query)) continue;
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        if (ANammaBicycle* Bike = GetWorld()->SpawnActor<ANammaBicycle>(Position, Rotation, Params))
        {
            Bike->Tags.Add(TEXT("NammaCycle.Starter"));
            UE_LOG(LogNammaDelivery, Log, TEXT("Starter cycle restored beside PlayerStart at %s"), *Position.ToString());
            return;
        }
    }
    UE_LOG(LogNammaDelivery, Warning, TEXT("No clear ground for starter cycle; run setup_cycle to inspect placement"));
}
