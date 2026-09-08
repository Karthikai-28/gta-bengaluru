#include "NammaBicycle.h"
#include "NammaBicycleMovement.h"
#include "NammaCityGameModeBase.h"
#include "NammaPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogNammaVehicle);

namespace NB = NammaBicycle;

namespace
{
// Bicycle layout in centimetres, measured from the ground at the midpoint of the
// wheelbase. VEH-01-002 section 4 fixes the wheelbase, wheel size and overall height;
// the rest is ordinary roadster proportion.
constexpr float RootHeight = 65.f;
constexpr float WheelRadius = 35.f;
constexpr float HalfWheelbase = 52.4f;
constexpr float BottomBracketHeight = 28.f;
constexpr float ChainLine = 4.5f;      // drivetrain sits right of the frame centreline
constexpr float GripHalfWidth = 26.f;

const FVector RearAxle(-HalfWheelbase, 0.f, WheelRadius);
const FVector FrontAxle(HalfWheelbase, 0.f, WheelRadius);
const FVector SeatCluster(-22.f, 0.f, 88.f);
const FVector SaddleTop(-26.f, 0.f, 96.f);
const FVector HeadTop(38.f, 0.f, 78.f);
const FVector HeadBottom(46.f, 0.f, 62.f);
const FVector BarCentre(33.f, 0.f, 100.f);

// The bottom bracket is placed so the chainstay is exactly the length the drivetrain
// model uses, which is what lets the drawn chain land on the real rear sprocket.
float BottomBracketOffsetX()
{
    const float Chainstay = 46.5f;
    const float Rise = WheelRadius - BottomBracketHeight;
    return RearAxle.X + FMath::Sqrt(FMath::Max(1.f, Chainstay * Chainstay - Rise * Rise));
}

FVector Ground(const FVector& Point) { return Point - FVector(0.f, 0.f, RootHeight); }
}  // namespace

ANammaBicycle::ANammaBicycle()
{
    PrimaryActorTick.bCanEverTick = true;
    Hull = CreateDefaultSubobject<UBoxComponent>(TEXT("Hull"));
    // The hull clears the road by 22 cm so kerbs, seams and speed breakers are
    // resolved by the wheel traces instead of snagging a corner of the collision box.
    Hull->InitBoxExtent(FVector(55.f, 22.f, 43.f));
    Hull->SetCollisionProfileName(TEXT("Pawn"));
    Hull->SetCanEverAffectNavigation(false);
    RootComponent = Hull;

    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(Hull);
    Movement = CreateDefaultSubobject<UNammaBicycleMovementComponent>(TEXT("BicycleMovement"));
    Movement->UpdatedComponent = Hull;

    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    // The boom hangs off the upright hull, not the visual root, so the horizon does
    // not roll with the bike.
    Boom->SetupAttachment(Hull);
    Boom->TargetArmLength = 460.f;
    Boom->SocketOffset = FVector(0.f, 0.f, 70.f);
    Boom->bUsePawnControlRotation = true;
    Boom->bDoCollisionTest = true;
    Boom->bEnableCameraLag = true;
    Boom->CameraLagSpeed = 10.f;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 85.f;

    BuildBicycle();
}

