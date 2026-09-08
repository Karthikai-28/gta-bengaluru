#include "NammaBicycleMovement.h"
#include "NammaCity.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"

namespace NB = NammaBicycle;

namespace
{
struct FSurfaceProfile
{
    double Friction;
    double RollingResistance;
};

// VEH-01-002 section 12. Grip and rolling resistance per surface; wet is the painted
// road and metal cover case the specification calls out as dangerous for two-wheelers.
FSurfaceProfile ProfileFor(ENammaSurface Surface)
{
    switch (Surface)
    {
    case ENammaSurface::Paving: return {0.80, 0.011};
    case ENammaSurface::Dirt:   return {0.55, 0.025};
    case ENammaSurface::Grass:  return {0.45, 0.033};
    case ENammaSurface::Gravel: return {0.40, 0.038};
    case ENammaSurface::Wet:    return {0.35, 0.010};
    default:                    return {0.85, 0.008};
    }
}

// The sandbox is built from named materials and tagged actors, so both are read.
// An explicit tag wins, because the test strip reuses the road material.
bool SurfaceFromName(const FString& Name, ENammaSurface& Out)
{
    static const TPair<const TCHAR*, ENammaSurface> Table[] = {
        {TEXT("gravel"), ENammaSurface::Gravel},
        {TEXT("wet"), ENammaSurface::Wet},
        {TEXT("dirt"), ENammaSurface::Dirt},
        {TEXT("grass"), ENammaSurface::Grass},
        {TEXT("ground"), ENammaSurface::Grass},
        {TEXT("paving"), ENammaSurface::Paving},
        {TEXT("road"), ENammaSurface::Asphalt},
    };
    for (const auto& Entry : Table)
        if (Name.Contains(Entry.Key, ESearchCase::IgnoreCase))
        {
            Out = Entry.Value;
            return true;
        }
    return false;
}
}  // namespace

UNammaBicycleMovementComponent::UNammaBicycleMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Tick after input so the rider's controls reach the same frame they were pressed.
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

float UNammaBicycleMovementComponent::GetMaxSpeed() const
{
    // Sprinting downhill is faster than the flat top speed, so report the envelope.
    return float(Setup.SprintPower > 0.0 ? 1600.0 : 1000.0);
}

void UNammaBicycleMovementComponent::SetRiderAboard(bool bAboard)
{
    if (bRiderAboard == bAboard) return;
    bRiderAboard = bAboard;
    if (bAboard)
    {
        // Mounting a bike that went down is picking it up and getting back on.
        State.bCrashed = false;
        State.Pitch = State.PitchRate = 0.0;
        State.Lean = State.LeanRate = 0.0;
        State.SlideTime = 0.0;
    }
    // The rider is most of the mass and all of the centre of gravity height. A parked
    // bike is 13 kg sitting low; a ridden one is 88 kg sitting at 1.04 m.
    Setup.RiderMass = bAboard ? 75.0 : 0.0;
    Setup.CentreOfMassHeight = bAboard ? 1.037 : 0.560;
    Setup.CentreOfMassToRearAxle = bAboard ? 0.470 : 0.524;
}

void UNammaBicycleMovementComponent::ResetTo(const FTransform& Transform)
{
    State = NB::FState();
    Telemetry = NB::FTelemetry();
    Input = NB::FRiderInput();
    State.Yaw = FMath::DegreesToRadians(Transform.Rotator().Yaw);
    State.Gear = 2;
    State.ChainLateral = NB::SprocketOffset(Setup, 2);
    VerticalVelocity = 0.0;
    TerrainPitch = 0.0;
    SubstepCarry = 0.0;
    if (UpdatedComponent) UpdatedComponent->SetWorldTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
}

ENammaSurface UNammaBicycleMovementComponent::ClassifyHit(const FHitResult& Hit)
{
    ENammaSurface Surface = ENammaSurface::Asphalt;
    if (const AActor* Actor = Hit.GetActor())
        for (const FName& Tag : Actor->Tags)
        {
            FString Name = Tag.ToString();
            if (Name.RemoveFromStart(TEXT("NammaSurface.")) && SurfaceFromName(Name, Surface)) return Surface;
        }
    if (const UPrimitiveComponent* Component = Hit.GetComponent())
        if (const UMaterialInterface* Material = Component->GetMaterial(0))
            if (SurfaceFromName(Material->GetName(), Surface)) return Surface;
    return Surface;
}

