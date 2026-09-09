#include "NammaPlayerCharacter.h"
#include "NammaBicycle.h"
#include "NammaHumanAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "NammaCityGameModeBase.h"
#include "NammaInteractable.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

ANammaPlayerCharacter::ANammaPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 360, 0);
    // Unreal units are centimetres, seconds and kilograms. 4.2 m/s launch speed
    // under 9.81 m/s^2 gravity gives a 0.90 m apex and 0.86 s total airtime.
    GetCharacterMovement()->Mass = 75.f;
    GetCharacterMovement()->MaxAcceleration = 1000.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
    GetCharacterMovement()->bUseSeparateBrakingFriction = true;
    GetCharacterMovement()->BrakingFriction = 0.f;
    GetCharacterMovement()->GroundFriction = 6.f;
    GetCharacterMovement()->MaxStepHeight = 35.f;
    GetCharacterMovement()->SetWalkableFloorAngle(45.f);
    GetCharacterMovement()->MaxSimulationTimeStep = 1.f / 120.f;
    GetCharacterMovement()->MaxSimulationIterations = 8;
    GetCharacterMovement()->bEnablePhysicsInteraction = true;
    GetCharacterMovement()->InitialPushForceFactor = 100.f;
    GetCharacterMovement()->PushForceFactor = 7500.f;
    GetCharacterMovement()->bPushForceScaledToMass = false;
    GetCharacterMovement()->bTouchForceScaledToMass = false;
    GetCharacterMovement()->TouchForceFactor = 100.f;
    GetCharacterMovement()->MaxWalkSpeed = 350;
    GetCharacterMovement()->JumpZVelocity = 420;
    GetCharacterMovement()->AirControl = 0.15f;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 175;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
    // A flat capsule base stops the character sliding off ledge edges it should be
    // standing on, which matters once mantling puts it on top of thin surfaces.
    GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(60.f);
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 400;
    Boom->SocketOffset = FVector(0, 45, 30);
    Boom->bUsePawnControlRotation = true;
    Boom->bDoCollisionTest = true;
    Boom->bEnableCameraLag = true;
    Boom->CameraLagSpeed = 12;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 80;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Human(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    static ConstructorHelpers::FClassFinder<UAnimInstance> Animation(
        TEXT("/Game/NammaCity/Characters/ABP_NammaHuman"));
    GetMesh()->SetSkeletalMesh(Human.Object);
    GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
    GetMesh()->SetAnimInstanceClass(Animation.Class);
    GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
    PhysicsHandle->LinearStiffness = 1500.f;
    PhysicsHandle->LinearDamping = 200.f;
    PhysicsHandle->AngularStiffness = 1000.f;
    PhysicsHandle->AngularDamping = 100.f;
    Parcel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Parcel"));
    Parcel->SetupAttachment(GetMesh(), TEXT("spine_03"));
    Parcel->SetStaticMesh(Cube.Object);
    Parcel->SetRelativeScale3D(FVector(0.35, 0.45, 0.4));
    Parcel->SetRelativeLocation(FVector(-15, -20, 0));
    Parcel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Parcel->SetVisibility(false);
}

void ANammaPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    StartTransform = GetActorTransform();
    StandingMeshTransform = GetMesh()->GetRelativeTransform();
    auto ApplyColor = [](UStaticMeshComponent* Part, const TCHAR* Path) {
        if (auto* Material = LoadObject<UMaterialInterface>(nullptr, Path)) Part->SetMaterial(0, Material);
    };
    ApplyColor(Parcel, TEXT("/Game/NammaCity/Materials/Sandbox/M_Sandbox_ochre.M_Sandbox_ochre"));
    if (auto* PC = Cast<APlayerController>(Controller))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
        PC->SetControlRotation(FRotator(-12, GetActorRotation().Yaw, 0));
        if (PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->ViewPitchMin = -65;
            PC->PlayerCameraManager->ViewPitchMax = 35;
        }
    }
}

void ANammaPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* PC = Cast<APlayerController>(Controller))
        if (auto* LP = PC->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (Mapping) Subsystem->RemoveMappingContext(Mapping);
    ReleaseObject();
    Super::EndPlay(EndPlayReason);
}

void ANammaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    if (auto* PC = Cast<APlayerController>(Controller))
        if (auto* LP = PC->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (Mapping) Subsystem->RemoveMappingContext(Mapping);
    Mapping = NewObject<UInputMappingContext>(this);
    Actions.Reset();
    auto Axis = [this](const FKey& Positive, const FKey& Negative) {
        auto* Action = NewObject<UInputAction>(this);
        Action->ValueType = EInputActionValueType::Axis1D;
        Actions.Add(Action);
        Mapping->MapKey(Action, Positive);
        if (Negative.IsValid()) Mapping->MapKey(Action, Negative).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
        return Action;
    };
    Input->BindAction(Axis(EKeys::W, EKeys::S), ETriggerEvent::Triggered, this, &ANammaPlayerCharacter::Forward);
    Input->BindAction(Axis(EKeys::D, EKeys::A), ETriggerEvent::Triggered, this, &ANammaPlayerCharacter::Right);
    Input->BindAction(Axis(EKeys::MouseX, FKey()), ETriggerEvent::Triggered, this, &ANammaPlayerCharacter::LookYaw);
    Input->BindAction(Axis(EKeys::MouseY, FKey()), ETriggerEvent::Triggered, this, &ANammaPlayerCharacter::LookPitch);
    auto Button = [this](const FKey& Key) {
        auto* Action = NewObject<UInputAction>(this);
        Actions.Add(Action);
        Mapping->MapKey(Action, Key);
        return Action;
    };
    auto* Sprint = Button(EKeys::LeftShift);
    Input->BindAction(Sprint, ETriggerEvent::Started, this, &ANammaPlayerCharacter::SprintStart);
    Input->BindAction(Sprint, ETriggerEvent::Completed, this, &ANammaPlayerCharacter::SprintEnd);
    Input->BindAction(Sprint, ETriggerEvent::Canceled, this, &ANammaPlayerCharacter::SprintEnd);
    auto* JumpAction = Button(EKeys::SpaceBar);
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::JumpStart);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ANammaPlayerCharacter::JumpEnd);
    auto* CrouchAction = Button(EKeys::C);
    Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::CrouchStart);
    Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ANammaPlayerCharacter::CrouchEnd);
    Input->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &ANammaPlayerCharacter::CrouchEnd);
    Input->BindAction(Button(EKeys::E), ETriggerEvent::Started, this, &ANammaPlayerCharacter::Interact);
    Input->BindAction(Button(EKeys::F), ETriggerEvent::Started, this, &ANammaPlayerCharacter::GrabOrRelease);
    Input->BindAction(Button(EKeys::X), ETriggerEvent::Started, this, &ANammaPlayerCharacter::ToggleRagdoll);
    auto* PauseAction = Button(EKeys::Escape);
    PauseAction->bTriggerWhenPaused = true;
    Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::TogglePause);
    auto* RestartAction = Button(EKeys::R);
    RestartAction->bTriggerWhenPaused = true;
    Input->BindAction(RestartAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::RestartDelivery);
    auto* QuitAction = Button(EKeys::Q);
    QuitAction->bTriggerWhenPaused = true;
    Input->BindAction(QuitAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::Quit);
    if (auto* PC = Cast<APlayerController>(Controller))
        if (auto* LP = PC->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                Subsystem->AddMappingContext(Mapping, 0);
}

bool ANammaPlayerCharacter::IsPaused() const { return UGameplayStatics::IsGamePaused(this); }
void ANammaPlayerCharacter::Forward(const FInputActionValue& V)
{
    if (Controller && !IsPaused() && !bRagdoll && !bTraversing && !IsRiding()) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V.Get<float>());
}
void ANammaPlayerCharacter::Right(const FInputActionValue& V)
{
    if (Controller && !IsPaused() && !bRagdoll && !bTraversing && !IsRiding()) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V.Get<float>());
}
void ANammaPlayerCharacter::LookYaw(const FInputActionValue& V) { if (!IsPaused()) AddControllerYawInput(V.Get<float>()); }
void ANammaPlayerCharacter::LookPitch(const FInputActionValue& V) { if (!IsPaused()) AddControllerPitchInput(-V.Get<float>()); }
void ANammaPlayerCharacter::SprintStart() { if (!IsPaused()) GetCharacterMovement()->MaxWalkSpeed = 600; }
void ANammaPlayerCharacter::SprintEnd() { GetCharacterMovement()->MaxWalkSpeed = 350; }
void ANammaPlayerCharacter::JumpStart() { if (!IsPaused() && !bRagdoll && !bTraversing && !IsRiding() && !TryTraversal()) Jump(); }
void ANammaPlayerCharacter::JumpEnd() { StopJumping(); }
void ANammaPlayerCharacter::CrouchStart() { if (!IsPaused() && !bRagdoll && !bTraversing && !IsRiding()) Crouch(); }
void ANammaPlayerCharacter::CrouchEnd() { UnCrouch(); }