void ANammaBicycle::BuildBicycle()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Cube = CubeMesh.Object;
    UStaticMesh* Cylinder = CylinderMesh.Object;
    int32 Serial = 0;

    auto Piece = [&](USceneComponent* Parent, UStaticMesh* Mesh, const TCHAR* Palette,
                     const FVector& Location, const FRotator& Rotation, const FVector& Scale)
    {
        auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(
            FName(*FString::Printf(TEXT("Part%d"), Serial++)));
        Part->SetupAttachment(Parent);
        Part->SetStaticMesh(Mesh);
        Part->SetRelativeLocationAndRotation(Location, Rotation);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCanEverAffectNavigation(false);
        Part->ComponentTags.Add(FName(Palette));
        Parts.Add(Part);
        return Part;
    };
    // Frame tubes are boxes stretched between two points, so the frame is described by
    // its joints rather than by hand-placed transforms.
    auto Tube = [&](USceneComponent* Parent, const TCHAR* Palette, const FVector& From,
                    const FVector& To, float Thickness)
    {
        const FVector Along = To - From;
        return Piece(Parent, Cube, Palette, (From + To) * 0.5f,
                     FRotationMatrix::MakeFromX(Along).Rotator(),
                     FVector(Along.Size() / 100.f, Thickness / 100.f, Thickness / 100.f));
    };
    auto Disc = [&](USceneComponent* Parent, const TCHAR* Palette, const FVector& Location,
                    float Radius, float Width)
    {
        // The basic cylinder is 100 tall and 100 across; rolling it 90 degrees puts its
        // axis along Y, which is the axle direction.
        return Piece(Parent, Cylinder, Palette, Location, FRotator(0.f, 0.f, 90.f),
                     FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, Width / 100.f));
    };

    const FVector BottomBracket(BottomBracketOffsetX(), 0.f, BottomBracketHeight);
    BottomBracketX = BottomBracket.X;
    BottomBracketZ = BottomBracket.Z;
    ChainLineY = ChainLine;
    const FVector Chainstay = RearAxle - BottomBracket;
    ChainstayCos = float(-Chainstay.X / Chainstay.Size2D());
    ChainstaySin = float(-Chainstay.Z / Chainstay.Size2D());

    // ---- frame ----
    for (float Side : {-5.f, 5.f})
    {
        Tube(VisualRoot, TEXT("teal"), Ground(BottomBracket) + FVector(0, Side, 0),
             Ground(RearAxle) + FVector(0, Side, 0), 2.2f);
        Tube(VisualRoot, TEXT("teal"), Ground(SeatCluster) + FVector(0, Side * 0.8f, 0),
             Ground(RearAxle) + FVector(0, Side, 0), 1.8f);
    }
    Tube(VisualRoot, TEXT("teal"), Ground(BottomBracket), Ground(SeatCluster), 3.2f);
    Tube(VisualRoot, TEXT("teal"), Ground(BottomBracket), Ground(HeadBottom), 3.6f);
    Tube(VisualRoot, TEXT("teal"), Ground(SeatCluster), Ground(HeadTop), 3.0f);
    Tube(VisualRoot, TEXT("teal"), Ground(HeadTop), Ground(HeadBottom), 4.0f);
    Tube(VisualRoot, TEXT("dark"), Ground(SeatCluster), Ground(SaddleTop), 2.4f);
    Piece(VisualRoot, Cube, TEXT("trunk"), Ground(SaddleTop) + FVector(0, 0, 2.f),
          FRotator::ZeroRotator, FVector(0.24f, 0.13f, 0.035f));
    // Rear carrier: the detail that makes an Indian roadster read as one.
    Piece(VisualRoot, Cube, TEXT("dark"), Ground(FVector(-44.f, 0.f, 74.f)),
          FRotator::ZeroRotator, FVector(0.28f, 0.17f, 0.02f));
    for (float Side : {-7.f, 7.f})
        Tube(VisualRoot, TEXT("dark"), Ground(FVector(-52.f, Side, 73.f)),
             Ground(RearAxle) + FVector(0, Side, 0), 1.2f);

    // ---- steering assembly ----
    SteerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SteerPivot"));
    SteerPivot->SetupAttachment(VisualRoot);
    const FVector SteerAxis = HeadTop - HeadBottom;
    SteerPivot->SetRelativeLocationAndRotation(
        Ground(HeadTop), FRotator(90.f - FMath::RadiansToDegrees(FMath::Atan2(SteerAxis.Z, -SteerAxis.X)), 0.f, 0.f));
    SteerYaw = CreateDefaultSubobject<USceneComponent>(TEXT("SteerYaw"));
    SteerYaw->SetupAttachment(SteerPivot);

    const FTransform ForkFrame = SteerPivot->GetRelativeTransform();
    auto ToFork = [&ForkFrame](const FVector& BikeLocal) { return ForkFrame.InverseTransformPosition(BikeLocal); };
    for (float Side : {-8.f, 8.f})
        Tube(SteerYaw, TEXT("teal"), ToFork(Ground(HeadBottom) + FVector(0, Side * 0.4f, 0)),
             ToFork(Ground(FrontAxle) + FVector(0, Side, 0)), 2.4f);
    Tube(SteerYaw, TEXT("dark"), ToFork(Ground(HeadTop)), ToFork(Ground(BarCentre)), 2.6f);
    Piece(SteerYaw, Cube, TEXT("dark"), ToFork(Ground(BarCentre)),
          ForkFrame.InverseTransformRotation(FQuat::Identity).Rotator(),
          FVector(0.03f, 0.52f, 0.03f));
    for (float Side : {-GripHalfWidth, GripHalfWidth})
        Piece(SteerYaw, Cube, TEXT("trunk"), ToFork(Ground(BarCentre) + FVector(0, Side, 0)),
              ForkFrame.InverseTransformRotation(FQuat::Identity).Rotator(),
              FVector(0.035f, 0.11f, 0.035f));
    GripLeft = CreateDefaultSubobject<USceneComponent>(TEXT("GripLeft"));
    GripLeft->SetupAttachment(SteerYaw);
    GripLeft->SetRelativeLocation(ToFork(Ground(BarCentre) + FVector(0, -GripHalfWidth, 0)));
    GripRight = CreateDefaultSubobject<USceneComponent>(TEXT("GripRight"));
    GripRight->SetupAttachment(SteerYaw);
    GripRight->SetRelativeLocation(ToFork(Ground(BarCentre) + FVector(0, GripHalfWidth, 0)));

    // ---- wheels: rim, tyre and spokes, so rotation is visible rather than implied ----
    auto BuildWheel = [&](USceneComponent* Pivot)
    {
        Disc(Pivot, TEXT("dark"), FVector::ZeroVector, WheelRadius, 4.0f);
        Disc(Pivot, TEXT("cream"), FVector::ZeroVector, WheelRadius - 3.5f, 4.4f);
        Disc(Pivot, TEXT("dark"), FVector::ZeroVector, 4.f, 6.f);
        for (int32 Spoke = 0; Spoke < 4; ++Spoke)
        {
            const float Angle = Spoke * 45.f;
            Piece(Pivot, Cube, TEXT("cream"), FVector::ZeroVector, FRotator(Angle, 0.f, 0.f),
                  FVector((WheelRadius - 4.f) * 2.f / 100.f, 0.008f, 0.008f));
        }
    };
    FrontWheel = CreateDefaultSubobject<USceneComponent>(TEXT("FrontWheel"));
    FrontWheel->SetupAttachment(SteerYaw);
    FrontWheel->SetRelativeLocation(ToFork(Ground(FrontAxle)));
    BuildWheel(FrontWheel);
    RearWheel = CreateDefaultSubobject<USceneComponent>(TEXT("RearWheel"));
    RearWheel->SetupAttachment(VisualRoot);
    RearWheel->SetRelativeLocation(Ground(RearAxle));
    BuildWheel(RearWheel);

    // ---- drivetrain ----
    Crank = CreateDefaultSubobject<USceneComponent>(TEXT("Crank"));
    Crank->SetupAttachment(VisualRoot);
    Crank->SetRelativeLocation(Ground(BottomBracket));
    const float CrankArm = float(Movement->GetSetup().CrankLength * 100.0);
    const float RingRadius = float(NB::SprocketRadius(Movement->GetSetup().ChainringTeeth) * 100.0);
    Disc(Crank, TEXT("gold"), FVector(0.f, ChainLine, 0.f), RingRadius, 0.6f);
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const float Sign = Side == 0 ? 1.f : -1.f;
        const float Y = Side == 0 ? 7.f : -7.f;
        Piece(Crank, Cube, TEXT("dark"), FVector(Sign * CrankArm * 0.5f, Y, 0.f), FRotator::ZeroRotator,
              FVector(CrankArm / 100.f, 0.028f, 0.022f));
    }
    // Pedals hang from the crank ends and are levelled every frame, as real pedals are.
    PedalRight = CreateDefaultSubobject<USceneComponent>(TEXT("PedalRight"));
    PedalRight->SetupAttachment(Crank);
    PedalRight->SetRelativeLocation(FVector(CrankArm, 16.f, 0.f));
    Piece(PedalRight, Cube, TEXT("dark"), FVector::ZeroVector, FRotator::ZeroRotator,
          FVector(0.09f, 0.07f, 0.018f));
    PedalLeft = CreateDefaultSubobject<USceneComponent>(TEXT("PedalLeft"));
    PedalLeft->SetupAttachment(Crank);
    PedalLeft->SetRelativeLocation(FVector(-CrankArm, -16.f, 0.f));
    Piece(PedalLeft, Cube, TEXT("dark"), FVector::ZeroVector, FRotator::ZeroRotator,
          FVector(0.09f, 0.07f, 0.018f));

    Cassette = CreateDefaultSubobject<USceneComponent>(TEXT("Cassette"));
    Cassette->SetupAttachment(VisualRoot);
    Cassette->SetRelativeLocation(Ground(RearAxle));
    const NB::FSetup& Setup = Movement->GetSetup();
    for (int32 Sprocket = 0; Sprocket < Setup.GearCount; ++Sprocket)
        Disc(Cassette, TEXT("gold"),
             FVector(0.f, ChainLine + float(NB::SprocketOffset(Setup, Sprocket) * 100.0), 0.f),
             float(NB::SprocketRadius(Setup.SprocketTeeth[Sprocket]) * 100.0), 0.35f);

    Chain = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Chain"));
    Chain->SetupAttachment(VisualRoot);
    Chain->SetStaticMesh(Cube);
    Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Chain->SetCanEverAffectNavigation(false);
    Chain->ComponentTags.Add(FName("gold"));

    RiderMount = CreateDefaultSubobject<USceneComponent>(TEXT("RiderMount"));
    RiderMount->SetupAttachment(VisualRoot);
    RiderMount->SetRelativeLocation(Ground(SaddleTop) + FVector(0.f, 0.f, 4.f));
}

