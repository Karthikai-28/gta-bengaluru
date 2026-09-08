#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NammaRidePose.h"
#include "NammaPlayerCharacter.generated.h"

class ANammaBicycle;
class UPhysicsHandleComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class NAMMACITY_API ANammaPlayerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ANammaPlayerCharacter();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void RecoverToStart();
    FText GetInteractionPrompt() const;
    bool GetHandTarget(FVector& WorldTarget) const;
    // Riding hands the pawn over to the bicycle: this character keeps its mesh and
    // animation but stops driving itself, and the bicycle possesses the controller.
    void BeginRiding(ANammaBicycle* Bicycle);
    void EndRiding(const FVector& Where, const FVector& Momentum);
    bool IsRiding() const { return Riding.IsValid(); }
    bool GetRidePose(FNammaRidePose& Out) const;
    bool CanStandAt(const FVector& Location) const { return CapsuleFitsAt(Location); }
    void EnterRagdoll(const FVector& Momentum);
private:
    friend class FNammaHumanWorldTest;
    friend class FNammaBicycleWorldTest;
    void ToggleRagdoll();
    void GrabOrRelease();
    void ReleaseObject();
    void TickHeldObject();
    void Forward(const FInputActionValue& Value);
    void Right(const FInputActionValue& Value);
    void LookYaw(const FInputActionValue& Value);
    void LookPitch(const FInputActionValue& Value);
    void SprintStart();
    void SprintEnd();
    void JumpStart();
    void JumpEnd();
    void CrouchStart();
    void CrouchEnd();
    // Traversal: a ledge in front turns the jump key into a vault or a mantle.
    bool TryTraversal();
    void TickTraversal(float DeltaSeconds);
    bool CapsuleFitsAt(const FVector& Location) const;
    void Interact();
    void TogglePause();
    // Named RestartDelivery, not Restart: APawn::Restart() is virtual, so a method
    // with that exact signature silently overrides it. That swallowed the engine's
    // own possession-time Restart and left MovementMode at its zero-initialised
    // MOVE_None, making the pawn inert.
    void RestartDelivery();
    void Quit();
    void UpdateFocus();
    bool IsPaused() const;
    virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsHandleComponent> PhysicsHandle;
    bool bRagdoll = false;
    FTransform StandingMeshTransform;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Parcel;
    UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;
    // Interaction is a facing cone, not a crosshair ray: the player only has to be
    // looking roughly towards a target. Reach is deliberately wider than any
    // individual target's own range check, so those stay authoritative.
    UPROPERTY(EditAnywhere, Category = "Interaction") float InteractionReach = 400.f;
    UPROPERTY(EditAnywhere, Category = "Interaction") float InteractionHalfAngle = 55.f;
    // Ledges below MinLedgeHeight are simply walked over; above MaxLedgeHeight the
    // character has nothing to pull itself up on and the jump stays a plain jump.
    UPROPERTY(EditAnywhere, Category = "Traversal") float MinLedgeHeight = 40.f;
    UPROPERTY(EditAnywhere, Category = "Traversal") float MaxLedgeHeight = 170.f;
    UPROPERTY(EditAnywhere, Category = "Traversal") float TraversalReach = 75.f;
    bool bTraversing = false;
    bool bTraversalIsVault = false;
    float TraversalAlpha = 0.f;
    float TraversalDuration = 0.5f;
    FVector TraversalStart = FVector::ZeroVector;
    FVector TraversalTarget = FVector::ZeroVector;
    TWeakObjectPtr<AActor> FocusedActor;
    TWeakObjectPtr<ANammaBicycle> Riding;
    FTransform StartTransform;
};