// ACharacter preserves mesh height relative to the floor when the capsule shrinks.
// The post-process pose bends the hips/knees; no mesh scaling is involved.
void ANammaPlayerCharacter::OnStartCrouch(float H, float S) { Super::OnStartCrouch(H, S); }
void ANammaPlayerCharacter::OnEndCrouch(float H, float S) { Super::OnEndCrouch(H, S); }

bool ANammaPlayerCharacter::CapsuleFitsAt(const FVector& Location) const
{
    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaTraversalFit), false, this);
    // Shrink slightly so resting flush against the surface we just measured does not
    // count as a blocking overlap.
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(
        Capsule->GetScaledCapsuleRadius() - 2.f, Capsule->GetScaledCapsuleHalfHeight() - 2.f);
    return !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, Shape, Params);
}

// Looks for a ledge directly ahead. A thin obstacle with clear ground beyond becomes a
// vault; a taller or deeper one the character can stand on becomes a mantle. Anything
// else leaves the jump alone.
bool ANammaPlayerCharacter::TryTraversal()
{
    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const FVector Origin = GetActorLocation();
    const FVector Feet = Origin - FVector(0, 0, HalfHeight);
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    if (Forward.IsNearlyZero() || !GetCharacterMovement()->IsMovingOnGround() || bIsCrouched
        || PhysicsHandle->GetGrabbedComponent()) return false;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaTraversal), false, this);

    // Is something in front, at a height worth climbing?
    FHitResult Wall;
    const FVector ProbeStart = Feet + FVector(0, 0, MinLedgeHeight);
    if (!GetWorld()->LineTraceSingleByChannel(Wall, ProbeStart, ProbeStart + Forward * (Radius + TraversalReach),
        ECC_Visibility, Params))
        return false;

    // Find the top of it by dropping onto the surface just past the face we hit.
    const FVector Above = Wall.ImpactPoint + Forward * 12.f + FVector(0, 0, MaxLedgeHeight + 40.f);
    FHitResult Top;
    if (!GetWorld()->LineTraceSingleByChannel(Top, Above, Above - FVector(0, 0, MaxLedgeHeight + 80.f),
        ECC_Visibility, Params))
        return false;

    const float LedgeHeight = Top.ImpactPoint.Z - Feet.Z;
    if (!GetCharacterMovement()->IsWalkable(Top)) return false;
    if (LedgeHeight < MinLedgeHeight || LedgeHeight > MaxLedgeHeight) return false;

    // Prefer vaulting: if there is standable ground just beyond the obstacle, cross it
    // rather than stopping on top.
    const FVector Beyond = Top.ImpactPoint + Forward * (Radius * 2.f + 15.f);
    FHitResult Landing;
    if (GetWorld()->LineTraceSingleByChannel(Landing, Beyond + FVector(0, 0, 40.f),
        Beyond - FVector(0, 0, LedgeHeight + 120.f), ECC_Visibility, Params))
    {
        const FVector VaultTo = Landing.ImpactPoint + FVector(0, 0, HalfHeight + 2.f);
        if (GetCharacterMovement()->IsWalkable(Landing) && CapsuleFitsAt(VaultTo))
        {
            TraversalStart = Origin;
            TraversalTarget = VaultTo;
            bTraversalIsVault = true;
            TraversalDuration = 0.45f;
            bTraversing = true;
            TraversalAlpha = 0.f;
            GetCharacterMovement()->StopMovementImmediately();
            GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            return true;
        }
    }

    // Otherwise climb onto the ledge itself.
    const FVector MantleTo = Top.ImpactPoint + Forward * (Radius + 10.f) + FVector(0, 0, HalfHeight + 2.f);
    if (!CapsuleFitsAt(MantleTo)) return false;
    TraversalStart = Origin;
    TraversalTarget = MantleTo;
    bTraversalIsVault = false;
    TraversalDuration = 0.55f;
    bTraversing = true;
    TraversalAlpha = 0.f;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    return true;
}