void ANammaBicycle::BeginPlay()
{
    Super::BeginPlay();
    StartTransform = GetActorTransform();
    Movement->ResetTo(StartTransform);
    auto Paint = [](UPrimitiveComponent* Part)
    {
        if (Part->ComponentTags.Num() == 0) return;
        const FString Path = FString::Printf(
            TEXT("/Game/NammaCity/Materials/Sandbox/M_Sandbox_%s.M_Sandbox_%s"),
            *Part->ComponentTags[0].ToString(), *Part->ComponentTags[0].ToString());
        if (auto* Material = LoadObject<UMaterialInterface>(nullptr, *Path)) Part->SetMaterial(0, Material);
    };
    for (UStaticMeshComponent* Part : Parts) Paint(Part);
    Paint(Chain);

    // One instance per real chain link, laid out once and moved every frame.
    const NB::FChain Loop = NB::ChainGeometry(Movement->GetSetup(),
        NB::SprocketRadius(Movement->GetSetup().SprocketTeeth[Gear]),
        NB::SprocketOffset(Movement->GetSetup(), Gear));
    ChainLinks = FMath::RoundToInt(Loop.LinkCount);
    Chain->ClearInstances();
    for (int32 Link = 0; Link < ChainLinks; ++Link) Chain->AddInstance(FTransform::Identity);
    UpdateArticulation();
    UE_LOG(LogNammaVehicle, Log, TEXT("Cycle ready at %s: %d chain links, %d gears, %.2f m wheelbase"),
        *GetActorLocation().ToCompactString(), ChainLinks, Movement->GetSetup().GearCount,
        Movement->GetSetup().Wheelbase);
}