void UNammaBicycleMovementComponent::Probe(FNammaWheelGround& Wheel, const FVector& AxleWorld) const
{
    Wheel = FNammaWheelGround();
    const float Radius = WheelRadiusCm();
    const float Reach = Radius + SuspensionTravelCm + 60.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaBicycleWheel), false, PawnOwner);
    if (PawnOwner)
        for (AActor* Attached : PawnOwner->Children) Params.AddIgnoredActor(Attached);
    FHitResult Hit;
    // A sphere the size of the wheel, not a line: a bicycle tyre bridges a seam or a
    // narrow pothole edge instead of dropping into it like an infinitely thin probe.
    if (!GetWorld()->SweepSingleByChannel(Hit, AxleWorld + FVector(0, 0, Radius),
                                          AxleWorld - FVector(0, 0, Reach), FQuat::Identity,
                                          ECC_Visibility, FCollisionShape::MakeSphere(Radius * 0.5f), Params))
        return;
    Wheel.bContact = true;
    Wheel.GroundZ = Hit.ImpactPoint.Z;
    Wheel.Normal = Hit.ImpactNormal;
    Wheel.Surface = ClassifyHit(Hit);
}

void UNammaBicycleMovementComponent::ApplyGroundToSurface(NB::FSurface& Surface) const
{
    // Each wheel sees its own road, so a bike straddling gravel and asphalt gets the
    // average grip weighted by how much load each wheel is carrying.
    const FSurfaceProfile Front = ProfileFor(FrontGround.Surface);
    const FSurfaceProfile Rear = ProfileFor(RearGround.Surface);
    const double FrontShare = Setup.CentreOfMassToRearAxle / Setup.Wheelbase;
    Surface.Friction = Front.Friction * FrontShare + Rear.Friction * (1.0 - FrontShare);
    Surface.RollingResistance = Front.RollingResistance * FrontShare
                              + Rear.RollingResistance * (1.0 - FrontShare);
    Surface.Grade = TerrainPitch;
}

