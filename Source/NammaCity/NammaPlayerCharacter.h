#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NammaRidePose.h"
#include "NammaVitals.h"
#include "NammaPunchMotion.h"
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
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    void TogglePerspective();
    bool IsFirstPerson() const { return bFirstPerson && !bRagdoll; }
    void RecoverToStart();
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    virtual void Landed(const FHitResult& Hit) override;
    float GetHealth() const { return float(Vitals.Health); }
    float GetStamina() const { return float(Vitals.Stamina); }
    bool IsDown() const { return bRagdoll; }
    bool IsDead() const { return !Vitals.Alive(); }
    float GetRecoveryDepth() const { return 55.f * FMath::Clamp(RecoveryTime / .8f, 0.f, 1.f); }
    bool TryGetUp();
    bool GetCombatHands(FVector& Left, FVector& Right) const;
    NammaHuman::FPunchMotion GetPunchMotion() const;
    bool IsLeftPunch() const { return bLeftPunch; }
    void SetSparringPartner();
    bool IsSparringPartner() const { return bSparringPartner; }
    FText GetInteractionPrompt() const;
    bool GetHandTarget(FVector& WorldTarget) const;
    // Riding hands the pawn over to the bicycle: this character keeps its mesh and
    // animation but stops driving itself, and the bicycle possesses the controller.
    void BeginRiding(ANammaBicycle* Bicycle);
    void EndRiding(const FVector& Where, const FVector& Momentum, bool bThrown = false);
    bool IsRiding() const { return Riding.IsValid(); }
    bool GetRidePose(FNammaRidePose& Out) const;
    bool CanBeginRiding() const { return Vitals.Alive() && RecoveryTime <= 0 && AttackTime <= 0 && !bRagdoll && !bTraversing && !bIsCrouched && !IsRiding() && !IsPaused(); }
    bool CanStandAt(const FVector& Location) const { return CapsuleFitsAt(Location); }
    void EnterRagdoll(const FVector& Momentum);
private:
    friend class FNammaHumanWorldTest;
    friend class FNammaBicycleWorldTest;
    void ToggleRagdoll();
    void Punch();
    void BlockStart();
    void BlockEnd();
    void TickCombat(float DeltaSeconds);
    void Strike();
    bool CanFight() const;
    NammaHuman::FVitals Vitals;
    float AttackTime = 0.f;
    float RecoveryTime = 0.f;
    float DownTime = 0.f;
    float SettledTime = 0.f;
    bool bStrikeSpent = false;
    bool bBlocking = false;
    bool bLeftPunch = false;
    bool bSparringPartner = false;
    TWeakObjectPtr<ANammaPlayerCharacter> CombatTarget;
    FVector CombatHome = FVector::ZeroVector;
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
    bool bFirstPerson = false;
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