void ANammaPlayerCharacter::TickTraversal(float DeltaSeconds)
{
    if (!bTraversing) return;
    TraversalAlpha = FMath::Clamp(TraversalAlpha + DeltaSeconds / TraversalDuration, 0.f, 1.f);

    FVector Position;
    if (bTraversalIsVault)
    {
        // Arc over the obstacle rather than through it.
        Position = FMath::Lerp(TraversalStart, TraversalTarget, TraversalAlpha);
        Position.Z += FMath::Sin(TraversalAlpha * PI) * (MinLedgeHeight + 25.f);
    }
    else
    {
        // Rise first, then step forward onto the ledge, which reads as a climb.
        const float Rise = FMath::Sqrt(TraversalAlpha);
        const float Reach = TraversalAlpha * TraversalAlpha;
        Position.X = FMath::Lerp(TraversalStart.X, TraversalTarget.X, Reach);
        Position.Y = FMath::Lerp(TraversalStart.Y, TraversalTarget.Y, Reach);
        Position.Z = FMath::Lerp(TraversalStart.Z, TraversalTarget.Z, Rise);
    }
    FHitResult Hit;
    SetActorLocation(Position, true, &Hit);
    if (Hit.bBlockingHit)
    {
        bTraversing = false;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        return;
    }

    if (TraversalAlpha >= 1.f)
    {
        bTraversing = false;
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

void ANammaPlayerCharacter::TogglePause()
{
    SprintEnd();
    StopJumping();
    UGameplayStatics::SetGamePaused(this, !IsPaused());
}
void ANammaPlayerCharacter::RestartDelivery()
{
    auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    if (Mode && (IsPaused() || Mode->GetDeliveryStage() == ENammaDeliveryStage::Complete))
    {
        UGameplayStatics::SetGamePaused(this, false);
        Mode->RestartDelivery();
    }
}
void ANammaPlayerCharacter::Quit()
{
    if (IsPaused()) UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false);
}
void ANammaPlayerCharacter::RecoverToStart()
{
    if (ANammaBicycle* Bicycle = Riding.Get()) Bicycle->Dismount(true);
    ReleaseObject();
    if (bRagdoll)
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        GetMesh()->SetRelativeTransform(StandingMeshTransform);
        GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Boom->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        Boom->SetRelativeLocation(FVector::ZeroVector);
        bRagdoll = false;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    }
    UnCrouch();
    GetCharacterMovement()->StopMovementImmediately();
    SprintEnd();
    StopJumping();
    bTraversing = false;
    if (GetCharacterMovement()->MovementMode == MOVE_Flying) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (Controller) Controller->SetControlRotation(FRotator(-12, StartTransform.Rotator().Yaw, 0));
    FocusedActor.Reset();
}
void ANammaPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TickTraversal(DeltaSeconds);
    TickHeldObject();
    if (bRagdoll)
    {
        if (GetMesh()->GetComponentLocation().Z < -500.f) RecoverToStart();
        return;
    }
    const auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    Parcel->SetVisibility(Mode && Mode->GetDeliveryStage() == ENammaDeliveryStage::Carrying);
    if (IsRiding()) return;
    const FVector P = GetActorLocation();
    if (P.Z < -500 || FMath::Abs(P.X) > 6200 || FMath::Abs(P.Y) > 6200) RecoverToStart();
    UpdateFocus();
}
void ANammaPlayerCharacter::UpdateFocus()
{
    FocusedActor.Reset();
    const FVector Origin = GetActorLocation();
    const FVector Eye = Origin + FVector(0, 0, 60);
    // Facing comes from where the player is looking, flattened to the horizontal
    // plane so looking up or down does not narrow the cone.
    const FVector Facing = FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaInteraction), false, this);
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeSphere(InteractionReach), Params);

    // Best candidate is the one closest to straight ahead, so two adjacent targets
    // resolve predictably instead of by iteration order.
    float BestDot = FMath::Cos(FMath::DegreesToRadians(InteractionHalfAngle));
    AActor* Best = nullptr;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        auto* Target = Cast<INammaInteractable>(Candidate);
        if (!Target || Candidate == Best || !Target->CanInteract(this)) continue;
        FVector ToTarget = Candidate->GetActorLocation() - Origin;
        ToTarget.Z = 0;
        if (!ToTarget.Normalize()) continue;
        const float Dot = FVector::DotProduct(Facing, ToTarget);
        if (Dot < BestDot) continue;
        // Line of sight from the player's own eye, not the camera, so a wall between
        // the two still blocks while the trailing camera does not.
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Candidate->GetActorLocation(), ECC_Visibility, Params)
            && Hit.GetActor() != Candidate) continue;
        BestDot = Dot;
        Best = Candidate;
    }
    if (Best) FocusedActor = Best;
}
FText ANammaPlayerCharacter::GetInteractionPrompt() const
{
    if (auto* Target = Cast<INammaInteractable>(FocusedActor.Get()))
        if (Target->CanInteract(this)) return Target->GetInteractionPrompt();
    return FText::GetEmpty();
}
void ANammaPlayerCharacter::Interact()
{
    if (IsPaused() || bRagdoll || bTraversing || IsRiding()) return;
    UpdateFocus();
    if (auto* Target = Cast<INammaInteractable>(FocusedActor.Get())) Target->Interact(this);
}

