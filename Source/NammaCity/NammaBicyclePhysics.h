#pragma once

// Engine-independent single-track bicycle dynamics in SI units (m, kg, s, rad, N).
// Shared by the Unreal movement component and the host test, exactly as
// NammaDeliveryState.h is shared by the game mode and Tests/delivery_state_test.cpp.
//
// Defaults describe VEH-01-002 "Indian Roadster Bicycle"
// (docs/vehicles/01_cycles/VEH-01-002_indian_roadster_bicycle.md) carrying the
// project's 75 kg rider. docs/phase-1/CYCLE_RIDING.md derives every number and
// records which ones the specification sheet fixes and which are calculated.

#include <algorithm>
#include <cmath>

namespace NammaBicycle
{
inline constexpr double Pi = 3.14159265358979323846;
inline constexpr double Gravity = 9.81;
inline constexpr double AirDensity = 1.225;
// ISO 606 half-inch bicycle chain. One link per 12.7 mm of chain travel, which
// also fixes every sprocket pitch radius: r = pitch * teeth / 2pi.
inline constexpr double ChainPitch = 0.0127;
inline constexpr int MaxGears = 8;

inline double Sign(double Value) { return Value > 0.0 ? 1.0 : (Value < 0.0 ? -1.0 : 0.0); }

struct FSetup
{
    // Geometry. Wheelbase and mass come from the specification sheet; the
    // combined centre of mass is computed from the 13 kg frame and 75 kg rider.
    double Wheelbase = 1.048;
    double WheelRadius = 0.350;            // 700C rim plus a 35 mm roadster tyre
    double CentreOfMassHeight = 1.037;
    double CentreOfMassToRearAxle = 0.470; // 45/55 front/rear static split
    double ChainstayLength = 0.465;        // bottom bracket to rear axle
    double BikeMass = 13.0;
    double RiderMass = 75.0;
    double WheelMass = 2.2;                // steel rim, tube and tyre

    // Drivetrain. A 46 T chainring on a six-speed freewheel: 4.21 m to 7.23 m of
    // road per crank revolution.
    int ChainringTeeth = 46;
    int GearCount = 6;
    int SprocketTeeth[MaxGears] = {24, 22, 20, 18, 16, 14, 0, 0};
    double SprocketSpacing = 0.0055;
    double CrankLength = 0.170;
    double DrivetrainEfficiency = 0.95;
    double ShiftEfficiency = 0.35;         // chain climbing the shift ramps
    double DerailleurSpeed = 0.09;         // m/s of cage travel
    double CrossChainAngle = 0.021;        // rad; beyond this the chain runs skewed
    double CrossChainPenalty = 0.02;

    // Rider. Torque caps the standing start, power caps the cruise, and cadence
    // caps how fast the legs can turn at all.
    double MaxCrankTorque = 140.0;         // peak, out of the saddle
    double SustainedPower = 310.0;
    double SprintPower = 520.0;
    double PedalDeadSpot = 0.35;           // floor of the two-legged torque curve
    double CadenceRollOff = 105.0;         // rpm where the legs start losing torque
    double MaxCadence = 140.0;

    // Brakes. Sized so the front can lock a fully loaded front wheel and the rear
    // locks easily once braking has moved the load forward.
    double MaxFrontBrakeTorque = 320.0;
    double MaxRearBrakeTorque = 200.0;

    // Tyre. Simplified Pacejka: F = mu*N*sin(C*atan(B*slip)), peaking near 15 %.
    double TyreShape = 1.45;              // sliding grip stays at 76 % of peak
    double TyreStiffness = 12.0;
    double SlipReferenceSpeed = 1.5;       // regularises slip ratio near standstill

    // Aerodynamics. Upright roadster posture.
    double DragCoefficient = 0.9;
    double FrontalArea = 0.55;

