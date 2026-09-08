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
    if (Controller && !IsPaused()) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V.Get<float>());
}
void ANammaPlayerCharacter::Right(const FInputActionValue& V)
{
    if (Controller && !IsPaused()) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V.Get<float>());
}
void ANammaPlayerCharacter::LookYaw(const FInputActionValue& V) { if (!IsPaused()) AddControllerYawInput(V.Get<float>()); }
void ANammaPlayerCharacter::LookPitch(const FInputActionValue& V) { if (!IsPaused()) AddControllerPitchInput(-V.Get<float>()); }
void ANammaPlayerCharacter::SprintStart() { if (!IsPaused()) GetCharacterMovement()->MaxWalkSpeed = 600; }
void ANammaPlayerCharacter::SprintEnd() { GetCharacterMovement()->MaxWalkSpeed = 350; }
void ANammaPlayerCharacter::JumpStart() { if (!IsPaused()) Jump(); }
void ANammaPlayerCharacter::JumpEnd() { StopJumping(); }

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
    SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (Controller) Controller->SetControlRotation(FRotator(-12, StartTransform.Rotator().Yaw, 0));
    FocusedActor.Reset();
}
void ANammaPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
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
