#include "../NammaPlayerCharacter.h"
#include "../NammaHumanAnimInstance.h"
#include "Misc/AutomationTest.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "TwoBoneIK.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNammaHumanWorldTest, "NammaCity.Human.PhysicsAndPose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNammaHumanWorldTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Values);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -25), FRotator::ZeroRotator);
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(100, 100, 0.5));
    auto* Human = World->SpawnActor<ANammaPlayerCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
    auto* Controller = World->SpawnActor<APlayerController>();
    Controller->SetAsLocalPlayerController();
    Controller->Possess(Human);
    World->InitializeActorsForPlay(FURL());
    World->GetWorldSettings()->NotifyBeginPlay();
    World->GetWorldSettings()->NotifyMatchStarted();
    World->BeginPlay();
    // No viewport/net connection exists to deliver ClientRestart in this isolated world.
    Human->Restart();
    auto* Movement = Human->GetCharacterMovement();
    auto* Mesh = Human->GetMesh();
    Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    TestNotNull(TEXT("Human skeletal mesh loaded"), Mesh->GetSkeletalMeshAsset());
    // The art import can land at the wrong unit scale and still load; a mis-sized
    // skin detaches every centimetre tuned below from the character wearing it.
    TestTrue(TEXT("Human mesh is human-sized"), Mesh->GetSkeletalMeshAsset()
        && FMath::IsWithin(Mesh->GetSkeletalMeshAsset()->GetImportedBounds().BoxExtent.Z, 75.f, 105.f));
    TestNotNull(TEXT("Human animation loaded"), Mesh->GetAnimInstance());
    TestTrue(TEXT("Custom IK proxy used"), Mesh->GetAnimInstance() && Mesh->GetAnimInstance()->IsA<UNammaHumanAnimInstance>());
    TestTrue(TEXT("Skeleton includes hands and knees"), Mesh->GetBoneIndex(TEXT("hand_r")) >= 0 && Mesh->GetBoneIndex(TEXT("calf_l")) >= 0);
    TestTrue(TEXT("Ragdoll includes constrained joints"), Mesh->GetPhysicsAsset() && Mesh->GetPhysicsAsset()->ConstraintSetup.Num() > 10);
    TestEqual(TEXT("Human mass in kg"), Movement->Mass, 75.f);
    TestTrue(TEXT("Calculated jump apex about 90 cm"), FMath::IsNearlyEqual(
        FMath::Square(Movement->JumpZVelocity) / (2.f * FMath::Abs(Movement->GetGravityZ())), 89.91f, 1.f));
    auto Tick = [&](int32 Count) { for (int32 I = 0; I < Count; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 60.f); } };
    Tick(30);
    TestTrue(TEXT("Character settles on floor"), Movement->IsMovingOnGround());
    // Imported bounds can be right while the animated skeleton is not: a scaled
    // root bone reads as full size at rest and collapses once animation drives it.
    TestTrue(TEXT("Animated skeleton stands human-sized"),
        FVector::Dist(Mesh->GetSocketLocation(TEXT("head")), Mesh->GetSocketLocation(TEXT("foot_l"))) > 120.f);
    const FVector StandingPelvis = Mesh->GetSocketLocation(TEXT("pelvis"));
    const FVector StandingFoot = Mesh->GetSocketLocation(TEXT("foot_l"));
    Human->Crouch();
    Tick(40);
    AddInfo(FString::Printf(TEXT("Crouch state: wants=%d can=%d mode=%d begun=%d local=%d ticks=%d location=%s"), Movement->bWantsToCrouch, Human->CanCrouch(), int32(Movement->MovementMode), Human->HasActorBegunPlay(), Human->IsLocallyControlled(), Movement->IsComponentTickEnabled(), *Human->GetActorLocation().ToString()));
    TestTrue(TEXT("Crouch accepted"), Human->bIsCrouched);
    TestTrue(TEXT("Crouching lowers pelvis without shrinking bones"), Mesh->GetSocketLocation(TEXT("pelvis")).Z < StandingPelvis.Z - 40.f);
    TestTrue(TEXT("Crouching keeps foot at floor"), FMath::Abs(Mesh->GetSocketLocation(TEXT("foot_l")).Z - StandingFoot.Z) < 10.f);
    for (const FTransform& Bone : Mesh->GetComponentSpaceTransforms())
        if (Bone.ContainsNaN()) { AddError(TEXT("Pose contains non-finite bone transform")); break; }
    Human->UnCrouch();
    Tick(40);
    // A movable body at chest height exercises pickup without relying on camera UI.
    const FVector GrabLocation = Mesh->GetSocketLocation(TEXT("spine_03")) + Controller->GetControlRotation().Vector() * 100.f;
    auto* Crate = World->SpawnActor<AStaticMeshActor>(GrabLocation, FRotator::ZeroRotator);
    auto* CrateBody = Crate->GetStaticMeshComponent();
    CrateBody->SetMobility(EComponentMobility::Movable);
    CrateBody->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Crate->SetActorScale3D(FVector(0.3));
    CrateBody->SetCollisionProfileName(TEXT("PhysicsActor"));
    CrateBody->SetMassOverrideInKg(NAME_None, 60.f);
    CrateBody->SetSimulatePhysics(true);
    Human->GrabOrRelease();
    TestNull(TEXT("Heavy objects cannot be lifted"), Human->PhysicsHandle->GetGrabbedComponent());
    CrateBody->SetMassOverrideInKg(NAME_None, 12.f);
    Human->GrabOrRelease();
    TestTrue(TEXT("Light body grabbed by physics constraint"), Human->PhysicsHandle->GetGrabbedComponent() == CrateBody);
    Tick(15);
    TestTrue(TEXT("Held body retains physics simulation"), CrateBody->IsSimulatingPhysics());
    Human->ReleaseObject();
    TestNull(TEXT("Drop releases constraint"), Human->PhysicsHandle->GetGrabbedComponent());
    Crate->Destroy();
    Human->ToggleRagdoll();
    Tick(30);
    TestTrue(TEXT("Chaos ragdoll active"), Human->bRagdoll && Mesh->IsSimulatingPhysics(TEXT("pelvis")));
    const FVector FallenAt=Mesh->GetSocketLocation(TEXT("pelvis"));
    Human->ToggleRagdoll();
    Tick(180);
    TestTrue(TEXT("Get-up stays near the fall instead of resetting to spawn"), FVector::Dist2D(Human->GetActorLocation(),FallenAt)<150.f);
    TestFalse(TEXT("Recovery exits ragdoll"), Human->bRagdoll);
    TestTrue(TEXT("Recovery restores movement"), Movement->IsMovingOnGround());
    TestTrue(TEXT("Recovered character is eligible to ride"),Human->CanBeginRiding());
    TestEqual(TEXT("Harmless fall preserves health"),Human->GetHealth(),100.f);
    // A standing dummy, not a sparring partner: a partner walks back into
    // punching range after being hit, which would defeat the kicking check.
    auto* Opponent=World->SpawnActor<ANammaPlayerCharacter>(Human->GetActorLocation()+FVector(100,0,0),FRotator(0,180,0));
    Tick(10);
    using namespace NammaHuman;
    Human->Attack();
    TestTrue(TEXT("First strike in punching range is a jab"),Human->IsStriking() && Human->GetCurrentStrike()==EStrike::Jab);
    Tick(FMath::CeilToInt(float(StrikeImpactTime(EStrike::Jab))*60.f));
    const FVector PunchShoulder=Mesh->GetSocketLocation(TEXT("upperarm_l"));
    const FVector PunchElbow=Mesh->GetSocketLocation(TEXT("lowerarm_l"));
    const FVector PunchWrist=Mesh->GetSocketLocation(TEXT("hand_l"));
    const FVector Knuckles=Mesh->GetSocketLocation(TEXT("middle_01_l"));
    const FVector Right=Human->GetActorRightVector();
    AddInfo(FString::Printf(TEXT("Jab at impact: shoulder %s elbow %s wrist %s"),*PunchShoulder.ToString(),*PunchElbow.ToString(),*PunchWrist.ToString()));
    TestTrue(TEXT("Punch wrist stays aligned with the forearm"),FVector::DotProduct(
        (Knuckles-PunchWrist).GetSafeNormal(),(PunchWrist-PunchElbow).GetSafeNormal())>.85f);
    TestTrue(TEXT("Punch keeps a bend at the elbow"),FVector::Dist(PunchShoulder,PunchWrist)
        <.995f*(FVector::Dist(PunchShoulder,PunchElbow)+FVector::Dist(PunchElbow,PunchWrist)));
    TestTrue(TEXT("Punch reaches forward"),FVector::DotProduct(PunchWrist-PunchShoulder,Human->GetActorForwardVector())>35.f);
    // The left elbow must not drift across the chest: it stays on its own side
    // of the fist, and no further inboard of its shoulder than a real guard.
    TestTrue(TEXT("Left elbow stays outside the line to the fist"),
        FVector::DotProduct(PunchElbow-PunchShoulder,Right)<=FVector::DotProduct(PunchWrist-PunchShoulder,Right)+3.f);
    TestTrue(TEXT("Left elbow does not cross inward"),FVector::DotProduct(PunchElbow-PunchShoulder,Right)<12.f);
    Tick(6);
    TestTrue(TEXT("Punch damages a reachable opponent"),Opponent->GetHealth()<100.f);
    const float Stamina=Human->GetStamina();
    Human->Attack();
    TestEqual(TEXT("Attack cooldown prevents repeat spending"),Human->GetStamina(),Stamina);
    // Let the chain lapse, then offer the opponent at kicking range.
    Tick(120);
    Opponent->SetActorLocation(Human->GetActorLocation()+Human->GetActorForwardVector()*125.f);
    Tick(5);
    const float BeforeKick=Opponent->GetHealth();
    const float StandingFootZ=Mesh->GetSocketLocation(TEXT("foot_l")).Z;
    Human->Attack();
    TestTrue(TEXT("Out of punching range the strike is a front kick"),Human->IsStriking() && Human->GetCurrentStrike()==EStrike::FrontKick);
    Tick(FMath::CeilToInt(float(StrikeImpactTime(EStrike::FrontKick))*60.f));
    const FVector KickFoot=Mesh->GetSocketLocation(TEXT("foot_l"));
    AddInfo(FString::Printf(TEXT("Front kick at impact: foot %s hip %s"),*KickFoot.ToString(),*Mesh->GetSocketLocation(TEXT("thigh_l")).ToString()));
    TestTrue(TEXT("Kicking foot lifts off the floor"),KickFoot.Z>StandingFootZ+35.f);
    TestTrue(TEXT("Kicking foot reaches forward"),FVector::DotProduct(KickFoot-Mesh->GetSocketLocation(TEXT("thigh_l")),Human->GetActorForwardVector())>40.f);
    TestTrue(TEXT("Support foot stays planted"),FMath::Abs(Mesh->GetSocketLocation(TEXT("foot_r")).Z-StandingFootZ)<12.f);
    Tick(8);
    TestTrue(TEXT("Kick damages an opponent beyond punching range"),Opponent->GetHealth()<BeforeKick);
    for (const FTransform& Bone : Mesh->GetComponentSpaceTransforms())
        if (Bone.ContainsNaN()) { AddError(TEXT("Strike pose contains non-finite bone transform")); break; }
    Opponent->Destroy();
    Tick(60);
    Human->TakeDamage(150.f,FDamageEvent(),nullptr,nullptr);
    TestTrue(TEXT("Lethal damage leaves character down"),Human->IsDead() && Human->IsDown());
    Tick(180);
    TestFalse(TEXT("Death cannot use ordinary get-up"),Human->TryGetUp());
    TestFalse(TEXT("Dead character cannot mount"),Human->CanBeginRiding());
    Human->RecoverToStart();Tick(60);
    TestEqual(TEXT("Respawn restores health"),Human->GetHealth(),100.f);
    TestTrue(TEXT("Respawn restores movement"),Movement->IsMovingOnGround());
    // Exercise solver singularities independently of the authored animation.
    for (const FVector Target : {FVector::ZeroVector, FVector(0, 0, 500), FVector(0, 20, -60)})
    {
        FVector Knee, Foot;
        AnimationCore::SolveTwoBoneIK(FVector::ZeroVector, FVector(0, 0, -45), FVector(0, 0, -90),
            FVector(0, 100, 0), Target, Knee, Foot, false, 1., 1.);
        TestFalse(TEXT("IK singularities stay finite"), Knee.ContainsNaN() || Foot.ContainsNaN());
        TestTrue(TEXT("IK preserves thigh length"), FMath::IsNearlyEqual(Knee.Length(), 45., 0.1));
        TestTrue(TEXT("IK preserves calf length"), FMath::IsNearlyEqual((Foot - Knee).Length(), 45., 0.1));
    }
    World->EndPlay(EEndPlayReason::Quit);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