void ANammaBicycle::UnPossessed()
{
    if (auto* PC = Cast<APlayerController>(Controller))
        if (auto* LocalPlayer = PC->GetLocalPlayer())
            if (auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (Mapping) Subsystem->RemoveMappingContext(Mapping);
    Super::UnPossessed();
}

bool ANammaBicycle::IsPaused() const { return UGameplayStatics::IsGamePaused(this); }

void ANammaBicycle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Controls.Gear = Gear;
    Controls.bSprint = bSprint;
    if (IsPaused())
    {
        Controls.Pedal = 0.0;
        Controls.Steer = 0.0;
    }
    Movement->SetRiderInput(Controls);
    UpdateArticulation();
    if (Movement->IsCrashed() && Rider.IsValid()) Dismount(true);
    const FVector Position = GetActorLocation();
    if (Position.Z < -500.f || FMath::Abs(Position.X) > 6200.f || FMath::Abs(Position.Y) > 6200.f)
    {
        if (Rider.IsValid()) Dismount(true);
        Movement->ResetTo(StartTransform);
    }
}

void ANammaBicycle::UpdateArticulation()
{
    const NB::FState& State = Movement->GetState();
    auto Spin = [](double Radians) { return FRotator(float(-FMath::RadiansToDegrees(Radians)), 0.f, 0.f); };
    VisualRoot->SetRelativeRotation(FRotator(Movement->GetPitchDegrees(), 0.f, Movement->GetLeanDegrees()));
    SteerYaw->SetRelativeRotation(FRotator(0.f, float(FMath::RadiansToDegrees(State.SteerAngle)), 0.f));
    FrontWheel->SetRelativeRotation(Spin(State.FrontWheelAngle));
    RearWheel->SetRelativeRotation(Spin(State.RearWheelAngle));
    Cassette->SetRelativeRotation(Spin(State.RearWheelAngle));
    Crank->SetRelativeRotation(Spin(State.CrankAngle));
    // Cancelling the crank rotation keeps both pedal platforms level, which is what the
    // rider's feet stand on.
    const FRotator Level(float(FMath::RadiansToDegrees(State.CrankAngle)), 0.f, 0.f);
    PedalLeft->SetRelativeRotation(Level);
    PedalRight->SetRelativeRotation(Level);
    UpdateChain();
}

