#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "NammaBicyclePhysics.h"
#include "NammaBicycleMovement.generated.h"

// Road surfaces the sandbox can present. Each supplies peak grip and rolling
// resistance for VEH-01-002 section 12; the wheel trace picks one per wheel.
UENUM()
enum class ENammaSurface : uint8
{
    Asphalt,
    Paving,
    Dirt,
    Grass,
    Gravel,
    Wet
};

struct FNammaWheelGround
{
    bool bContact = false;
    double GroundZ = 0.0;          // cm
    double Compression = 0.0;      // m of suspension travel used
    FVector Normal = FVector::UpVector;
    ENammaSurface Surface = ENammaSurface::Asphalt;
};

// Drives the pawn from NammaBicyclePhysics. The core solves the single-track
// dynamics in SI units; this component supplies the world it runs against - per
// wheel ground traces, surface grip, road gradient and swept collision - and writes
// the result back onto the actor. Unreal units are centimetres, so every quantity
// crossing this boundary is converted exactly once, here.
UCLASS()
class NAMMACITY_API UNammaBicycleMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()
public:
    UNammaBicycleMovementComponent();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;
    virtual float GetMaxSpeed() const override;

    // Rider controls, written by the pawn each frame.
    void SetRiderInput(const NammaBicycle::FRiderInput& NewInput) { Input = NewInput; }
    void SetRiderAboard(bool bAboard);
    void ResetTo(const FTransform& Transform);

    const NammaBicycle::FSetup& GetSetup() const { return Setup; }
    const NammaBicycle::FState& GetState() const { return State; }
    const NammaBicycle::FTelemetry& GetTelemetry() const { return Telemetry; }
    const FNammaWheelGround& GetFrontGround() const { return FrontGround; }
    const FNammaWheelGround& GetRearGround() const { return RearGround; }
    // Peak grip the two wheels are averaging, for the HUD and for grip assertions.
    double GetSurfaceFriction() const { return SurfaceFriction; }

    float GetSpeedKph() const { return float(State.Speed * 3.6); }
    // Terrain pitch is geometric, from the two contact heights. The core's own pitch
    // is the dynamic stoppie/wheelie rotation; the drawn bike carries their sum.
    float GetTerrainPitchDegrees() const { return float(FMath::RadiansToDegrees(TerrainPitch)); }
    float GetLeanDegrees() const { return float(FMath::RadiansToDegrees(State.Lean)); }
    float GetPitchDegrees() const
    {
        return float(FMath::RadiansToDegrees(TerrainPitch + State.Pitch));
    }
    bool IsCrashed() const { return State.bCrashed; }
    static FText DescribeSurface(ENammaSurface Surface);

    // Axle heights and offsets the pawn also needs when it builds the visual bike.
    float WheelRadiusCm() const { return float(Setup.WheelRadius * 100.0); }
    float HalfWheelbaseCm() const { return float(Setup.Wheelbase * 50.0); }
    float RestHeightCm() const { return RootRestHeight; }

    UPROPERTY(EditAnywhere, Category = "Bicycle") float SuspensionTravelCm = 6.f;
    UPROPERTY(EditAnywhere, Category = "Bicycle") float StaticSagCm = 2.5f;
    UPROPERTY(EditAnywhere, Category = "Bicycle") float SuspensionDampingRatio = 0.35f;
    // The collision hull clears the road, so kerbs and speed breakers are resolved by
    // the wheel traces rather than by the hull catching on them.
    UPROPERTY(EditAnywhere, Category = "Bicycle") float RootRestHeight = 65.f;
    UPROPERTY(EditAnywhere, Category = "Bicycle") float ParkedLeanDegrees = 11.f;
    UPROPERTY(EditAnywhere, Category = "Bicycle") float CrashedLeanDegrees = 74.f;

private:
    void Probe(FNammaWheelGround& Wheel, const FVector& AxleWorld) const;
    void ApplyGroundToSurface(NammaBicycle::FSurface& Surface) const;
    static ENammaSurface ClassifyHit(const FHitResult& Hit);

    NammaBicycle::FSetup Setup;
    NammaBicycle::FState State;
    NammaBicycle::FTelemetry Telemetry;
    NammaBicycle::FRiderInput Input;
    FNammaWheelGround FrontGround;
    FNammaWheelGround RearGround;
    double SurfaceFriction = 0.85;
    double TerrainPitch = 0.0;      // rad
    double VerticalVelocity = 0.0;  // m/s
    double SubstepCarry = 0.0;
    bool bRiderAboard = false;
};