void UNammaBicycleMovementComponent::TickComponent(float DeltaSeconds, ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
    if (ShouldSkipUpdate(DeltaSeconds) || !UpdatedComponent || !PawnOwner) return;

    const double Mass = NB::TotalMass(Setup);
    const float Radius = WheelRadiusCm();
    const float HalfBase = HalfWheelbaseCm();
    const FRotator Heading(0.f, float(FMath::RadiansToDegrees(State.Yaw)), 0.f);
    const FVector Forward = Heading.Vector();
    const FVector Origin = UpdatedComponent->GetComponentLocation();
    // The axle line sits RootRestHeight - WheelRadius below the collision hull's centre.
    const FVector AxleLine = Origin - FVector(0, 0, RootRestHeight - Radius);
    Probe(FrontGround, AxleLine + Forward * HalfBase);
    Probe(RearGround, AxleLine - Forward * HalfBase);

    if (FrontGround.bContact && RearGround.bContact)
        TerrainPitch = FMath::Atan2(FrontGround.GroundZ - RearGround.GroundZ, 2.0 * HalfBase);
    else
        TerrainPitch = FMath::FInterpTo(float(TerrainPitch), 0.f, DeltaSeconds, 6.f);

    // Suspension. Sag is chosen so both wheels read a load scale of exactly one when
    // the bike is standing still, which is what makes the tyre model's normal loads
    // agree with the static split the specification sheet gives.
    const double Sag = FMath::Max(0.005f, StaticSagCm) / 100.0;
    const double FrontStatic = Mass * NB::Gravity * Setup.CentreOfMassToRearAxle / Setup.Wheelbase;
    const double RearStatic = Mass * NB::Gravity - FrontStatic;
    const double FrontRate = FrontStatic / Sag;
    const double RearRate = RearStatic / Sag;
    const double Travel = SuspensionTravelCm / 100.0;

    auto Compression = [&](const FNammaWheelGround& Wheel, double Lift)
    {
        if (!Wheel.bContact) return 0.0;
        const double AxleZ = (Origin.Z - RootRestHeight + Radius + Lift) / 100.0;
        return FMath::Max(0.0, Setup.WheelRadius - (AxleZ - Wheel.GroundZ / 100.0));
    };
    const double Lift = HalfBase * FMath::Sin(TerrainPitch);
    FrontGround.Compression = Compression(FrontGround, Lift);
    RearGround.Compression = Compression(RearGround, -Lift);

    auto SpringForce = [&](double Deflection, double Rate)
    {
        // Linear over the travel, then a progressive bump stop rather than a wall.
        const double Over = FMath::Max(0.0, Deflection - Travel);
        return Rate * FMath::Min(Deflection, Travel) + Rate * 12.0 * Over * Over / FMath::Max(1e-4, Travel);
    };
    const double FrontSpring = SpringForce(FrontGround.Compression, FrontRate);
    const double RearSpring = SpringForce(RearGround.Compression, RearRate);
    const bool bGrounded = FrontGround.Compression > 0.0 || RearGround.Compression > 0.0;
    const double Damping = 2.0 * SuspensionDampingRatio * FMath::Sqrt((FrontRate + RearRate) * Mass);
    const double VerticalAccel = (FrontSpring + RearSpring - (bGrounded ? Damping * VerticalVelocity : 0.0))
                                 / Mass - NB::Gravity;
    VerticalVelocity += VerticalAccel * DeltaSeconds;

    NB::FSurface Surface;
    ApplyGroundToSurface(Surface);
    SurfaceFriction = Surface.Friction;
    Surface.FrontLoadScale = FMath::Clamp(FrontSpring / FMath::Max(1.0, FrontStatic), 0.0, 2.0);
    Surface.RearLoadScale = FMath::Clamp(RearSpring / FMath::Max(1.0, RearStatic), 0.0, 2.0);

    NB::FRiderInput Applied = Input;
    if (!bRiderAboard || State.bCrashed)
    {
        // A bike nobody is on is parked, not free-rolling downhill.
        Applied = NB::FRiderInput();
        Applied.FrontBrake = Applied.RearBrake = 1.0;
        Applied.Gear = State.Gear;
    }

    // Fixed 240 Hz substeps with a bounded catch-up budget, matching how the character
    // caps its own substepping. A long stall drops simulation time rather than
    // integrating one enormous step.
    constexpr double Substep = 1.0 / 240.0;
    SubstepCarry = FMath::Min(SubstepCarry + DeltaSeconds, Substep * 24.0);
    while (SubstepCarry >= Substep)
    {
        Telemetry = NB::Step(Setup, State, Applied, Surface, Substep);
        SubstepCarry -= Substep;
    }
    // Parked it rests on its stand; crashed it lies on its side until someone picks it up.
    if (!bRiderAboard)
        State.Lean = FMath::FInterpTo(float(State.Lean),
            FMath::DegreesToRadians(State.bCrashed ? -CrashedLeanDegrees : -ParkedLeanDegrees),
            DeltaSeconds, 5.f);

    const FVector Delta = Forward * float(State.Speed * 100.0 * DeltaSeconds)
                        + FVector(0, 0, float(VerticalVelocity * 100.0 * DeltaSeconds));
    const FRotator NewHeading(0.f, float(FMath::RadiansToDegrees(State.Yaw)), 0.f);
    FHitResult Hit;
    SafeMoveUpdatedComponent(Delta, NewHeading, true, Hit);
    if (Hit.IsValidBlockingHit())
    {
        SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
        // Only the component of travel into the obstacle is lost; a graze keeps its speed.
        const double Into = -FVector::DotProduct(Forward, Hit.Normal);
        if (Into > 0.0)
        {
            State.Speed *= FMath::Max(0.0, 1.0 - Into);
            if (Into > 0.7 && FMath::Abs(State.Speed) > 2.0) State.bCrashed = true;
        }
        if (Hit.Normal.Z > 0.7 && VerticalVelocity < 0.0) VerticalVelocity = 0.0;
    }
    // Publish the pawn velocity: GetVelocity() feeds the camera, the animation graph
    // and the momentum a crash hands to the rider's ragdoll.
    Velocity = Forward * float(State.Speed * 100.0) + FVector(0, 0, float(VerticalVelocity * 100.0));
    UpdateComponentVelocity();
}

FText UNammaBicycleMovementComponent::DescribeSurface(ENammaSurface Surface)
{
    switch (Surface)
    {
    case ENammaSurface::Paving: return NSLOCTEXT("Namma", "SurfacePaving", "paving");
    case ENammaSurface::Dirt:   return NSLOCTEXT("Namma", "SurfaceDirt", "dirt");
    case ENammaSurface::Grass:  return NSLOCTEXT("Namma", "SurfaceGrass", "grass");
    case ENammaSurface::Gravel: return NSLOCTEXT("Namma", "SurfaceGravel", "gravel");
    case ENammaSurface::Wet:    return NSLOCTEXT("Namma", "SurfaceWet", "wet");
    default:                    return NSLOCTEXT("Namma", "SurfaceAsphalt", "asphalt");
    }
}