void ANammaBicycle::UpdateChain()
{
    if (ChainLinks <= 0) return;
    const NB::FSetup& Setup = Movement->GetSetup();
    const NB::FChain& Loop = Movement->GetTelemetry().Chain;
    if (Loop.TotalLength <= 0.0) return;
    const double Travel = Movement->GetState().ChainTravel;
    // The chain model treats the sprocket as level with the chainring; the real rear
    // axle sits above it, so every link is rotated into the frame by the chainstay angle.
    auto ToFrame = [this](const NB::FChainPoint& Point)
    {
        const float X = float(Point.X * 100.0);
        const float Z = float(Point.Z * 100.0);
        return FVector(BottomBracketX + X * ChainstayCos - Z * ChainstaySin,
                       ChainLineY + float(Point.Y * 100.0),
                       BottomBracketZ + X * ChainstaySin + Z * ChainstayCos) - FVector(0.f, 0.f, RootHeight);
    };
    TArray<FTransform> Transforms;
    Transforms.Reserve(ChainLinks);
    for (int32 Link = 0; Link < ChainLinks; ++Link)
    {
        const double Along = Travel + Link * NB::ChainPitch;
        const FVector Here = ToFrame(NB::ChainPathPoint(Setup, Loop, Along));
        const FVector Next = ToFrame(NB::ChainPathPoint(Setup, Loop, Along + NB::ChainPitch));
        Transforms.Emplace(FRotationMatrix::MakeFromX(Next - Here).Rotator(), Here,
                           FVector(0.011f, 0.006f, 0.009f));
    }
    Chain->BatchUpdateInstancesTransforms(0, Transforms, false, true, false);
}

// ---------------------------------------------------------------------------
FText ANammaBicycle::GetInteractionPrompt() const
{
    return NSLOCTEXT("Namma", "RideCycle", "Press E to ride the cycle");
}

bool ANammaBicycle::CanInteract(const ANammaPlayerCharacter* Player) const
{
    return Player && !Rider.IsValid()
        && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(260.f);
}

void ANammaBicycle::Interact(ANammaPlayerCharacter* Player) { Mount(Player); }

bool ANammaBicycle::Mount(ANammaPlayerCharacter* Player)
{
    if (!Player || Rider.IsValid()) return false;
    auto* PC = Cast<APlayerController>(Player->GetController());
    if (!PC) return false;
    // Measure before attaching: the mannequin's hips have to land on the saddle.
    const float PelvisAboveActor =
        float(Player->GetMesh()->GetSocketLocation(TEXT("pelvis")).Z - Player->GetActorLocation().Z);
    Rider = Player;
    RiderController = PC;
    Player->BeginRiding(this);
    Player->AttachToComponent(RiderMount, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Player->SetActorRelativeLocation(FVector(0.f, 0.f, -PelvisAboveActor));
    Player->SetActorRelativeRotation(FRotator::ZeroRotator);
    Movement->SetRiderAboard(true);
    PC->Possess(this);
    PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
    UE_LOG(LogNammaVehicle, Log, TEXT("Rider mounted the cycle at %s"), *GetActorLocation().ToCompactString());
    return true;
}

bool ANammaBicycle::FindDismountSpot(const ANammaPlayerCharacter* Player, FVector& Out) const
{
    if (!Player) return false;
    const float HalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Right = GetActorRightVector();
    const FVector Forward = GetActorForwardVector();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NammaDismount), false, this);
    Params.AddIgnoredActor(Player);
    // Step off on the kerb side first, then the road side, then behind the bike.
    for (const FVector& Offset : {Right * -95.f, Right * 95.f, Forward * -130.f, Forward * 130.f})
    {
        const FVector Above = GetActorLocation() + Offset + FVector(0.f, 0.f, 120.f);
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, Above, Above - FVector(0.f, 0.f, 400.f),
                                                  ECC_Visibility, Params))
            continue;
        const FVector Candidate = Floor.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 3.f);
        if (Player->CanStandAt(Candidate))
        {
            Out = Candidate;
            return true;
        }
    }
    return false;
}