bool ANammaPlayerCharacter::GetHandTarget(FVector& WorldTarget) const
{
    if (bRagdoll || !PhysicsHandle->GetGrabbedComponent()) return false;
    FRotator Rotation;
    PhysicsHandle->GetTargetLocationAndRotation(WorldTarget, Rotation);
    return true;
}

void ANammaPlayerCharacter::ReleaseObject()
{
    if (auto* Held = PhysicsHandle->GetGrabbedComponent())
    {
        GetCapsuleComponent()->IgnoreComponentWhenMoving(Held, false);
        Held->IgnoreActorWhenMoving(this, false);
        PhysicsHandle->ReleaseComponent();
    }
}

void ANammaPlayerCharacter::GrabOrRelease()
{
    if (IsPaused() || bRagdoll || bTraversing || IsRiding()) return;
    if (PhysicsHandle->GetGrabbedComponent()) { ReleaseObject(); return; }
    const FVector Start = GetMesh()->GetSocketLocation(TEXT("spine_03"));
    const FVector Direction = GetControlRotation().Vector();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaGrab), false, this);
    // Visibility sweep stops at walls, including static walls before a movable prop.
    if (!GetWorld()->SweepSingleByChannel(Hit, Start, Start + Direction * 160.f, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(25.f), Params)) return;
    UPrimitiveComponent* Part = Hit.GetComponent();
    if (!Part || !Part->IsSimulatingPhysics(Hit.BoneName) || Part->GetMass() > 20.f) return;
    PhysicsHandle->GrabComponentAtLocationWithRotation(Part, Hit.BoneName, Hit.ImpactPoint, Part->GetComponentRotation());
    GetCapsuleComponent()->IgnoreComponentWhenMoving(Part, true);
    Part->IgnoreActorWhenMoving(this, true);
}

void ANammaPlayerCharacter::TickHeldObject()
{
    auto* Held = PhysicsHandle->GetGrabbedComponent();
    if (!Held) return;
    const FVector Shoulder = GetMesh()->GetSocketLocation(TEXT("upperarm_r"));
    const FVector Target = Shoulder + GetActorForwardVector() * 55.f + FVector(0, 0, -20.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaHold), false, this);
    Params.AddIgnoredComponent(Held);
    FHitResult Hit;
    if (FVector::DistSquared(Held->GetComponentLocation(), Shoulder) > FMath::Square(220.f)
        || GetWorld()->LineTraceSingleByChannel(Hit, Shoulder, Target, ECC_Visibility, Params))
    {
        ReleaseObject();
        return;
    }
    PhysicsHandle->SetTargetLocationAndRotation(Target, GetActorRotation());
}

void ANammaPlayerCharacter::ToggleRagdoll()
{
    if (IsPaused() || IsRiding()) return;
    // Recovery is an explicit safe reset, not a fabricated get-up animation.
    if (bRagdoll) { RecoverToStart(); return; }
    if (bTraversing) return;
    EnterRagdoll(GetVelocity());
}

void ANammaPlayerCharacter::EnterRagdoll(const FVector& Momentum)
{
    if (bRagdoll || !GetMesh()->GetPhysicsAsset()) return;
    ReleaseObject();
    SprintEnd();
    StopJumping();
    UnCrouch();
    bRagdoll = true;
    FocusedActor.Reset();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->SetAllPhysicsLinearVelocity(Momentum);
    Boom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, TEXT("pelvis"));
    Boom->SetRelativeLocation(FVector::ZeroVector);
}

void ANammaPlayerCharacter::BeginRiding(ANammaBicycle* Bicycle)
{
    ReleaseObject();
    SprintEnd();
    StopJumping();
    UnCrouch();
    bTraversing = false;
    FocusedActor.Reset();
    Riding = Bicycle;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    // The bicycle carries the rider; a second colliding capsule inside its hull would
    // only fight the wheel traces.
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (auto* PC = Cast<APlayerController>(Controller))
        if (auto* LP = PC->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (Mapping) Subsystem->RemoveMappingContext(Mapping);
}

void ANammaPlayerCharacter::EndRiding(const FVector& Where, const FVector& Momentum, bool bThrown)
{
    Riding.Reset();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    // Detaching keeps the bike's lean and pitch, so put the rider back upright.
    SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
    SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    if (bThrown || !Momentum.IsNearlyZero()) EnterRagdoll(Momentum);
}

bool ANammaPlayerCharacter::GetRidePose(FNammaRidePose& Out) const
{
    const ANammaBicycle* Bicycle = Riding.Get();
    return !bRagdoll && Bicycle && Bicycle->GetRidePose(Out);
}
