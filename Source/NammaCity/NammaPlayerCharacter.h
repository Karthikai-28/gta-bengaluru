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
    void Interact();
    void TogglePause();
    void Restart();
    void Quit();
    void UpdateFocus();
    bool IsPaused() const;
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
    TWeakObjectPtr<AActor> FocusedActor;
    FTransform StartTransform;
};