void ANammaBicycle::Dismount(bool bThrown)
{
    ANammaPlayerCharacter* Player = Rider.Get();
    APlayerController* PC = RiderController.Get();
    Rider.Reset();
    RiderController.Reset();
    Movement->SetRiderAboard(false);
    Controls = NB::FRiderInput();
    bSprint = false;
    if (!Player) return;
    UE_LOG(LogNammaVehicle, Log, TEXT("Rider %s the cycle at %.1f km/h (lean %.0f deg, pitch %.2f rad)"),
        bThrown ? TEXT("was thrown off") : TEXT("got off"), Movement->GetSpeedKph(),
        Movement->GetLeanDegrees(), Movement->GetState().Pitch);
    FVector Spot;
    if (!FindDismountSpot(Player, Spot))
        Spot = GetActorLocation() - GetActorRightVector() * 95.f
             + FVector(0.f, 0.f, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    // A crash always throws the rider, even a slow tip-over: going over the bars at
    // walking pace still puts the rider on the road in front of the bike.
    Player->EndRiding(Spot, bThrown ? GetVelocity() + GetActorForwardVector() * 130.f
                                    : FVector::ZeroVector);
    if (PC)
    {
        PC->Possess(Player);
        PC->SetControlRotation(FRotator(-12.f, GetActorRotation().Yaw, 0.f));
    }
}

bool ANammaBicycle::GetRidePose(FNammaRidePose& Out) const
{
    if (!Rider.IsValid()) return false;
    // The ankle sits above the pedal platform, not on it.
    const FVector Ankle = VisualRoot->GetUpVector() * 11.f;
    Out.PedalLeft = PedalLeft->GetComponentLocation() + Ankle;
    Out.PedalRight = PedalRight->GetComponentLocation() + Ankle;
    Out.GripLeft = GripLeft->GetComponentLocation();
    Out.GripRight = GripRight->GetComponentLocation();
    Out.Saddle = RiderMount->GetComponentLocation();
    // An upright roadster posture that tucks a little as speed rises.
    Out.TorsoPitchDegrees = 18.f + FMath::Clamp(Movement->GetSpeedKph(), 0.f, 35.f) * 0.35f;
    Out.bFootDown = Movement->GetTelemetry().bFootDown;
    return true;
}

FText ANammaBicycle::GetGearLine() const
{
    const NB::FTelemetry& Telemetry = Movement->GetTelemetry();
    return FText::FromString(FString::Printf(
        TEXT("Gear %d/%d  %dT  ratio %.2f  development %.2f m  cadence %.0f rpm%s"),
        Gear + 1, Movement->GetSetup().GearCount, Movement->GetSetup().SprocketTeeth[Gear],
        Telemetry.GearRatio, Telemetry.Development, Telemetry.Cadence,
        Telemetry.bShifting ? TEXT("  SHIFTING") : TEXT("")));
}

FText ANammaBicycle::GetTelemetryLine() const
{
    const NB::FTelemetry& Telemetry = Movement->GetTelemetry();
    FString Flags;
    if (Telemetry.bFreewheeling) Flags += TEXT(" freewheel");
    if (Telemetry.bFrontLocked) Flags += TEXT(" FRONT LOCKED");
    if (Telemetry.bRearLocked) Flags += TEXT(" REAR SKID");
    if (Telemetry.bSliding) Flags += TEXT(" SLIDING");
    if (Telemetry.bRearAirborne) Flags += TEXT(" REAR UP");
    if (Telemetry.bFrontAirborne) Flags += TEXT(" FRONT UP");
    if (Telemetry.bFootDown) Flags += TEXT(" foot down");
    return FText::FromString(FString::Printf(
        TEXT("%.1f km/h  lean %.0f deg  chain %.2f m/s  %.0f W  load %.0f/%.0f N  %s%s"),
        Movement->GetSpeedKph(), Movement->GetLeanDegrees(), Telemetry.Chain.Speed,
        Telemetry.RiderPower, Telemetry.FrontLoad, Telemetry.RearLoad,
        *UNammaBicycleMovementComponent::DescribeSurface(Movement->GetRearGround().Surface).ToString(),
        *Flags));
}

// ---------------------------------------------------------------------------
void ANammaBicycle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    auto* PC = Cast<APlayerController>(Controller);
    auto* Subsystem = PC && PC->GetLocalPlayer()
        ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (Subsystem && Mapping) Subsystem->RemoveMappingContext(Mapping);
    Mapping = NewObject<UInputMappingContext>(this);
    Actions.Reset();
    auto Axis = [this](const FKey& Positive, const FKey& Negative)
    {
        auto* Action = NewObject<UInputAction>(this);
        Action->ValueType = EInputActionValueType::Axis1D;
        Actions.Add(Action);
        Mapping->MapKey(Action, Positive);
        if (Negative.IsValid())
            Mapping->MapKey(Action, Negative).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
        return Action;
    };
    auto Button = [this](const FKey& Key)
    {
        auto* Action = NewObject<UInputAction>(this);
        Actions.Add(Action);
        Mapping->MapKey(Action, Key);
        return Action;
    };

    auto* PedalAction = Axis(EKeys::W, FKey());
    Input->BindAction(PedalAction, ETriggerEvent::Triggered, this, &ANammaBicycle::Pedal);
    Input->BindAction(PedalAction, ETriggerEvent::Completed, this, &ANammaBicycle::ReleaseAxis, 0);
    Input->BindAction(PedalAction, ETriggerEvent::Canceled, this, &ANammaBicycle::ReleaseAxis, 0);
    auto* RearAction = Axis(EKeys::S, FKey());
    Input->BindAction(RearAction, ETriggerEvent::Triggered, this, &ANammaBicycle::RearBrake);
    Input->BindAction(RearAction, ETriggerEvent::Completed, this, &ANammaBicycle::ReleaseAxis, 1);
    Input->BindAction(RearAction, ETriggerEvent::Canceled, this, &ANammaBicycle::ReleaseAxis, 1);
    auto* FrontAction = Axis(EKeys::SpaceBar, FKey());
    Input->BindAction(FrontAction, ETriggerEvent::Triggered, this, &ANammaBicycle::FrontBrake);
    Input->BindAction(FrontAction, ETriggerEvent::Completed, this, &ANammaBicycle::ReleaseAxis, 2);
    Input->BindAction(FrontAction, ETriggerEvent::Canceled, this, &ANammaBicycle::ReleaseAxis, 2);
    auto* SteerAction = Axis(EKeys::D, EKeys::A);
    Input->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ANammaBicycle::Steer);
    Input->BindAction(SteerAction, ETriggerEvent::Completed, this, &ANammaBicycle::ReleaseAxis, 3);
    Input->BindAction(SteerAction, ETriggerEvent::Canceled, this, &ANammaBicycle::ReleaseAxis, 3);
    Input->BindAction(Axis(EKeys::MouseX, FKey()), ETriggerEvent::Triggered, this, &ANammaBicycle::LookYaw);
    Input->BindAction(Axis(EKeys::MouseY, FKey()), ETriggerEvent::Triggered, this, &ANammaBicycle::LookPitch);

    auto* SprintAction = Button(EKeys::LeftShift);
    Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ANammaBicycle::SprintStart);
    Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ANammaBicycle::SprintEnd);
    Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ANammaBicycle::SprintEnd);
    Input->BindAction(Button(EKeys::MouseScrollUp), ETriggerEvent::Started, this, &ANammaBicycle::ShiftUp);
    Input->BindAction(Button(EKeys::MouseScrollDown), ETriggerEvent::Started, this, &ANammaBicycle::ShiftDown);
    const FKey GearKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six};
    for (int32 Slot = 0; Slot < Movement->GetSetup().GearCount && Slot < 6; ++Slot)
        Input->BindAction(Button(GearKeys[Slot]), ETriggerEvent::Started, this, &ANammaBicycle::SelectGear, Slot);
    Input->BindAction(Button(EKeys::E), ETriggerEvent::Started, this, &ANammaBicycle::RequestDismount);
    Input->BindAction(Button(EKeys::X), ETriggerEvent::Started, this, &ANammaBicycle::Bail);

    auto* PauseAction = Button(EKeys::Escape);
    PauseAction->bTriggerWhenPaused = true;
    Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ANammaBicycle::TogglePause);
    auto* RestartAction = Button(EKeys::R);
    RestartAction->bTriggerWhenPaused = true;
    Input->BindAction(RestartAction, ETriggerEvent::Started, this, &ANammaBicycle::RestartDelivery);
    auto* QuitAction = Button(EKeys::Q);
    QuitAction->bTriggerWhenPaused = true;
    Input->BindAction(QuitAction, ETriggerEvent::Started, this, &ANammaBicycle::Quit);
    if (Subsystem) Subsystem->AddMappingContext(Mapping, 0);
}

