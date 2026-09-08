#include "../NammaBicycle.h"
#include "../NammaBicycleMovement.h"
#include "../NammaBicyclePhysics.h"
#include "../NammaPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace NB = NammaBicycle;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNammaBicycleWorldTest, "NammaCity.Bicycle.RideAndDrivetrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNammaBicycleWorldTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Values);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);

    auto Block = [World](const FVector& Centre, const FVector& Size)
    {
        auto* Actor = World->SpawnActor<AStaticMeshActor>(Centre, FRotator::ZeroRotator);
        Actor->GetStaticMeshComponent()->SetStaticMesh(
            LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Actor->SetActorScale3D(Size / 100.f);
        return Actor;
    };
    // A road big enough that a full-lock turn cannot ride off the edge of it, plus a
    // speed breaker for the suspension checks.
    Block(FVector(0, 0, -25), FVector(40000, 40000, 50));
    Block(FVector(4000, 0, 6), FVector(40, 1200, 12))->Tags.Add(FName("NammaSurface.asphalt"));

    auto* Bicycle = World->SpawnActor<ANammaBicycle>(FVector(0, 0, 70), FRotator::ZeroRotator);
    auto* Human = World->SpawnActor<ANammaPlayerCharacter>(FVector(-700, 0, 100), FRotator::ZeroRotator);
    auto* Controller = World->SpawnActor<APlayerController>();
    Controller->SetAsLocalPlayerController();
    Controller->Possess(Human);
    World->InitializeActorsForPlay(FURL());
    World->GetWorldSettings()->NotifyBeginPlay();
    World->GetWorldSettings()->NotifyMatchStarted();
    World->BeginPlay();
    Human->Restart();
    Human->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    auto Tick = [&](int32 Count) { for (int32 I = 0; I < Count; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 60.f); } };
    auto* Movement = Bicycle->GetBicycleMovement();
    // Each phase starts from a known place and speed so one failure cannot cascade.
    auto Place = [&](float X)
    {
        Movement->ResetTo(FTransform(FRotator::ZeroRotator, FVector(X, 0.f, 70.f)));
        Bicycle->Controls = NB::FRiderInput();
        Tick(30);
    };
    auto RunUpTo = [&](float Kph, bool bSprint = false)
    {
        Bicycle->Controls.Pedal = 1.0;
        Bicycle->Controls.bSprint = bSprint;
        for (int32 I = 0; I < 1800 && Movement->GetSpeedKph() < Kph; ++I) Tick(1);
        Bicycle->Controls.Pedal = 0.0;
        Bicycle->Controls.bSprint = false;
    };
    const NB::FSetup& Setup = Movement->GetSetup();
    Tick(60);

    // ---- the parked bike sits on its wheels -------------------------------
    TestTrue(TEXT("Parked bike settles at ride height"),
        FMath::Abs(Bicycle->GetActorLocation().Z - Movement->RestHeightCm()) < 6.f);
    TestTrue(TEXT("Parked bike stays put"), FMath::Abs(Movement->GetSpeedKph()) < 0.5f);
    TestTrue(TEXT("Both wheels are on the ground"),
        Movement->GetFrontGround().bContact && Movement->GetRearGround().bContact);
    TestTrue(TEXT("Chain is built from real links"),
        Bicycle->Chain->GetInstanceCount() > 95 && Bicycle->Chain->GetInstanceCount() < 115);

    // ---- mounting -----------------------------------------------------------
    TestFalse(TEXT("Cannot mount from across the street"), Bicycle->CanInteract(Human));
    Human->SetActorLocation(Bicycle->GetActorLocation() + FVector(0, -120, 30));
    Tick(2);
    TestTrue(TEXT("Can mount from beside the bike"), Bicycle->CanInteract(Human));
    TestTrue(TEXT("Mount accepted"), Bicycle->Mount(Human));
    TestFalse(TEXT("A second rider is rejected"), Bicycle->Mount(Human));
    TestTrue(TEXT("Controller possesses the bicycle"), Controller->GetPawn() == Bicycle);
    TestTrue(TEXT("Rider is attached to the bicycle"), Human->GetAttachParentActor() == Bicycle);
    TestTrue(TEXT("Rider reports riding"), Human->IsRiding());
    Tick(30);
    FNammaRidePose Pose;
    TestTrue(TEXT("Ride pose is published"), Human->GetRidePose(Pose));
    TestTrue(TEXT("Rider's hips sit on the saddle"),
        FVector::Dist(Human->GetMesh()->GetSocketLocation(TEXT("pelvis")), Pose.Saddle) < 22.f);

    // ---- pedalling ----------------------------------------------------------
    Place(0.f);
    Bicycle->Controls.Pedal = 1.0;
    Bicycle->SelectGear(2);
    const FVector Launch = Bicycle->GetActorLocation();
    const double ChainAtLaunch = Movement->GetState().ChainTravel;
    Tick(300);
    const NB::FState& State = Movement->GetState();
    const NB::FTelemetry& Telemetry = Movement->GetTelemetry();
    AddInfo(FString::Printf(TEXT("After 5 s pedalling: %.1f km/h, cadence %.0f rpm, chain %.2f m/s, travelled %.1f m"),
        Movement->GetSpeedKph(), Telemetry.Cadence, Telemetry.Chain.Speed,
        FVector::Dist(Launch, Bicycle->GetActorLocation()) / 100.f));
    TestTrue(TEXT("Pedalling moves the bike forward"),
        (Bicycle->GetActorLocation() - Launch).X > 400.f);
    TestTrue(TEXT("Pedalling reaches a plausible speed"),
        Movement->GetSpeedKph() > 8.f && Movement->GetSpeedKph() < 40.f);
    TestFalse(TEXT("Driven bike is not freewheeling"), Telemetry.bFreewheeling);
    TestTrue(TEXT("Rider is producing power"), Telemetry.RiderPower > 50.0);

    // The chain, cranks and rear wheel are one rigid loop.
    // The chain runs at chainring surface speed, roughly a tenth of road speed, so a
    // 16 m ride pulls a couple of metres of chain through the drivetrain.
    TestTrue(TEXT("Chain is travelling"), State.ChainTravel - ChainAtLaunch > 1.0);
    TestTrue(TEXT("Chain speed is the chainring surface speed"),
        FMath::IsNearlyEqual(Telemetry.Chain.Speed, State.CrankRate * Telemetry.Chain.ChainringRadius, 1e-9));
    TestTrue(TEXT("Chain speed is also the sprocket surface speed"),
        FMath::IsNearlyEqual(Telemetry.Chain.Speed, State.RearWheelRate * Telemetry.Chain.SprocketRadius, 1e-9));
    TestTrue(TEXT("Rear wheel surface speed tracks road speed"),
        FMath::Abs(State.RearWheelRate * Setup.WheelRadius - State.Speed) < 0.4);

    // Every drawn link sits on the drivetrain, between the bottom bracket and the axle.
    bool bChainPlaced = true;
    for (int32 Link = 0; Link < Bicycle->Chain->GetInstanceCount(); ++Link)
    {
        FTransform Instance;
        Bicycle->Chain->GetInstanceTransform(Link, Instance, false);
        const FVector At = Instance.GetLocation();
        if (At.ContainsNaN() || At.X > 20.f || At.X < -70.f || FMath::Abs(At.Z + 37.f) > 20.f)
            bChainPlaced = false;
    }
    TestTrue(TEXT("Chain links stay on the chainring, sprocket and runs"), bChainPlaced);

    // ---- the rider is actually pedalling -----------------------------------
    Human->GetRidePose(Pose);
    const FVector FootLeft = Human->GetMesh()->GetSocketLocation(TEXT("foot_l"));
    const FVector FootRight = Human->GetMesh()->GetSocketLocation(TEXT("foot_r"));
    TestTrue(TEXT("Left foot is on the left pedal"), FVector::Dist(FootLeft, Pose.PedalLeft) < 14.f);
    TestTrue(TEXT("Right foot is on the right pedal"), FVector::Dist(FootRight, Pose.PedalRight) < 14.f);
    TestTrue(TEXT("Feet are at different crank positions"), FMath::Abs(FootLeft.Z - FootRight.Z) > 3.f);
    TestTrue(TEXT("Left hand is on the grip"),
        FVector::Dist(Human->GetMesh()->GetSocketLocation(TEXT("hand_l")), Pose.GripLeft) < 22.f);
    TestTrue(TEXT("Right hand is on the grip"),
        FVector::Dist(Human->GetMesh()->GetSocketLocation(TEXT("hand_r")), Pose.GripRight) < 22.f);
    // Manny faces +Y in component space, so a forward fold moves the head that way.
    const FTransform ToMesh = Human->GetMesh()->GetComponentTransform();
    TestTrue(TEXT("Torso folds forward over the bars"),
        ToMesh.InverseTransformPosition(Human->GetMesh()->GetSocketLocation(TEXT("head"))).Y
        > ToMesh.InverseTransformPosition(Human->GetMesh()->GetSocketLocation(TEXT("pelvis"))).Y + 2.f);
    bool bFinitePose = true;
    for (const FTransform& Bone : Human->GetMesh()->GetComponentSpaceTransforms())
        if (Bone.ContainsNaN()) bFinitePose = false;
    TestTrue(TEXT("Riding pose stays finite"), bFinitePose);

    // The pedals themselves stay level however far the cranks have turned.
    TestTrue(TEXT("Pedal platforms stay level"),
        FMath::Abs(Bicycle->PedalLeft->GetComponentRotation().Pitch
                 - Bicycle->VisualRoot->GetComponentRotation().Pitch) < 2.f);

    // ---- steering and lean --------------------------------------------------
    Bicycle->Controls.Steer = 1.0;
    Tick(90);
    TestTrue(TEXT("Steering right leans the bike right"), Movement->GetLeanDegrees() > 3.f);
    TestTrue(TEXT("Leaning right drops the right side"),
        Bicycle->VisualRoot->GetRightVector().Z < -0.05f);
    TestTrue(TEXT("Steering right turns the bike right"),
        FRotator::NormalizeAxis(Bicycle->GetActorRotation().Yaw) > 5.f);
    Bicycle->Controls.Steer = 0.0;
    Tick(90);
    TestTrue(TEXT("Releasing the bars stands the bike back up"),
        FMath::Abs(Movement->GetLeanDegrees()) < 8.f);

    // ---- braking ------------------------------------------------------------
    Place(-5500.f);
    RunUpTo(20.f);
    Bicycle->Controls.FrontBrake = 0.35;
    Bicycle->Controls.RearBrake = 0.6;
    const FVector BrakeFrom = Bicycle->GetActorLocation();
    const float EntrySpeed = Movement->GetSpeedKph();
    float PeakDeceleration = 0.f;
    for (int32 I = 0; I < 900 && Movement->GetSpeedKph() > 0.2f; ++I)
    {
        Tick(1);
        PeakDeceleration = FMath::Max(PeakDeceleration, float(-Movement->GetTelemetry().Acceleration));
    }
    const float Distance = FVector::Dist(BrakeFrom, Bicycle->GetActorLocation()) / 100.f;
    AddInfo(FString::Printf(TEXT("Braked from %.1f km/h in %.2f m, peak %.2f m/s2"),
        EntrySpeed, Distance, PeakDeceleration));
    TestTrue(TEXT("Brakes stop the bike"), Movement->GetSpeedKph() <= 0.2f);
    // 20 km/h is the class-relevant braking test in VEH-01-002 section 25.
    TestTrue(TEXT("Braking distance is believable"), Distance > 1.5f && Distance < 6.f);
    TestTrue(TEXT("Deceleration stays inside the friction limit"), PeakDeceleration < 0.85f * 9.81f);
    TestFalse(TEXT("A firm stop does not crash"), Movement->IsCrashed());
    Bicycle->Controls.FrontBrake = Bicycle->Controls.RearBrake = 0.0;

    // ---- speed breaker ------------------------------------------------------
    Place(0.f);
    Bicycle->SelectGear(3);
    RunUpTo(16.f);
    const float SettledHeight = float(Bicycle->GetActorLocation().Z);
    Bicycle->Controls.Pedal = 0.45;
    float LeastLoad = 1e9f;
    float HighestPoint = 0.f;
    bool bReachedBreaker = false;
    for (int32 I = 0; I < 1200 && Bicycle->GetActorLocation().X < 4600.f; ++I)
    {
        Tick(1);
        // Only sample while the bike is actually on the breaker.
        if (FMath::Abs(Bicycle->GetActorLocation().X - 4000.f) < 180.f)
        {
            bReachedBreaker = true;
            LeastLoad = FMath::Min(LeastLoad, float(Movement->GetTelemetry().FrontLoad));
            HighestPoint = FMath::Max(HighestPoint, float(Bicycle->GetActorLocation().Z));
        }
    }
    AddInfo(FString::Printf(TEXT("Speed breaker: settled %.1f cm, peak %.1f cm, lowest front load %.0f N, now at x=%.0f"),
        SettledHeight, HighestPoint, LeastLoad, Bicycle->GetActorLocation().X));
    TestTrue(TEXT("Bike reaches the speed breaker"), bReachedBreaker);
    TestTrue(TEXT("Speed breaker crosses without falling through the world"),
        Bicycle->GetActorLocation().Z > 40.f);
    TestTrue(TEXT("Speed breaker lifts the bike"), HighestPoint > SettledHeight + 3.f);
    TestTrue(TEXT("Speed breaker unloads a wheel"), LeastLoad < 250.f);
    TestTrue(TEXT("Bike carries on past the breaker"), Bicycle->GetActorLocation().X > 4600.f);
    TestFalse(TEXT("A speed breaker at 16 km/h does not crash"), Movement->IsCrashed());
    Bicycle->Controls.Pedal = 0.0;

    // ---- dismount -----------------------------------------------------------
    Place(0.f);
    const FVector Parked = Bicycle->GetActorLocation();
    Bicycle->Dismount(false);
    Tick(30);
    AddInfo(FString::Printf(TEXT("Dismount: bike at %s, rider at %s, %.0f cm apart"),
        *Parked.ToCompactString(), *Human->GetActorLocation().ToCompactString(),
        FVector::Dist2D(Human->GetActorLocation(), Parked)));
    TestFalse(TEXT("Bike has no rider after dismount"), Bicycle->HasRider());
    TestFalse(TEXT("Rider is no longer riding"), Human->IsRiding());
    TestTrue(TEXT("Controller possesses the rider again"), Controller->GetPawn() == Human);
    TestNull(TEXT("Rider is detached"), Human->GetAttachParentActor());
    TestTrue(TEXT("Rider steps off beside the bike"),
        FVector::Dist2D(Human->GetActorLocation(), Parked) < 220.f);
    TestTrue(TEXT("Rider lands on their feet"), Human->GetCharacterMovement()->IsMovingOnGround());
    TestFalse(TEXT("A clean dismount does not ragdoll"), Human->bRagdoll);
    TestTrue(TEXT("Rider stands upright again"), FMath::Abs(Human->GetActorRotation().Roll) < 1.f);
    for (int32 I = 0; I < 30; ++I) { Human->AddMovementInput(FVector(1, 0, 0), 1.f); Tick(1); }
    TestTrue(TEXT("Rider can walk again"), Human->GetVelocity().SizeSquared() > 100.f);

    // ---- crashing throws the rider -----------------------------------------
    Human->SetActorLocation(Bicycle->GetActorLocation() + FVector(0, -120, 30));
    Tick(2);
    TestTrue(TEXT("Can remount after stepping off"), Bicycle->Mount(Human));
    Place(-5500.f);
    Bicycle->SelectGear(4);
    RunUpTo(24.f, true);
    AddInfo(FString::Printf(TEXT("Crash run-up reached %.1f km/h at x=%.0f"),
        Movement->GetSpeedKph(), Bicycle->GetActorLocation().X));
    TestTrue(TEXT("Back up to speed before the crash"), Movement->GetSpeedKph() > 20.f);
    // Threshold braking on the front alone: hard enough to work the tyre near peak
    // grip, which is exactly the case that lifts the rear and pitches the rider over.
    Bicycle->Controls.FrontBrake = 0.75;
    for (int32 I = 0; I < 900 && !Movement->IsCrashed(); ++I) Tick(1);
    AddInfo(FString::Printf(TEXT("Crash at v=%.2f m/s pitch=%.3f lean=%.3f slide=%.2f pawnvel=%.0f rider=%d"),
        Movement->GetState().Speed, Movement->GetState().Pitch, Movement->GetState().Lean,
        Movement->GetState().SlideTime, Bicycle->GetVelocity().Size(), Bicycle->HasRider() ? 1 : 0));
    TestTrue(TEXT("Grabbing the front brake pitches the rider over"), Movement->IsCrashed());
    Tick(30);
    AddInfo(FString::Printf(TEXT("After crash: rider ragdoll=%d at %s, bike at %s"),
        Human->bRagdoll ? 1 : 0, *Human->GetActorLocation().ToCompactString(),
        *Bicycle->GetActorLocation().ToCompactString()));
    TestFalse(TEXT("Crash throws the rider off"), Bicycle->HasRider());
    TestTrue(TEXT("Crash ragdolls the rider"), Human->bRagdoll);
    TestTrue(TEXT("The bike itself reports a speed"), Bicycle->GetVelocity().SizeSquared() >= 0.f);
    TestTrue(TEXT("Controller follows the thrown rider"), Controller->GetPawn() == Human);
    TestTrue(TEXT("The bike is left lying on its side"),
        FMath::Abs(Movement->GetLeanDegrees()) > 40.f);
    Human->RecoverToStart();
    Tick(60);
    TestFalse(TEXT("Recovery clears the ragdoll"), Human->bRagdoll);

    // ---- picking a downed bike back up -------------------------------------
    Human->SetActorLocation(Bicycle->GetActorLocation() + FVector(0, -120, 30));
    Tick(2);
    TestTrue(TEXT("A downed bike can be picked up"), Bicycle->Mount(Human));
    Tick(30);
    TestFalse(TEXT("Picking the bike up clears the crash"), Movement->IsCrashed());
    TestTrue(TEXT("The bike stands back up"), FMath::Abs(Movement->GetLeanDegrees()) < 8.f);
    Bicycle->Controls.Pedal = 1.0;
    Tick(180);
    TestTrue(TEXT("It rides again after a crash"), Movement->GetSpeedKph() > 5.f);

    // A locked front wheel skids instead: less grip than threshold braking, so it
    // stops longer and does not lift the rear as hard.
    Place(-5500.f);
    RunUpTo(20.f);
    Bicycle->Controls.FrontBrake = 1.0;
    const FVector SkidFrom = Bicycle->GetActorLocation();
    bool bSawFrontLock = false;
    double WorstRatio = 0.0;
    double WorstSlip = 0.0;
    for (int32 I = 0; I < 900 && Movement->GetSpeedKph() > 0.5f; ++I)
    {
        Tick(1);
        const NB::FTelemetry& Skid = Movement->GetTelemetry();
        if (!Skid.bFrontLocked) continue;
        bSawFrontLock = true;
        WorstSlip = FMath::Min(WorstSlip, Skid.FrontSlip);
        // A tyre past its peak slip gives up grip: a locked wheel cannot reach mu*N.
        // Only while the slip is saturated. Below about 0.3 m/s the regularised slip
        // ratio sweeps back through the tyre's peak on its way to zero, which is a
        // property of the formulation rather than of the locked wheel.
        if (Skid.FrontLoad > 50.0 && Skid.FrontSlip < -0.9)
            WorstRatio = FMath::Max(WorstRatio,
                FMath::Abs(Skid.FrontTyreForce) / (Movement->GetSurfaceFriction() * Skid.FrontLoad));
    }
    const float SkidDistance = FVector::Dist(SkidFrom, Bicycle->GetActorLocation()) / 100.f;
    AddInfo(FString::Printf(TEXT("Locked front wheel skidded %.2f m from 20 km/h, worst slip %.2f, peak force ratio %.3f, mu %.2f"),
        SkidDistance, WorstSlip, WorstRatio, Movement->GetSurfaceFriction()));
    TestTrue(TEXT("A full front grab locks the wheel"), bSawFrontLock);
    TestTrue(TEXT("A locked wheel is fully sliding"), WorstSlip < -0.9);
    TestTrue(TEXT("A fully sliding tyre delivers less than peak grip"),
        WorstRatio > 0.0 && WorstRatio < 0.9);
    TestTrue(TEXT("The skid still stops the bike in a believable distance"),
        SkidDistance > 1.f && SkidDistance < 6.f);

    World->EndPlay(EEndPlayReason::Quit);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