    // Steering and balance.
    double MaxSteerAngle = 0.559;          // 32 deg, giving the 1.7 m turning circle
    double SteerRate = 3.2;                // rad/s of handlebar movement
    double LeanFrequency = 6.0;            // rider's roll controller
    double LeanDamping = 0.9;
    double GripSafety = 0.95;              // lean the rider is willing to hold
    double FootDownSpeed = 0.6;
    double SlideYawFactor = 0.55;          // how much yaw survives a lateral slide
    double LoadTransferTime = 0.08;        // fork dive is not instantaneous
    double PitchStiffness = 120.0;          // frame/fork return once both wheels land
    double PitchDamping = 8.0;
    double EndoAngle = 0.55;               // past atan(Lf/h) = 29 deg, gravity takes over
    double SlideCrashTime = 0.45;
};

struct FRiderInput
{
    double Pedal = 0.0;         // 0..1
    double FrontBrake = 0.0;    // 0..1
    double RearBrake = 0.0;     // 0..1
    double Steer = 0.0;         // -1..1, positive turns right
    int Gear = 3;
    bool bSprint = false;
};

struct FSurface
{
    double Friction = 0.85;            // dry asphalt
    double RollingResistance = 0.008;
    double Grade = 0.0;                // rad, positive uphill along travel
    double FrontLoadScale = 1.0;       // 0 while the wheel is off the ground
    double FrontFriction = -1.0;       // negative inherits Friction
    double RearFriction = -1.0;
    double RearLoadScale = 1.0;
};

struct FChain
{
    double ChainringRadius = 0.0;
    double SprocketRadius = 0.0;
    double SpanLength = 0.0;       // straight run between the two tangent points
    double WrapChainring = 0.0;    // rad of chain wrapped on the chainring
    double WrapSprocket = 0.0;
    double TotalLength = 0.0;
    double LinkCount = 0.0;
    double Speed = 0.0;            // m/s, positive when driving forward
    double Tension = 0.0;          // N in the loaded upper run
    double LateralOffset = 0.0;    // m from the chainring plane at the sprocket
    double LineAngle = 0.0;        // rad of chain-line skew
    bool bCrossChained = false;
};

struct FState
{
    double Speed = 0.0;              // m/s along the heading
    double FrontWheelRate = 0.0;     // rad/s
    double RearWheelRate = 0.0;
    double FrontWheelAngle = 0.0;    // rad, accumulated for rendering
    double RearWheelAngle = 0.0;
    double CassetteAngle = 0.0;       // freewheel carrier follows chain, not coasting wheel
    double CrankAngle = 0.0;         // rad, 0 puts the right pedal at top dead centre
    double CrankRate = 0.0;
    int Gear = 3;
    double ChainLateral = 0.0;
    double ChainTravel = 0.0;        // m of chain that has passed a fixed point
    double SteerAngle = 0.0;         // rad
    double Lean = 0.0;               // rad, positive leans right
    double LeanRate = 0.0;
    double Pitch = 0.0;              // rad, positive is nose-up
    double PitchRate = 0.0;
    double Yaw = 0.0;                // rad
    double YawRate = 0.0;
    double DistanceTravelled = 0.0;
    double LastAcceleration = 0.0;   // drives the next step's load transfer
    double TransferAcceleration = 0.0;   // lagged, so the fork dives rather than snaps
    double SlideTime = 0.0;
    bool bCrashed = false;
};

struct FTelemetry
{
    double FrontLoad = 0.0;          // N
    double RearLoad = 0.0;
    double FrontTyreForce = 0.0;     // N, longitudinal, positive drives forward
    double RearTyreForce = 0.0;
    double FrontSlip = 0.0;
    double RearSlip = 0.0;
    double CrankTorque = 0.0;        // Nm
    double RearDriveTorque = 0.0;
    double GearRatio = 0.0;          // wheel revolutions per crank revolution
    double Development = 0.0;        // m of road per crank revolution
    double Cadence = 0.0;            // rpm
    double RiderPower = 0.0;         // W
    double DragForce = 0.0;
    double RollingForce = 0.0;
    double GradeForce = 0.0;
    double Acceleration = 0.0;       // m/s^2
    double TurnRadius = 0.0;         // m, 0 when running straight
    double DemandedLean = 0.0;       // rad
    double LateralForce = 0.0;       // N required to hold the corner
    double LateralGrip = 0.0;        // N available after longitudinal use
    FChain Chain;
    bool bFreewheeling = true;
    bool bShifting = false;
    bool bFrontLocked = false;
    bool bRearLocked = false;
    bool bSliding = false;
    bool bFootDown = false;
    bool bRearAirborne = false;
    bool bFrontAirborne = false;
};

inline double SprocketRadius(int Teeth) { return ChainPitch * Teeth / (2.0 * Pi); }

// Lateral position of each sprocket, centred on the chainring plane.
inline double SprocketOffset(const FSetup& Setup, int Gear)
{
    const int Count = std::max(1, Setup.GearCount);
    return (Gear - (Count - 1) * 0.5) * Setup.SprocketSpacing;
}

// The engaged radius comes from where the chain actually sits, so a shift in
// progress produces a continuously changing ratio instead of a snap.
inline double EngagedSprocketRadius(const FSetup& Setup, double Lateral)
{
    const int Count = std::max(1, Setup.GearCount);
    const double Position = std::clamp(Lateral / Setup.SprocketSpacing + (Count - 1) * 0.5,
                                       0.0, double(Count - 1));
    const int Low = int(std::floor(Position));
    const int High = std::min(Low + 1, Count - 1);
    const double Blend = Position - Low;
    return SprocketRadius(Setup.SprocketTeeth[Low]) * (1.0 - Blend)
         + SprocketRadius(Setup.SprocketTeeth[High]) * Blend;
}

// Open chain drive between two circles a chainstay apart. Tangent length and wrap
// angles are exact; their sum is the chain length the frame needs.
inline FChain ChainGeometry(const FSetup& Setup, double EngagedRadius, double Lateral)
{
    FChain Chain;
    Chain.ChainringRadius = SprocketRadius(Setup.ChainringTeeth);
    Chain.SprocketRadius = EngagedRadius;
    const double Centre = std::max(1e-4, Setup.ChainstayLength);
    const double Offset = std::clamp((Chain.ChainringRadius - Chain.SprocketRadius) / Centre, -1.0, 1.0);
    Chain.SpanLength = Centre * std::sqrt(std::max(0.0, 1.0 - Offset * Offset));
    Chain.WrapChainring = Pi + 2.0 * std::asin(Offset);
    Chain.WrapSprocket = Pi - 2.0 * std::asin(Offset);
    Chain.TotalLength = 2.0 * Chain.SpanLength
                      + Chain.ChainringRadius * Chain.WrapChainring
                      + Chain.SprocketRadius * Chain.WrapSprocket;
    Chain.LinkCount = Chain.TotalLength / ChainPitch;
    Chain.LateralOffset = Lateral;
    Chain.LineAngle = std::atan2(Lateral, Centre);
    Chain.bCrossChained = std::fabs(Chain.LineAngle) > Setup.CrossChainAngle;
    return Chain;
}

struct FChainPoint
{
    double X = 0.0;   // forward, metres from the bottom bracket
    double Y = 0.0;   // right
    double Z = 0.0;   // up
};

// Maps arc length around the chain loop to a point in the bike's own frame. Drawing a
// link at every multiple of the chain pitch reproduces the real thing: it wraps the
// chainring, spans to whichever sprocket the derailleur has selected, and steps
// sideways by exactly that sprocket's offset. Advancing the arc length by the chain
// speed makes the links travel at the speed the drivetrain actually computes.
inline FChainPoint ChainPathPoint(const FSetup& Setup, const FChain& Chain, double Distance)
{
    const double Centre = std::max(1e-4, Setup.ChainstayLength);
    const double Ring = Chain.ChainringRadius;
    const double Sprocket = Chain.SprocketRadius;
    // The two external tangents touch both circles along the same unit vector.
    const double Along = std::clamp((Ring - Sprocket) / Centre, -1.0, 1.0);
    const double Across = std::sqrt(std::max(0.0, 1.0 - Along * Along));
    const double Contact = std::atan2(Across, -Along);
    const double AxleX = -Centre;
    const double Span = Chain.SpanLength;
    const double SprocketArc = Sprocket * Chain.WrapSprocket;
    const double Total = std::max(1e-6, Chain.TotalLength);

    double Walk = std::fmod(Distance, Total);
    if (Walk < 0.0) Walk += Total;

    // The four tangent points, shared by the two runs.
    const double RingTopX = -Ring * Along,     RingTopZ = Ring * Across;
    const double SprocketTopX = AxleX - Sprocket * Along, SprocketTopZ = Sprocket * Across;
    const double RingBottomX = RingTopX,       RingBottomZ = -RingTopZ;
    const double SprocketBottomX = SprocketTopX, SprocketBottomZ = -SprocketTopZ;

    FChainPoint Point;
    if (Walk < Span)   // loaded upper run, chainring to sprocket
    {
        const double T = Walk / Span;
        Point.X = RingTopX + (SprocketTopX - RingTopX) * T;
        Point.Z = RingTopZ + (SprocketTopZ - RingTopZ) * T;
        Point.Y = Chain.LateralOffset * T;
        return Point;
    }
    Walk -= Span;
    if (Walk < SprocketArc)   // around the back of the engaged sprocket
    {
        const double Angle = Contact + Walk / std::max(1e-6, Sprocket);
        Point.X = AxleX + Sprocket * std::cos(Angle);
        Point.Z = Sprocket * std::sin(Angle);
        Point.Y = Chain.LateralOffset;
        return Point;
    }
    Walk -= SprocketArc;
    if (Walk < Span)   // slack lower run, back to the chainring
    {
        const double T = Walk / Span;
        Point.X = SprocketBottomX + (RingBottomX - SprocketBottomX) * T;
        Point.Z = SprocketBottomZ + (RingBottomZ - SprocketBottomZ) * T;
        Point.Y = Chain.LateralOffset * (1.0 - T);
        return Point;
    }
    Walk -= Span;   // around the front of the chainring
    const double Angle = -Contact + Walk / std::max(1e-6, Ring);
    Point.X = Ring * std::cos(Angle);
    Point.Z = Ring * std::sin(Angle);
    Point.Y = 0.0;
    return Point;
}

inline double TotalMass(const FSetup& Setup) { return Setup.BikeMass + Setup.RiderMass; }

// Deceleration at which the rear wheel leaves the ground, from moments about the
// front contact patch: a = g * (CG to front axle) / (CG height).
inline double EndoDeceleration(const FSetup& Setup)
{
    return Gravity * (Setup.Wheelbase - Setup.CentreOfMassToRearAxle) / Setup.CentreOfMassHeight;
}

// Two legs 180 deg apart. Tangential effectiveness is |sin| over a floor that
// stands in for the rider pulling through the dead spot and for crank inertia.
// Normalised to unit mean over a revolution, so the rider's quoted power really is
// the average delivered power and the shape only supplies the torque ripple. The
// instantaneous peak is 1/mean = 1.31 times the envelope.
inline double PedalEffectiveness(const FSetup& Setup, double CrankAngle)
{
    const double Mean = Setup.PedalDeadSpot + (1.0 - Setup.PedalDeadSpot) * 2.0 / Pi;
    return (Setup.PedalDeadSpot + (1.0 - Setup.PedalDeadSpot) * std::fabs(std::sin(CrankAngle)))
         / std::max(1e-6, Mean);
}

inline double SlipRatio(const FSetup& Setup, double WheelRate, double Speed)
{
    const double Contact = WheelRate * Setup.WheelRadius;
    const double Reference = std::max(std::fabs(Speed), Setup.SlipReferenceSpeed);
    return std::clamp((Contact - Speed) / Reference, -3.0, 3.0);
}

inline double TyreLongitudinalForce(const FSetup& Setup, double Slip, double Friction, double Load,
                                    double* Gradient = nullptr)
{
    if (Gradient) *Gradient = 0.0;
    const double Peak = Friction * std::max(0.0, Load);
    if (Peak <= 0.0) return 0.0;
    const double Stiffness = Setup.TyreStiffness / (Setup.TyreShape * Friction);
    const double Inner = std::atan(Stiffness * Slip);
    if (Gradient)
    {
        const double Tangent = Stiffness * Slip;
        *Gradient = Peak * Setup.TyreShape * Stiffness * std::cos(Setup.TyreShape * Inner)
                  / (1.0 + Tangent * Tangent);
    }
    return Peak * std::sin(Setup.TyreShape * Inner);
}

// Advances one substep. Callers integrate at a fixed rate; Dt is clamped so a
// stalled frame cannot inject an unbounded impulse.
inline FTelemetry Step(const FSetup& Setup, FState& State, const FRiderInput& Input,
                       const FSurface& Surface, double Dt)
{
    FTelemetry Out;
    Dt = std::clamp(Dt, 1e-5, 0.02);
    const double Mass = TotalMass(Setup);
    const double Radius = Setup.WheelRadius;
    const double WheelInertia = std::max(1e-6, Setup.WheelMass * Radius * Radius); // thin hoop
    const double Friction = std::max(0.05, Surface.Friction);
    const double FrontFriction = Surface.FrontFriction >= 0.0 ? Surface.FrontFriction : Friction;
    const double RearFriction = Surface.RearFriction >= 0.0 ? Surface.RearFriction : Friction;

    // ---- derailleur ---------------------------------------------------------
    State.Gear = std::clamp(Input.Gear, 0, Setup.GearCount - 1);
    const double TargetLateral = SprocketOffset(Setup, State.Gear);
    const double LateralStep = (State.CrankRate > 0.1 || Input.Pedal > 0.01) ? Setup.DerailleurSpeed * Dt : 0.0;
    if (std::fabs(TargetLateral - State.ChainLateral) <= LateralStep) State.ChainLateral = TargetLateral;
    else State.ChainLateral += Sign(TargetLateral - State.ChainLateral) * LateralStep;
    Out.bShifting = std::fabs(TargetLateral - State.ChainLateral) > 1e-6;

    const double EngagedRadius = EngagedSprocketRadius(Setup, State.ChainLateral);
    Out.Chain = ChainGeometry(Setup, EngagedRadius, State.ChainLateral);
    // The ratio is the chain geometry, not a separate table: equal chain speed at
    // both ends means wheel revs per crank rev is exactly ring radius / sprocket radius.
    const double Ratio = Out.Chain.ChainringRadius / std::max(1e-6, EngagedRadius);
    Out.GearRatio = Ratio;
    Out.Development = Ratio * 2.0 * Pi * Radius;

    double Efficiency = Setup.DrivetrainEfficiency;
    if (Out.bShifting) Efficiency *= Setup.ShiftEfficiency;
    if (Out.Chain.bCrossChained) Efficiency *= (1.0 - Setup.CrossChainPenalty);

    // ---- rider and freewheel -----------------------------------------------
    // The freewheel only carries torque forwards, and only while the legs can keep
    // up with the wheel.
    const double DrivenCrankRate = std::max(0.0, State.RearWheelRate) / Ratio;
    const double MaxCrankRate = Setup.MaxCadence * 2.0 * Pi / 60.0;
    const double Pedal = std::clamp(Input.Pedal, 0.0, 1.0);
    const bool bSpunOut = DrivenCrankRate > MaxCrankRate;
    const bool bEngaged = Pedal > 0.01 && !bSpunOut && State.Speed > -0.2;

    // Preserve crank phase at rest; the dead-spot torque floor permits starting
    // without teleporting the pedals (and the rider feet) through a quarter turn.
    if (bEngaged) State.CrankRate = DrivenCrankRate;
    else if (bSpunOut && Pedal > 0.01) State.CrankRate = MaxCrankRate;  // legs spinning, no useful torque
    else State.CrankRate = std::max(0.0, State.CrankRate - 3.0 * Dt);   // legs coast to rest

    Out.Cadence = State.CrankRate * 60.0 / (2.0 * Pi);
    double CrankTorque = 0.0;
    if (bEngaged)
    {
        const double Power = Input.bSprint ? Setup.SprintPower : Setup.SustainedPower;
        // Torque-limited off the line, power-limited once the legs are turning.
        double Available = std::min(Setup.MaxCrankTorque, Power / std::max(0.5, State.CrankRate));
        const double RollOff = (Setup.MaxCadence - Setup.CadenceRollOff);
        if (RollOff > 0.0)
            Available *= std::clamp(1.0 - (Out.Cadence - Setup.CadenceRollOff) / RollOff, 0.0, 1.0);
        CrankTorque = Pedal * Available * PedalEffectiveness(Setup, State.CrankAngle);
    }
    Out.CrankTorque = CrankTorque;
    Out.RiderPower = CrankTorque * State.CrankRate;
    Out.bFreewheeling = !bEngaged;
    Out.RearDriveTorque = CrankTorque * Efficiency / Ratio;
    Out.Chain.Tension = CrankTorque / std::max(1e-6, Out.Chain.ChainringRadius);

    // ---- load transfer ------------------------------------------------------
    // Moments about the contact patches, using the previous step's acceleration.
    const double Normal = Mass * Gravity * std::cos(Surface.Grade);
    const double ToFront = Setup.Wheelbase - Setup.CentreOfMassToRearAxle;
    State.TransferAcceleration += (State.LastAcceleration - State.TransferAcceleration)
                               * (1.0 - std::exp(-Dt / std::max(1e-3, Setup.LoadTransferTime)));
    const double Transfer = Mass * (State.TransferAcceleration + Gravity * std::sin(Surface.Grade))
                          * Setup.CentreOfMassHeight / Setup.Wheelbase;
    const double RawFront = Normal * Setup.CentreOfMassToRearAxle / Setup.Wheelbase - Transfer;
    const double RawRear = Normal * ToFront / Setup.Wheelbase + Transfer;

    // ---- pitch: stoppie and wheelie ----------------------------------------
    // Once a raw load goes negative the bike rotates about the other contact patch.
    // RawRear * Wheelbase is exactly the moment about the front contact patch, so a
    // negative rear load and a nose-over moment are the same condition. Nothing damps
    // the rotation once a wheel is in the air; on both wheels the frame springs back.
    const double PitchInertia = Mass * (Setup.CentreOfMassHeight * Setup.CentreOfMassHeight + ToFront * ToFront);
    const double PitchCos = std::cos(State.Pitch);
    const double PitchSin = std::sin(State.Pitch);
    const double Height = Setup.CentreOfMassHeight;
    const double Weight = Mass * Gravity * std::cos(Surface.Grade);
    const double Inertial = Mass * State.TransferAcceleration;
    double PitchAccel;
    if (RawRear <= 0.0)
    {
        // Rear in the air: centre of mass at (-Lf, h) from the front contact, rotated.
        const double X = -ToFront * PitchCos - Height * PitchSin;
        const double Z = -ToFront * PitchSin + Height * PitchCos;
        PitchAccel = (-Weight * X + Inertial * Z) / PitchInertia;
    }
    else if (RawFront <= 0.0)
    {
        // Front in the air: centre of mass at (+Lr, h) from the rear contact, rotated.
        const double X = Setup.CentreOfMassToRearAxle * PitchCos - Height * PitchSin;
        const double Z = Setup.CentreOfMassToRearAxle * PitchSin + Height * PitchCos;
        PitchAccel = (-Weight * X + Inertial * Z) / PitchInertia;
    }
    else PitchAccel = -Setup.PitchStiffness * State.Pitch - Setup.PitchDamping * State.PitchRate;
    State.PitchRate += PitchAccel * Dt;
    State.Pitch = std::clamp(State.Pitch + State.PitchRate * Dt, -1.2, 1.2);
    if (RawRear > 0.0 && RawFront > 0.0 && std::fabs(State.Pitch) < 1e-3 && std::fabs(State.PitchRate) < 1e-2)
    {
        State.Pitch = 0.0;
        State.PitchRate = 0.0;
    }
    Out.bRearAirborne = State.Pitch < -1e-3;
    Out.bFrontAirborne = State.Pitch > 1e-3;

    Out.FrontLoad = std::max(0.0, RawFront) * std::clamp(Surface.FrontLoadScale, 0.0, 2.0);
    Out.RearLoad = std::max(0.0, RawRear) * std::clamp(Surface.RearLoadScale, 0.0, 2.0);
    if (Out.bRearAirborne) Out.RearLoad = 0.0;
    if (Out.bFrontAirborne) Out.FrontLoad = 0.0;

    // ---- wheels -------------------------------------------------------------
    // Rolling resistance is a wheel torque, not a chassis force, so coasting
    // decelerates through the contact patch like the real thing.
    const double FrontBrake = std::clamp(Input.FrontBrake, 0.0, 1.0) * Setup.MaxFrontBrakeTorque;
    const double RearBrake = std::clamp(Input.RearBrake, 0.0, 1.0) * Setup.MaxRearBrakeTorque;

    // A bicycle tyre near zero slip is stiff enough that explicit integration would
    // need a sub-millisecond step. Linearising the tyre about the current slip and
    // solving the wheel update implicitly removes that limit entirely.
    auto Spin = [&](double& Rate, double& Angle, double Drive, double Brake, double Load, double Mu,
                    double& TyreForce, double& Slip)
    {
        const double Reference = std::max(std::fabs(State.Speed), Setup.SlipReferenceSpeed);
        Slip = std::clamp((Rate * Radius - State.Speed) / Reference, -3.0, 3.0);
        double Gradient = 0.0;
        TyreForce = TyreLongitudinalForce(Setup, Slip, Mu, Load, &Gradient);
        const double Rolling = Surface.RollingResistance * Load * Radius;
        const double Applied = Drive - TyreForce * Radius;
        bool bLocked = false;
        if (Rate == 0.0 && Brake >= std::fabs(Applied) - Rolling)
        {
            bLocked = Brake > 0.0;  // brake holds the wheel against the contact patch
        }
        else
        {
            const double Torque = Applied - (Brake + Rolling) * Sign(Rate);
            const double Damping = Gradient * Radius * Radius / Reference;
            const double Next = Rate + Torque / (WheelInertia / Dt + Damping);
            // Braking can stop a wheel but never drive it backwards.
            if (Brake > 0.0 && Rate != 0.0 && Sign(Next) != Sign(Rate)) { Rate = 0.0; bLocked = true; }
            else Rate = Next;
        }
        Angle = std::fmod(Angle + Rate * Dt, 2.0 * Pi);
        // Re-evaluate at the end-of-step wheel rate so the chassis sees the force the
        // implicit solve actually converged to.
        Slip = std::clamp((Rate * Radius - State.Speed) / Reference, -3.0, 3.0);
        TyreForce = TyreLongitudinalForce(Setup, Slip, Mu, Load);
        return bLocked && std::fabs(State.Speed) > 0.05;
    };
    Out.bFrontLocked = Spin(State.FrontWheelRate, State.FrontWheelAngle, 0.0, FrontBrake,
                            Out.FrontLoad, FrontFriction, Out.FrontTyreForce, Out.FrontSlip);
    Out.bRearLocked = Spin(State.RearWheelRate, State.RearWheelAngle, Out.RearDriveTorque, RearBrake,
                           Out.RearLoad, RearFriction, Out.RearTyreForce, Out.RearSlip);

    // The chain is rigid: with the freewheel engaged the crank and the rear wheel end
    // the step in exact lockstep, which is what keeps the drawn cranks, chain and
    // wheel from sliding against each other.
    if (bEngaged) State.CrankRate = std::max(0.0, State.RearWheelRate) / Ratio;
    State.CrankAngle = std::fmod(State.CrankAngle + State.CrankRate * Dt, 2.0 * Pi);
    Out.Cadence = State.CrankRate * 60.0 / (2.0 * Pi);
    Out.Chain.Speed = State.CrankRate * Out.Chain.ChainringRadius;
    State.ChainTravel += Out.Chain.Speed * Dt;
    State.CassetteAngle = std::fmod(State.CassetteAngle + Out.Chain.Speed / EngagedRadius * Dt, 2.0 * Pi);

    // ---- chassis ------------------------------------------------------------
    Out.DragForce = 0.5 * AirDensity * Setup.DragCoefficient * Setup.FrontalArea * State.Speed * std::fabs(State.Speed);
    Out.GradeForce = Mass * Gravity * std::sin(Surface.Grade);
    Out.RollingForce = Surface.RollingResistance * (Out.FrontLoad + Out.RearLoad) * Sign(State.Speed);
    const double Longitudinal = Out.FrontTyreForce + Out.RearTyreForce - Out.DragForce - Out.GradeForce;
    Out.Acceleration = Longitudinal / Mass;
    State.LastAcceleration = Out.Acceleration;
    State.Speed += Out.Acceleration * Dt;
    // At rest the bike is held by the brakes, or on a near-flat road by rolling
    // resistance alone. A gradient steeper than that still rolls it away.
    const double Holding = (FrontBrake + RearBrake) > 0.0
        ? Friction * Normal : Surface.RollingResistance * Normal;
    if (std::fabs(State.Speed) < 0.02 && Pedal < 0.01 && std::fabs(Out.GradeForce) <= Holding)
        State.Speed = 0.0;
    State.DistanceTravelled += State.Speed * Dt;

    // ---- steering, lean and yaw --------------------------------------------
    Out.bFootDown = std::fabs(State.Speed) < Setup.FootDownSpeed && Pedal < 0.01;
    const double Speed = State.Speed;
    // Steering is limited by grip, not by a hand-tuned speed curve: the lock is
    // whatever still keeps the required lean inside the friction cone.
    const double LeanLimit = std::atan(Friction * Setup.GripSafety);
    double SteerLimit = Setup.MaxSteerAngle;
    if (std::fabs(Speed) > 0.5)
        SteerLimit = std::min(SteerLimit,
            std::atan(Gravity * Setup.Wheelbase * std::tan(LeanLimit) / (Speed * Speed)));
    const double SteerTarget = std::clamp(Input.Steer, -1.0, 1.0) * SteerLimit;
    const double SteerStep = Setup.SteerRate * Dt;
    State.SteerAngle += std::clamp(SteerTarget - State.SteerAngle, -SteerStep, SteerStep);

    // Kinematic single-track yaw, then the lean that balances it.
    double YawDemand = Speed * std::tan(State.SteerAngle) / Setup.Wheelbase;
    Out.TurnRadius = std::fabs(std::tan(State.SteerAngle)) > 1e-4
        ? Setup.Wheelbase / std::fabs(std::tan(State.SteerAngle)) : 0.0;
    const double FrontGrip = FrontFriction * Out.FrontLoad;
    const double RearGrip = RearFriction * Out.RearLoad;
    // Friction circle: cornering gets whatever the tyres are not already spending.
    Out.LateralGrip = std::sqrt(std::max(0.0, FrontGrip * FrontGrip - Out.FrontTyreForce * Out.FrontTyreForce))
                    + std::sqrt(std::max(0.0, RearGrip * RearGrip - Out.RearTyreForce * Out.RearTyreForce));
    // A bike held at lean phi needs m*g*tan(phi) sideways from the road simply to stay
    // up, whatever the handlebars are doing. That, not the steering demand, is what
    // decides a lowside: it is why wet paint under a leaned two-wheeler drops it.
    Out.LateralForce = Mass * Gravity * std::cos(Surface.Grade) * std::fabs(std::tan(State.Lean));
    Out.bSliding = Out.LateralForce > Out.LateralGrip + 1e-6;
    const bool bGrounded = (Out.FrontLoad + Out.RearLoad) > 1.0;
    double Authority = 1.0;
    if (Out.bSliding && Out.LateralForce > 1e-6)
    {
        Authority = std::clamp(Out.LateralGrip / Out.LateralForce, 0.0, 1.0);
        YawDemand *= Authority + (1.0 - Authority) * Setup.SlideYawFactor;
        State.SlideTime += Dt;
    }
    else State.SlideTime = std::max(0.0, State.SlideTime - Dt);
    if (!bGrounded) Authority = 1.0;   // airborne there is nothing to topple against

    if (!bGrounded) YawDemand = State.YawRate; // steering cannot turn the trajectory in mid-air
    State.YawRate = YawDemand;
    State.Yaw = std::fmod(State.Yaw + State.YawRate * Dt, 2.0 * Pi);

    Out.DemandedLean = std::clamp(std::atan2(Speed * YawDemand, Gravity), -LeanLimit, LeanLimit);
    if (Out.bFootDown) Out.DemandedLean = 0.0;
    // The rider is a competent second-order roll controller, so a straight line never
    // falls over on its own. Losing grip costs that authority, and what is left is an
    // inverted pendulum that keeps going over.
    const double Frequency = Setup.LeanFrequency;
    const double Controlled = Frequency * Frequency * (Out.DemandedLean - State.Lean)
                            - 2.0 * Setup.LeanDamping * Frequency * State.LeanRate;
    const double Toppling = Gravity / Setup.CentreOfMassHeight * std::sin(State.Lean);
    State.LeanRate += (Authority * Controlled + (1.0 - Authority) * Toppling) * Dt;
    State.Lean = std::clamp(State.Lean + State.LeanRate * Dt, -1.4, 1.4);

    if (State.Pitch < -Setup.EndoAngle || State.SlideTime > Setup.SlideCrashTime
        || std::fabs(State.Lean) >= 1.4) State.bCrashed = true;
    return Out;
}
}  // namespace NammaBicycle