void ANammaBicycle::Pedal(const FInputActionValue& Value)
{
    if (!IsPaused()) Controls.Pedal = FMath::Clamp(Value.Get<float>(), 0.f, 1.f);
}
void ANammaBicycle::RearBrake(const FInputActionValue& Value)
{
    if (!IsPaused()) Controls.RearBrake = FMath::Clamp(Value.Get<float>(), 0.f, 1.f);
}
void ANammaBicycle::FrontBrake(const FInputActionValue& Value)
{
    if (!IsPaused()) Controls.FrontBrake = FMath::Clamp(Value.Get<float>(), 0.f, 1.f);
}
void ANammaBicycle::Steer(const FInputActionValue& Value)
{
    if (!IsPaused()) Controls.Steer = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
}
void ANammaBicycle::ReleaseAxis(int32 Which)
{
    switch (Which)
    {
    case 0: Controls.Pedal = 0.0; break;
    case 1: Controls.RearBrake = 0.0; break;
    case 2: Controls.FrontBrake = 0.0; break;
    default: Controls.Steer = 0.0; break;
    }
}
void ANammaBicycle::LookYaw(const FInputActionValue& Value)
{
    if (!IsPaused()) AddControllerYawInput(Value.Get<float>());
}
void ANammaBicycle::LookPitch(const FInputActionValue& Value)
{
    if (!IsPaused()) AddControllerPitchInput(-Value.Get<float>());
}
void ANammaBicycle::SprintStart() { if (!IsPaused()) bSprint = true; }
void ANammaBicycle::SprintEnd() { bSprint = false; }
void ANammaBicycle::ShiftUp() { SelectGear(Gear + 1); }
void ANammaBicycle::ShiftDown() { SelectGear(Gear - 1); }
void ANammaBicycle::SelectGear(int32 Requested)
{
    if (!IsPaused()) Gear = FMath::Clamp(Requested, 0, Movement->GetSetup().GearCount - 1);
}
void ANammaBicycle::RequestDismount() { if (!IsPaused()) Dismount(false); }
void ANammaBicycle::Bail() { if (!IsPaused()) Dismount(true); }
void ANammaBicycle::TogglePause()
{
    SprintEnd();
    UGameplayStatics::SetGamePaused(this, !IsPaused());
}
void ANammaBicycle::RestartDelivery()
{
    auto* Mode = GetWorld()->GetAuthGameMode<ANammaCityGameModeBase>();
    if (Mode && (IsPaused() || Mode->GetDeliveryStage() == ENammaDeliveryStage::Complete))
    {
        UGameplayStatics::SetGamePaused(this, false);
        if (Rider.IsValid()) Dismount(false);
        Movement->ResetTo(StartTransform);
        Mode->RestartDelivery();
    }
}
void ANammaBicycle::Quit()
{
    if (IsPaused())
        UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false);
}

