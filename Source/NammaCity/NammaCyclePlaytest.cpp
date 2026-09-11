// Opt-in Development-build smoke test. Exercises the same Enhanced Input key path
// as a player; never runs during ordinary gameplay or in Shipping builds.
#if !UE_BUILD_SHIPPING
#include "NammaBicycle.h"
#include "NammaBicycleMovement.h"
#include "NammaPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace
{
struct FCyclePlaytest : TSharedFromThis<FCyclePlaytest>
{
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<APlayerController> PC;
    TWeakObjectPtr<ANammaPlayerCharacter> Human;
    TWeakObjectPtr<ANammaBicycle> Bike;
    FTimerHandle Timer;
    float Elapsed = 0.f;
    int32 Stage = 0;
    bool Failed = false;
    FVector WalkStart;
    float StandingFootZ = 0.f;
    void Check(bool OK, const TCHAR* Message)
    {
        Failed |= !OK;
        UE_LOG(LogNammaVehicle, Display, TEXT("Cycle input check %s: %s"), OK ? TEXT("PASS") : TEXT("FAIL"), Message);
    }
    void Key(const FKey& Key, bool Down)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Key,
            Down ? IE_Pressed : IE_Released, FPlatformTime::Cycles64()));
    }
    void Tick()
    {
        if (!World.IsValid() || !PC.IsValid() || !Human.IsValid() || !Bike.IsValid()) return;
        Elapsed += .1f;
        switch (Stage)
        {
        case 0: if (Elapsed > .5f) { PC->ConsoleCommand(TEXT("HighResShot 1280x720 filename=/tmp/namma-human-stand.png")); ++Stage; } break;
        case 1: if (Elapsed > .7f) { Key(EKeys::E,true); ++Stage; } break;
        case 2: if (Elapsed > .9f) { Key(EKeys::E,false); ++Stage; } break;
        case 3: if (Elapsed > 1.5f) { Check(Bike->HasRider() && PC->GetPawn() == Bike.Get(), TEXT("E mounts")); Key(EKeys::W,true); ++Stage; } break;
        case 4: if (Elapsed > 3.f) { Key(EKeys::Four,true); ++Stage; } break;
        case 5: if (Elapsed > 3.2f) { Key(EKeys::Four,false); ++Stage; } break;
        case 6: if (Elapsed > 5.f)
        {
            Check(Bike->GetBicycleMovement()->GetSpeedKph() > 5.f, TEXT("W pedals through input mapping"));
            Check(Bike->GetBicycleMovement()->GetState().Gear == 3, TEXT("4 selects gear four"));
            PC->SetControlRotation(FRotator(-10.f, Bike->GetActorRotation().Yaw - 70.f, 0.f));
            ++Stage;
        } break;
        case 7: if (Elapsed > 5.8f)
        {
            PC->ConsoleCommand(TEXT("HighResShot 1280x720 filename=/tmp/namma-cycle-ride.png"));
            ++Stage;
        } break;
        case 8: if (Elapsed > 7.f) { Key(EKeys::W,false); Key(EKeys::S,true); ++Stage; } break;
        case 9: if (Elapsed > 10.5f)
        {
            Check(Bike->GetBicycleMovement()->GetSpeedKph() < 5.f, TEXT("S brakes to dismount speed"));
            Key(EKeys::S,false); Key(EKeys::E,true); ++Stage;
        } break;
        case 10: if (Elapsed > 10.7f) { Key(EKeys::E,false); ++Stage; } break;
        case 11: if (Elapsed > 11.5f)
        {
            Check(!Human->IsRiding() && PC->GetPawn() == Human.Get(), TEXT("E returns control to the human"));
            WalkStart = Human->GetActorLocation(); Key(EKeys::W,true); ++Stage;
        } break;
        case 12: if (Elapsed > 12.f)
        {
            Key(EKeys::W,false);
            Check(FVector::Dist2D(WalkStart, Human->GetActorLocation()) > 30.f, TEXT("walking works after dismount"));
            PC->SetControlRotation((Bike->GetActorLocation() - Human->GetActorLocation()).Rotation());
            ++Stage;
        } break;
        case 13: if (Elapsed > 12.6f) { Key(EKeys::E,true); ++Stage; } break;
        case 14: if (Elapsed > 12.8f) { Key(EKeys::E,false); ++Stage; } break;
        case 15: if (Elapsed > 13.5f)
        {
            Check(Bike->HasRider(), TEXT("remount works without stale input contexts"));
            Key(EKeys::X,true); ++Stage;
        } break;
        case 16: if (Elapsed > 13.7f) { Key(EKeys::X,false); ++Stage; } break;
        case 17: if (Elapsed > 14.5f)
        {
            Check(!Bike->HasRider() && Human->GetMesh()->IsSimulatingPhysics(TEXT("pelvis")), TEXT("X bails into Chaos ragdoll"));
            Key(EKeys::X,true); ++Stage;
        } break;
        case 18: if (Elapsed > 14.7f) { Key(EKeys::X,false); ++Stage; } break;
        case 19: if (Elapsed > 15.5f)
        {
            Check(!Human->IsRiding() && Human->GetCharacterMovement()->IsMovingOnGround(), TEXT("X recovery restores walking"));
            // Back to the open spot the standing frame used: next to the bike
            // the camera boom collides and ends up inside the mesh. A small
            // swing off the shoulder lets elbows and knees read.
            Human->SetActorLocation(Bike->GetActorLocation() - Bike->GetActorRightVector() * 180.f + FVector(0,0,30));
            const float Facing = (Bike->GetActorLocation() - Human->GetActorLocation()).Rotation().Yaw;
            Human->SetActorRotation(FRotator(0.f, Facing, 0.f));
            PC->SetControlRotation(FRotator(-12.f, Facing + 35.f, 0.f));
            ++Stage;
        } break;
        // Combat: a jab through the attack key, then a front kick, each
        // photographed at its template's impact time.
        case 20: if (Elapsed > 16.6f) { StandingFootZ = Human->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z; Key(EKeys::LeftMouseButton,true); ++Stage; } break;
        case 21: if (Elapsed > 16.7f)
        {
            Key(EKeys::LeftMouseButton,false);
            Check(Human->IsStriking() && Human->GetCurrentStrike() == NammaHuman::EStrike::Jab, TEXT("LMB opens with a jab"));
            ++Stage;
        } break;
        case 22: if (Elapsed > 16.6f + NammaHuman::StrikeImpactTime(NammaHuman::EStrike::Jab))
        {
            PC->ConsoleCommand(TEXT("HighResShot 1280x720 filename=/tmp/namma-human-jab.png"));
            ++Stage;
        } break;
        case 23: if (Elapsed > 18.2f) { Check(Human->BeginStrike(NammaHuman::EStrike::FrontKick, nullptr), TEXT("front kick can be thrown")); ++Stage; } break;
        case 24: if (Elapsed > 18.2f + NammaHuman::StrikeImpactTime(NammaHuman::EStrike::FrontKick))
        {
            Check(Human->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z > StandingFootZ + 35.f, TEXT("front kick lifts the foot"));
            PC->ConsoleCommand(TEXT("HighResShot 1280x720 filename=/tmp/namma-human-kick.png"));
            ++Stage;
        } break;
        case 25: if (Elapsed > 19.6f)
        {
            const FString Result = Failed ? TEXT("FAIL") : TEXT("PASS");
            UE_LOG(LogNammaVehicle, Display, TEXT("NAMMA_CYCLE_PLAYTEST_%s"), *Result);
            FFileHelper::SaveStringToFile(Result, *(FPaths::ProjectSavedDir() / TEXT("CyclePlaytest.result")));
            World->GetTimerManager().ClearTimer(Timer);
            FPlatformMisc::RequestExitWithStatus(false, Failed ? 1 : 0);
            ++Stage;
        } break;
        }
    }
};
FAutoConsoleCommandWithWorld Playtest(TEXT("Namma.Cycle.Playtest"),
    TEXT("Run a keyboard-input cycle smoke test, capture standing, riding, jab and kick frames, and exit."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Human = Cast<ANammaPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
        auto* PC = Human ? Cast<APlayerController>(Human->GetController()) : nullptr;
        ANammaBicycle* Bike = nullptr;
        double Best = TNumericLimits<double>::Max();
        if (Human) for (TActorIterator<ANammaBicycle> It(World); It; ++It)
        {
            const double D = FVector::DistSquared(It->GetActorLocation(), Human->GetActorLocation());
            if (D < Best) { Best = D; Bike = *It; }
        }
        if (!PC || !Bike) { UE_LOG(LogNammaVehicle, Error, TEXT("Playtest requires a player and cycle")); return; }
        if (Best > FMath::Square(600.0))
        {
            UE_LOG(LogNammaVehicle, Error, TEXT("NAMMA_CYCLE_PLAYTEST_FAIL: no cycle within six metres of player spawn"));
            FPlatformMisc::RequestExitWithStatus(false, 1);
            return;
        }
        Human->SetActorLocation(Bike->GetActorLocation() - Bike->GetActorRightVector() * 120.f + FVector(0,0,30));
        PC->SetControlRotation((Bike->GetActorLocation() - Human->GetActorLocation()).Rotation());
        const auto Test = MakeShared<FCyclePlaytest>();
        Test->World = World; Test->PC = PC; Test->Human = Human; Test->Bike = Bike;
        World->GetTimerManager().SetTimer(Test->Timer, FTimerDelegate::CreateLambda([Test]() { Test->Tick(); }), .1f, true);
    }));
}
#endif
