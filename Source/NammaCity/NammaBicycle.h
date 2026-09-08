#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NammaInteractable.h"
#include "NammaBicyclePhysics.h"
#include "NammaRidePose.h"
#include "NammaBicycle.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNammaVehicle, Log, All);

class ANammaPlayerCharacter;
class UBoxComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UInstancedStaticMeshComponent;
class UNammaBicycleMovementComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

// A rideable pedal cycle built to VEH-01-002. The dynamics live in
// NammaBicyclePhysics.h and are driven by UNammaBicycleMovementComponent; this actor
// owns the articulated parts, the rider handover and the controls. Every moving part
// is driven from simulation state, not from a canned animation: the wheels turn at
// their own angular rate, the cranks are geared to the rear wheel through the chain,
// and the chain links travel at the chain speed the drivetrain computes.
UCLASS()
class NAMMACITY_API ANammaBicycle : public APawn, public INammaInteractable
{
    GENERATED_BODY()
public:
    ANammaBicycle();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void UnPossessed() override;

    virtual FText GetInteractionPrompt() const override;
    virtual bool CanInteract(const ANammaPlayerCharacter* Player) const override;
    virtual void Interact(ANammaPlayerCharacter* Player) override;

    bool Mount(ANammaPlayerCharacter* Player);
    // bThrown separates a deliberate dismount from a crash, which ragdolls the rider.
    void Dismount(bool bThrown);
    bool HasRider() const { return Rider.IsValid(); }
    bool GetRidePose(FNammaRidePose& Out) const;
    UNammaBicycleMovementComponent* GetBicycleMovement() const { return Movement; }
    FText GetTelemetryLine() const;
    FText GetGearLine() const;

private:
    friend class FNammaBicycleWorldTest;
    void Pedal(const FInputActionValue& Value);
    void Steer(const FInputActionValue& Value);
    void LookYaw(const FInputActionValue& Value);
    void LookPitch(const FInputActionValue& Value);
    void RearBrake(const FInputActionValue& Value);
    // Axis keys stop firing on release, so each one explicitly zeroes its control.
    void ReleaseAxis(int32 Which);
    void FrontBrake(const FInputActionValue& Value);
    void SprintStart();
    void SprintEnd();
    void ShiftUp();
    void ShiftDown();
    void SelectGear(int32 Gear);
    void RequestDismount();
    void Bail();
    void TogglePause();
    void RestartDelivery();
    void Quit();
    bool IsPaused() const;

    void BuildBicycle();
    void UpdateArticulation();
    void UpdateChain();
    // Finds clear ground beside the bike for the rider to step onto.
    bool FindDismountSpot(const ANammaPlayerCharacter* Player, FVector& Out) const;

    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Hull;
    // Lean and pitch live on the visual root so the swept collision hull stays upright
    // and cannot catch a corner on the road while the bike is leaned over.
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SteerPivot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SteerYaw;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FrontWheel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> RearWheel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Crank;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PedalLeft;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PedalRight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Cassette;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> RiderMount;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> GripLeft;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> GripRight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Chain;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UNammaBicycleMovementComponent> Movement;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;

    NammaBicycle::FRiderInput Controls;
    TWeakObjectPtr<ANammaPlayerCharacter> Rider;
    TWeakObjectPtr<APlayerController> RiderController;
    FTransform StartTransform;
    int32 Gear = 2;
    bool bSprint = false;
    // Chain link instances are laid out once; only their transforms change per frame.
    int32 ChainLinks = 0;
    float BottomBracketX = 0.f;
    float BottomBracketZ = 0.f;
    float ChainLineY = 0.f;
    // Rotation from the chain's own plane into the bike, since the rear axle sits
    // above the bottom bracket while the chain model treats them as level.
    float ChainstayCos = 1.f;
    float ChainstaySin = 0.f;
};