#if !UE_BUILD_SHIPPING
namespace
{
ANammaBicycle* NearestCycle(UWorld* World, const FVector& To)
{
    ANammaBicycle* Best = nullptr;
    double Closest = TNumericLimits<double>::Max();
    for (TActorIterator<ANammaBicycle> It(World); It; ++It)
    {
        const double Distance = FVector::DistSquared(It->GetActorLocation(), To);
        if (Distance < Closest) { Closest = Distance; Best = *It; }
    }
    return Best;
}

// Debug travel only: it puts the player within interaction range of a cycle. Getting
// on is still the ordinary E interaction, so this shortens a playtest walk without
// bypassing any of the mounting code.
FAutoConsoleCommandWithWorld GNammaCycleGoTo(
    TEXT("Namma.Cycle.GoTo"),
    TEXT("Teleport the player beside the nearest cycle. Mounting still needs E."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Player = Cast<ANammaPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
        ANammaBicycle* Cycle = Player ? NearestCycle(World, Player->GetActorLocation()) : nullptr;
        if (!Cycle) { UE_LOG(LogNammaVehicle, Warning, TEXT("No cycle in this level")); return; }
        const FVector Beside = Cycle->GetActorLocation() - Cycle->GetActorRightVector() * 150.f
            + FVector(0.f, 0.f, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        Player->SetActorLocation(Beside, false, nullptr, ETeleportType::TeleportPhysics);
        // Face the cycle so the interaction cone is already looking at it.
        const FRotator Facing = (Cycle->GetActorLocation() - Beside).Rotation();
        Player->SetActorRotation(FRotator(0.f, Facing.Yaw, 0.f));
        if (auto* PC = Cast<APlayerController>(Player->GetController()))
            PC->SetControlRotation(FRotator(-10.f, Facing.Yaw, 0.f));
        UE_LOG(LogNammaVehicle, Log, TEXT("Player moved beside the cycle at %s"),
            *Cycle->GetActorLocation().ToCompactString());
    }));

FAutoConsoleCommandWithWorld GNammaCycleStatus(
    TEXT("Namma.Cycle.Status"),
    TEXT("Log drivetrain and contact telemetry for every cycle in the level."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        for (TActorIterator<ANammaBicycle> It(World); It; ++It)
            UE_LOG(LogNammaVehicle, Log, TEXT("%s | %s | %s | rider %s"), *It->GetName(),
                *It->GetTelemetryLine().ToString(), *It->GetGearLine().ToString(),
                It->HasRider() ? TEXT("aboard") : TEXT("none"));
    }));
}  // namespace
#endif
