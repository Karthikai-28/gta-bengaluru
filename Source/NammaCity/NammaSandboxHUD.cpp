#include "NammaSandboxHUD.h"
#include "NammaCityGameModeBase.h"
#include "NammaPlayerCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NammaDeliveryStation.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void ANammaSandboxHUD::BeginPlay()
{
    Super::BeginPlay();
    DeliveryMode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    if (DeliveryMode.IsValid())
    {
        ObjectiveText = DeliveryMode->GetObjective();
        DeliveryMode->OnObjectiveChanged.AddDynamic(this, &ANammaSandboxHUD::ObjectiveChanged);
    }
    for (TActorIterator<ANammaDeliveryStation> It(GetWorld()); It; ++It) Stations.Add(*It);
}

void ANammaSandboxHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (DeliveryMode.IsValid()) DeliveryMode->OnObjectiveChanged.RemoveDynamic(this, &ANammaSandboxHUD::ObjectiveChanged);
    Super::EndPlay(EndPlayReason);
}

void ANammaSandboxHUD::ObjectiveChanged(FText Objective) { ObjectiveText = Objective; }

void ANammaSandboxHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    const auto* Player = Cast<ANammaPlayerCharacter>(PlayerOwner->GetPawn());
    if (!Mode || !Player) return;
    const float W = Canvas->SizeX;
    const float H = Canvas->SizeY;
    const FLinearColor Gold(1.f, 0.76f, 0.3f);
    DrawRect(FLinearColor(0.02f, 0.04f, 0.055f, 0.9f), 20, 20, FMath::Min(W - 40, 760.f), 100);
    DrawText(TEXT("NAMMA CITY  /  FIRST DELIVERY"), Gold, 36, 30, nullptr, 1.35f);
    DrawText(ObjectiveText.ToString(), FLinearColor::White, 36, 62);
    DrawText(FString::Printf(TEXT("Parcel: %s"), Mode->GetDeliveryStage() == ENammaDeliveryStage::Carrying ? TEXT("carrying") : TEXT("none")), Gold, 36, 89);
    DrawRect(FLinearColor(0, 0, 0, 0.8f), 20, H - 54, W - 40, 34);
    DrawText(TEXT("WASD Move   Mouse Look   Shift Sprint   Space Jump   E Interact   Esc Pause"), FLinearColor::White, 32, H - 45);
    DrawLine(W / 2 - 5, H / 2, W / 2 + 5, H / 2, FLinearColor::White);
    DrawLine(W / 2, H / 2 - 5, W / 2, H / 2 + 5, FLinearColor::White);
    const FText Prompt = Player->GetInteractionPrompt();
    if (!Prompt.IsEmpty()) DrawText(Prompt.ToString(), Gold, W / 2 - 80, H / 2 + 34, nullptr, 1.3f);
    for (const auto& Station : Stations)
    {
        const auto* It = Station.Get();
        if (!It) continue;
        const bool Active = (It->bPickup && Mode->GetDeliveryStage() == ENammaDeliveryStage::AwaitingPickup)
            || (!It->bPickup && Mode->GetDeliveryStage() == ENammaDeliveryStage::Carrying);
        if (!Active) continue;
        const float Metres = FVector::Distance(Player->GetActorLocation(), It->GetActorLocation()) / 100.f;
        FVector2D Screen;
        if (PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0, 0, 220), Screen)
            && Screen.X > 20 && Screen.X < W - 150 && Screen.Y > 130 && Screen.Y < H - 80)
            DrawText(FString::Printf(TEXT("%s  %.0f m"), It->bPickup ? TEXT("PICKUP") : TEXT("DELIVER"), Metres), Gold, Screen.X, Screen.Y);
        DrawText(FString::Printf(TEXT("%s: %.0f m | follow gold street markers"), It->bPickup ? TEXT("Namma Tea") : TEXT("Corner Stores"), Gold, 32, 132);
    }
    if (UGameplayStatics::IsGamePaused(this))
    {
        DrawRect(FLinearColor(0.01f, 0.025f, 0.04f, 0.94f), W / 2 - 210, H / 2 - 100, 420, 190);
        DrawText(TEXT("PAUSED"), Gold, W / 2 - 175, H / 2 - 76, nullptr, 2);
        DrawText(TEXT("Esc  Resume\nR      Restart delivery\nQ      Quit game"), FLinearColor::White, W / 2 - 175, H / 2 - 25, nullptr, 1.3f);
    }
}
