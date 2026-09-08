#include "NammaPlayerCharacter.h"
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
    GetCharacterMovement()->RotationRate = FRotator(0, 540, 0);
    GetCharacterMovement()->MaxWalkSpeed = 350;
    GetCharacterMovement()->JumpZVelocity = 480;
    GetCharacterMovement()->AirControl = 0.3f;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 175;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
    // A flat capsule base stops the character sliding off ledge edges it should be
    // standing on, which matters once mantling puts it on top of thin surfaces.
    GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(60.f);
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 360;
    Boom->SocketOffset = FVector(0, 45, 65);
    Boom->bUsePawnControlRotation = true;
    Boom->bDoCollisionTest = true;
    Boom->bEnableCameraLag = true;
    Boom->CameraLagSpeed = 12;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 80;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(0.45, 0.55, 1.1));
    Body->SetRelativeLocation(FVector(0, 0, -20));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
    Head->SetupAttachment(RootComponent);
    Head->SetStaticMesh(Sphere.Object);
    Head->SetRelativeScale3D(FVector(0.4));
    Head->SetRelativeLocation(FVector(0, 0, 55));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Parcel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Parcel"));
    Parcel->SetupAttachment(RootComponent);
    Parcel->SetStaticMesh(Cube.Object);
    Parcel->SetRelativeScale3D(FVector(0.35, 0.45, 0.4));
    Parcel->SetRelativeLocation(FVector(-30, 0, 0));
    Parcel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Parcel->SetVisibility(false);
}

void ANammaPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    // The movement component comes up in MOVE_None on this map, which leaves the pawn
    // inert: no gravity, no walking, no jumping. Mouse look still worked because
    // rotation is controller-side, which made it look like only some keys were broken.
    if (auto* Move = GetCharacterMovement(); Move && Move->MovementMode == MOVE_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("Movement came up in MOVE_None (updated component: %s); forcing MOVE_Walking."),
            *GetNameSafe(Move->UpdatedComponent));
        Move->SetMovementMode(MOVE_Walking);
    }
    StartTransform = GetActorTransform();
    auto ApplyColor = [](UStaticMeshComponent* Part, const TCHAR* Path) {
        if (auto* Material = LoadObject<UMaterialInterface>(nullptr, Path)) Part->SetMaterial(0, Material);
    };
    ApplyColor(Body, TEXT("/Game/NammaCity/Materials/Sandbox/M_Sandbox_teal.M_Sandbox_teal"));
    ApplyColor(Head, TEXT("/Game/NammaCity/Materials/Sandbox/M_Sandbox_cream.M_Sandbox_cream"));
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
    Super::EndPlay(EndPlayReason);
}

void ANammaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
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
    auto* PauseAction = Button(EKeys::Escape);
    PauseAction->bTriggerWhenPaused = true;
    Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::TogglePause);
    auto* RestartAction = Button(EKeys::R);
    RestartAction->bTriggerWhenPaused = true;
    Input->BindAction(RestartAction, ETriggerEvent::Started, this, &ANammaPlayerCharacter::Restart);
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
    if (Controller && !IsPaused() && !bTraversing) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V.Get<float>());
}
void ANammaPlayerCharacter::Right(const FInputActionValue& V)
{
    if (Controller && !IsPaused() && !bTraversing) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V.Get<float>());
}
void ANammaPlayerCharacter::LookYaw(const FInputActionValue& V) { if (!IsPaused()) AddControllerYawInput(V.Get<float>()); }
void ANammaPlayerCharacter::LookPitch(const FInputActionValue& V) { if (!IsPaused()) AddControllerPitchInput(-V.Get<float>()); }
void ANammaPlayerCharacter::SprintStart() { if (!IsPaused()) GetCharacterMovement()->MaxWalkSpeed = 600; }
void ANammaPlayerCharacter::SprintEnd() { GetCharacterMovement()->MaxWalkSpeed = 350; }
void ANammaPlayerCharacter::JumpStart() { if (!IsPaused() && !bTraversing && !TryTraversal()) Jump(); }
void ANammaPlayerCharacter::JumpEnd() { StopJumping(); }
void ANammaPlayerCharacter::CrouchStart() { if (!IsPaused() && !bTraversing) Crouch(); }
void ANammaPlayerCharacter::CrouchEnd() { UnCrouch(); }

// The visible body is a plain static mesh on the capsule, so it does not follow the
// capsule shrinking the way a skeletal mesh would. Squash it to match.
void ANammaPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    Body->SetRelativeScale3D(FVector(0.45, 0.55, 0.73));
    Body->SetRelativeLocation(FVector(0, 0, -13));
    Head->SetRelativeLocation(FVector(0, 0, 25));
}

void ANammaPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    Body->SetRelativeScale3D(FVector(0.45, 0.55, 1.1));
    Body->SetRelativeLocation(FVector(0, 0, -20));
    Head->SetRelativeLocation(FVector(0, 0, 55));
}

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
    if (Forward.IsNearlyZero() || GetCharacterMovement()->IsFalling()) return false;

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
    if (LedgeHeight < MinLedgeHeight || LedgeHeight > MaxLedgeHeight) return false;

    // Prefer vaulting: if there is standable ground just beyond the obstacle, cross it
    // rather than stopping on top.
    const FVector Beyond = Top.ImpactPoint + Forward * (Radius * 2.f + 15.f);
    FHitResult Landing;
    if (GetWorld()->LineTraceSingleByChannel(Landing, Beyond + FVector(0, 0, 40.f),
        Beyond - FVector(0, 0, LedgeHeight + 120.f), ECC_Visibility, Params))
    {
        const FVector VaultTo = Landing.ImpactPoint + FVector(0, 0, HalfHeight + 2.f);
        if (CapsuleFitsAt(VaultTo))
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
    SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);

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
void ANammaPlayerCharacter::Restart()
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
    const FVector P = GetActorLocation();
    if (P.Z < -500 || FMath::Abs(P.X) > 6200 || FMath::Abs(P.Y) > 6200) RecoverToStart();
    UpdateFocus();
    const auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    Parcel->SetVisibility(Mode && Mode->GetDeliveryStage() == ENammaDeliveryStage::Carrying);
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
    if (IsPaused()) return;
    UpdateFocus();
    if (auto* Target = Cast<INammaInteractable>(FocusedActor.Get())) Target->Interact(this);
}
