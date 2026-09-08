#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NammaPlayerCharacter.generated.h"

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
private:
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
    void Restart();
    void Quit();
    void UpdateFocus();
    bool IsPaused() const;
    virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
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
    FTransform StartTransform;
};
