// Engine-independent verification of the bicycle dynamics in
// Source/NammaCity/NammaBicyclePhysics.h. Runs on the host with no Unreal.
#include "../Source/NammaCity/NammaBicyclePhysics.h"

#include <cmath>
#include "../Source/NammaCity/NammaBicycleChain.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace NammaBicycle;

static int Failures = 0;

static void Check(bool Condition, const std::string& What)
{
    if (!Condition)
    {
        std::printf("FAIL: %s\n", What.c_str());
        ++Failures;
    }
}

static void Near(double Value, double Expected, double Tolerance, const std::string& What)
{
    if (!(std::fabs(Value - Expected) <= Tolerance))
    {
        std::printf("FAIL: %s (got %.6f, expected %.6f +/- %.6f)\n",
                    What.c_str(), Value, Expected, Tolerance);
        ++Failures;
    }
}

static bool Finite(const FState& S)
{
    const double Values[] = {S.Speed, S.FrontWheelRate, S.RearWheelRate, S.FrontWheelAngle,
                             S.RearWheelAngle, S.CrankAngle, S.CrankRate, S.ChainLateral,
                             S.ChainTravel, S.SteerAngle, S.Lean, S.LeanRate, S.Pitch,
                             S.PitchRate, S.Yaw, S.YawRate, S.DistanceTravelled,
                             S.LastAcceleration, S.TransferAcceleration, S.SlideTime};
    for (double V : Values) if (!std::isfinite(V)) return false;
    return true;
}

// A fixed-step harness. 240 Hz is what the movement component substeps at.
struct FSim
{
    FSetup Setup;
    FState State;
    FSurface Surface;
    FTelemetry Last;
    double Dt = 1.0 / 240.0;

    FTelemetry Run(const FRiderInput& Input, double Seconds)
    {
        const int Steps = int(Seconds / Dt);
        for (int I = 0; I < Steps; ++I)
        {
            Last = Step(Setup, State, Input, Surface, Dt);
            if (!Finite(State)) { Check(false, "state stayed finite during Run"); break; }
        }
        return Last;
    }
};

static FRiderInput Pedalling(int Gear, bool bSprint = false)
{
    FRiderInput In;
    In.Pedal = 1.0;
    In.Gear = Gear;
    In.bSprint = bSprint;
    return In;
}

// ---------------------------------------------------------------------------
static void TestChainAndGears()
{
    const FSetup Setup;
    for (int Gear = 0; Gear < Setup.GearCount; ++Gear)
    {
        const double Engaged = SprocketRadius(Setup.SprocketTeeth[Gear]);
        const FChain Chain = ChainGeometry(Setup, Engaged, SprocketOffset(Setup, Gear));
        const double Ratio = Chain.ChainringRadius / Chain.SprocketRadius;
        Near(Ratio, double(Setup.ChainringTeeth) / Setup.SprocketTeeth[Gear], 1e-12,
             "gear ratio equals the tooth ratio");

        // The chain physically has one speed: what the chainring pays out, the
        // sprocket must take up. This is the whole reason the ratio is what it is.
        const double CrankRate = 7.5;
        Near(CrankRate * Chain.ChainringRadius, CrankRate * Ratio * Chain.SprocketRadius, 1e-12,
             "chain speed matches at both sprockets");

        // Wrap angles and the tangent span must close the loop geometrically.
        Near(Chain.WrapChainring + Chain.WrapSprocket, 2.0 * Pi, 1e-12, "wrap angles sum to a full turn");
        const double Delta = Chain.ChainringRadius - Chain.SprocketRadius;
        Near(Chain.SpanLength * Chain.SpanLength + Delta * Delta,
             Setup.ChainstayLength * Setup.ChainstayLength, 1e-12, "tangent span closes on the chainstay");
        Check(Chain.TotalLength > 1.25 && Chain.TotalLength < 1.40, "chain length is buildable");
        Check(Chain.LinkCount > 98.0 && Chain.LinkCount < 112.0, "chain link count is a real chain");

        const double Development = Ratio * 2.0 * Pi * Setup.WheelRadius;
        Check(Development > 4.0 && Development < 7.5, "development stays in roadster range");
    }

    // Sprockets are centred on the chainring plane and ordered across the freewheel.
    Near(SprocketOffset(Setup, 0), -SprocketOffset(Setup, Setup.GearCount - 1), 1e-12,
         "cassette is centred on the chainring");
    for (int Gear = 1; Gear < Setup.GearCount; ++Gear)
        Check(SprocketOffset(Setup, Gear) > SprocketOffset(Setup, Gear - 1),
              "sprockets are laid out in order");

    // Higher gears are harder: bigger ratio, longer development, smaller sprocket.
    for (int Gear = 1; Gear < Setup.GearCount; ++Gear)
        Check(Setup.SprocketTeeth[Gear] < Setup.SprocketTeeth[Gear - 1], "gears are ordered easy to hard");

    // Mid-shift the chain sits between two sprockets and the ratio moves smoothly.
    const double Between = 0.5 * (SprocketOffset(Setup, 2) + SprocketOffset(Setup, 3));
    const double Radius = EngagedSprocketRadius(Setup, Between);
    Check(Radius < SprocketRadius(Setup.SprocketTeeth[2]) && Radius > SprocketRadius(Setup.SprocketTeeth[3]),
          "a shift in progress engages an intermediate radius");
}

// ---------------------------------------------------------------------------
static void TestChainPath()
{
    const FSetup Setup;
    for (int Gear : {0, 3, 5})
    {
        const FChain Chain = ChainGeometry(Setup, SprocketRadius(Setup.SprocketTeeth[Gear]),
                                           SprocketOffset(Setup, Gear));
        const double Axle = -Setup.ChainstayLength;
        const int Links = int(std::floor(Chain.LinkCount));

        // Every link sits either on one of the two sprockets or on a straight run
        // between them. Nothing may float off the drivetrain.
        double Longest = 0.0;
        FChainPoint Previous = ChainPathPoint(Setup, Chain, 0.0);
        const FChainPoint First = Previous;
        for (int Link = 1; Link <= Links; ++Link)
        {
            const FChainPoint Point = ChainPathPoint(Setup, Chain, Link * ChainPitch);
            const double OnRing = std::hypot(Point.X, Point.Z) - Chain.ChainringRadius;
            const double OnSprocket = std::hypot(Point.X - Axle, Point.Z) - Chain.SprocketRadius;
            Check(OnRing > -1e-9 && OnSprocket > -1e-9, "no link cuts inside a sprocket");
            Check(Point.X < Chain.ChainringRadius + 1e-9 && Point.X > Axle - Chain.SprocketRadius - 1e-9,
                  "the chain stays between the two axles");
            Check(std::fabs(Point.Z) <= Chain.ChainringRadius + 1e-9, "the chain stays inside the chainring");
            // Consecutive links are one chain pitch apart, allowing for the chord
            // across a sprocket tooth.
            const double Gap = std::sqrt((Point.X - Previous.X) * (Point.X - Previous.X)
                                       + (Point.Y - Previous.Y) * (Point.Y - Previous.Y)
                                       + (Point.Z - Previous.Z) * (Point.Z - Previous.Z));
            Longest = std::max(Longest, Gap);
            // Chain length is measured in the chain's own plane, so the derailleur's
            // sideways step adds 0.05 % to each link's span. Anything larger than a
            // tenth of a millimetre would be a real geometry error.
            Check(Gap <= ChainPitch + 1e-4, "no link stretches past the chain pitch");
            Previous = Point;
        }
        Check(Longest > ChainPitch * 0.98, "links really are a chain pitch apart on the runs");

        // The loop closes: walking the full length returns to the start.
        const FChainPoint Wrapped = ChainPathPoint(Setup, Chain, Chain.TotalLength);
        Near(Wrapped.X, First.X, 1e-9, "the chain loop closes in x");
        Near(Wrapped.Z, First.Z, 1e-9, "the chain loop closes in z");

        // The chain leaves the chainring in its own plane and arrives laterally over
        // the selected sprocket. That is what a derailleur actually does.
        Near(ChainPathPoint(Setup, Chain, 0.0).Y, 0.0, 1e-12, "the chain leaves the chainring plane");
        Near(ChainPathPoint(Setup, Chain, Chain.SpanLength).Y, SprocketOffset(Setup, Gear), 1e-12,
             "the chain arrives over the selected sprocket");

        // Wrap contact points touch both circles exactly.
        const FChainPoint AtSprocket = ChainPathPoint(Setup, Chain, Chain.SpanLength + 0.01);
        Near(std::hypot(AtSprocket.X - Axle, AtSprocket.Z), Chain.SprocketRadius, 1e-12,
             "wrapped links ride the sprocket pitch circle");
        const FChainPoint AtRing = ChainPathPoint(
            Setup, Chain, Chain.TotalLength - Chain.ChainringRadius * Chain.WrapChainring * 0.5);
        Near(std::hypot(AtRing.X, AtRing.Z), Chain.ChainringRadius, 1e-12,
             "wrapped links ride the chainring pitch circle");
    }

    // Selecting a different sprocket moves the chain sideways, and only sideways.
    const FChain Low = ChainGeometry(Setup, SprocketRadius(Setup.SprocketTeeth[0]), SprocketOffset(Setup, 0));
    const FChain High = ChainGeometry(Setup, SprocketRadius(Setup.SprocketTeeth[5]), SprocketOffset(Setup, 5));
    Check(ChainPathPoint(Setup, High, High.SpanLength).Y > ChainPathPoint(Setup, Low, Low.SpanLength).Y,
          "shifting up moves the chain across the freewheel");
}

// ---------------------------------------------------------------------------
static void TestDrivetrainAndFreewheel()
{
    FSim Sim;
    Sim.State.Speed = 6.0;
    Sim.State.RearWheelRate = 6.0 / Sim.Setup.WheelRadius;
    Sim.State.FrontWheelRate = Sim.State.RearWheelRate;

    FRiderInput Coasting;
    Coasting.Gear = 3;
    FTelemetry Out = Step(Sim.Setup, Sim.State, Coasting, Sim.Surface, Sim.Dt);
    Check(Out.bFreewheeling, "not pedalling freewheels");
    Near(Out.CrankTorque, 0.0, 1e-12, "freewheel transmits no torque");
    Check(Out.RearDriveTorque <= 0.0 + 1e-12, "freewheel never drives backwards");

    // Legs cannot outrun the freewheel: past maximum cadence the drive drops out.
    FSim Fast;
    Fast.State.Speed = 25.0;
    Fast.State.RearWheelRate = 25.0 / Fast.Setup.WheelRadius;
    Out = Step(Fast.Setup, Fast.State, Pedalling(5), Fast.Surface, Fast.Dt);
    Check(Out.bFreewheeling, "spun out past maximum cadence");
    Near(Out.Cadence, Fast.Setup.MaxCadence, 1e-9, "spun out, the legs sit at maximum cadence");
    Near(Out.RearDriveTorque, 0.0, 1e-12, "spun out, the legs contribute nothing");

    // Torque-limited off the line, power-limited at cruise.
    FSim Start;
    Out = Step(Start.Setup, Start.State, Pedalling(0), Start.Surface, Start.Dt);
    Near(Out.CrankTorque, Start.Setup.MaxCrankTorque * PedalEffectiveness(Start.Setup, 0.0),
         1e-9, "standing start respects torque limit and actual pedal leverage");
    Check(Start.State.CrankAngle < 0.1, "starting does not teleport the crank to the power position");
    FSim Cruise;
    Cruise.State.Speed = 9.0;
    Cruise.State.RearWheelRate = 9.0 / Cruise.Setup.WheelRadius;
    Out = Step(Cruise.Setup, Cruise.State, Pedalling(5), Cruise.Surface, Cruise.Dt);
    Check(Out.CrankTorque < Cruise.Setup.MaxCrankTorque * 0.5, "cruising is power limited");

    // The pedal torque curve averages to the rider's quoted power over a revolution.
    double Sum = 0.0;
    const int Samples = 20000;
    for (int I = 0; I < Samples; ++I)
        Sum += PedalEffectiveness(Cruise.Setup, 2.0 * Pi * I / Samples);
    Near(Sum / Samples, 1.0, 1e-3, "pedal torque curve has unit mean");
    Check(PedalEffectiveness(Cruise.Setup, 0.0) < PedalEffectiveness(Cruise.Setup, Pi * 0.5),
          "dead spot delivers less than the power stroke");
}

// ---------------------------------------------------------------------------
static void TestTopSpeedAndAcceleration()
{
    FSim Sim;
    Sim.Run(Pedalling(5), 400.0);
    const double TopKph = Sim.State.Speed * 3.6;
    // VEH-01-002 section 6 gives 33 km/h. The model has to land there from rider
    // power, drag, rolling resistance and drivetrain loss, not from a speed cap.
    Near(TopKph, 33.0, 1.5, "sustained top speed matches the specification sheet");
    Check(Sim.Last.Cadence > 60.0 && Sim.Last.Cadence < 95.0, "top gear cruises at a sane cadence");
    // Torque ripples over a crank revolution, so the rider's quoted power is the mean.
    double PowerSum = 0.0;
    const int PowerSteps = int(2.0 * Pi / std::max(0.1, Sim.State.CrankRate) / Sim.Dt);
    for (int I = 0; I < PowerSteps; ++I)
        PowerSum += Step(Sim.Setup, Sim.State, Pedalling(5), Sim.Surface, Sim.Dt).RiderPower;
    Near(PowerSum / PowerSteps, Sim.Setup.SustainedPower, 25.0,
         "mean cruise power is the rider's sustained power");

    // Sprinting is faster but still bounded by the same physics.
    FSim Sprint;
    Sprint.Run(Pedalling(5, true), 400.0);
    Check(Sprint.State.Speed > Sim.State.Speed + 1.0, "sprint power raises top speed");
    Check(Sprint.State.Speed * 3.6 < 48.0, "sprint stays inside a believable envelope");

    // Starting in the easiest gear accelerates harder than starting in the hardest.
    FSim Low, High;
    Low.Run(Pedalling(0), 3.0);
    High.Run(Pedalling(5), 3.0);
    Check(Low.State.Speed > High.State.Speed, "low gear accelerates harder from rest");
    Check(Low.State.LastAcceleration < 3.0, "a roadster start is not a motorcycle start");

    // No slip at cruise: the rear wheel is rolling, not spinning.
    Check(std::fabs(Sim.Last.RearSlip) < 0.05, "cruising rear wheel rolls without slipping");
    Near(Sim.State.RearWheelRate * Sim.Setup.WheelRadius, Sim.State.Speed, 0.35,
         "wheel surface speed tracks road speed");
}

// ---------------------------------------------------------------------------
static void TestCoastDown()
{
    FSim Sim;
    Sim.State.Speed = 8.0;
    Sim.State.RearWheelRate = Sim.State.FrontWheelRate = 8.0 / Sim.Setup.WheelRadius;
    FRiderInput Coast;
    Coast.Gear = 3;

    // The first step must match the closed-form drag plus rolling resistance.
    const FSetup& S = Sim.Setup;
    const double Mass = TotalMass(S);
    const double Expected = -(0.5 * AirDensity * S.DragCoefficient * S.FrontalArea * 64.0
                              + Sim.Surface.RollingResistance * Mass * Gravity) / Mass;
    FTelemetry Out = Step(Sim.Setup, Sim.State, Coast, Sim.Surface, Sim.Dt);
    Near(Out.Acceleration, Expected, 0.05, "coast-down deceleration matches drag plus rolling resistance");

    double Previous = Sim.State.Speed;
    for (int I = 0; I < 240 * 200; ++I)
    {
        Step(Sim.Setup, Sim.State, Coast, Sim.Surface, Sim.Dt);
        Check(Sim.State.Speed <= Previous + 1e-9, "coasting never speeds up on the flat");
        Previous = Sim.State.Speed;
        if (Sim.State.Speed <= 0.0) break;
    }
    Near(Sim.State.Speed, 0.0, 1e-6, "coasting comes to rest");
    Check(Sim.State.DistanceTravelled > 100.0, "a roadster coasts a long way from 29 km/h");
}

// ---------------------------------------------------------------------------
static double StoppingDistance(double Front, double Rear, double FromSpeed, double Friction,
                               bool* bCrashed = nullptr, double* bPeakDecel = nullptr)
{
    FSim Sim;
    Sim.Surface.Friction = Friction;
    Sim.State.Speed = FromSpeed;
    Sim.State.RearWheelRate = Sim.State.FrontWheelRate = FromSpeed / Sim.Setup.WheelRadius;
    FRiderInput In;
    In.Gear = 3;
    In.FrontBrake = Front;
    In.RearBrake = Rear;
    const double Start = Sim.State.DistanceTravelled;
    double Peak = 0.0;
    for (int I = 0; I < 240 * 30 && Sim.State.Speed > 0.01; ++I)
    {
        const FTelemetry Out = Step(Sim.Setup, Sim.State, In, Sim.Surface, Sim.Dt);
        Peak = std::max(Peak, -Out.Acceleration);
    }
    if (bCrashed) *bCrashed = Sim.State.bCrashed;
    if (bPeakDecel) *bPeakDecel = Peak;
    return Sim.State.DistanceTravelled - Start;
}

static void TestBraking()
{
    const FSetup Setup;
    // Section 25 asks for a believable class braking distance. 20 km/h is the
    // class-relevant test speed for a pedal cycle.
    const double From = 20.0 / 3.6;
    double Peak = 0.0;
    bool bCrashed = false;
    const double Both = StoppingDistance(0.35, 0.6, From, 0.85, &bCrashed, &Peak);
    Check(Both > 2.0 && Both < 6.0, "20 km/h stop is believable for a bicycle: " + std::to_string(Both) + " m");
    Check(!bCrashed, "a controlled two-brake stop does not crash");
    Check(Peak <= 0.85 * Gravity + 0.01, "deceleration never exceeds the friction limit");
    const double Mean = From * From / (2.0 * Both);
    Check(Mean < EndoDeceleration(Setup), "a controlled stop stays under the pitch-over limit: "
          + std::to_string(Mean) + " m/s2");

    // The front brake does most of the work, because braking moves the load onto it.
    const double FrontOnly = StoppingDistance(0.35, 0.0, From, 0.85);
    const double RearOnly = StoppingDistance(0.0, 1.0, From, 0.85);
    Check(FrontOnly < RearOnly, "front brake stops shorter than the rear");

    // A locked rear wheel skids instead of gripping.
    FSim Skid;
    Skid.State.Speed = From;
    Skid.State.RearWheelRate = Skid.State.FrontWheelRate = From / Skid.Setup.WheelRadius;
    FRiderInput Grab;
    Grab.Gear = 3;
    Grab.RearBrake = 1.0;
    const FTelemetry Locked = Skid.Run(Grab, 0.5);
    Check(Locked.bRearLocked, "a full rear brake locks the wheel");
    Near(Skid.State.RearWheelRate, 0.0, 1e-9, "a locked wheel stops turning");
    Check(Skid.State.RearWheelRate >= 0.0, "braking never drives a wheel backwards");
    Check(Locked.RearSlip < -0.5, "a locked wheel is fully sliding");

    // Grabbing only the front brake at speed sends the rider over the bars.
    bool bWentOver = false;
    StoppingDistance(1.0, 0.0, 25.0 / 3.6, 0.85, &bWentOver, nullptr);
    Check(bWentOver, "a full front-brake grab at 25 km/h pitches over the front wheel");

    // The pitch-over threshold is the textbook one: a = g * (CG to front axle) / CG height.
    Near(EndoDeceleration(Setup),
         Gravity * (Setup.Wheelbase - Setup.CentreOfMassToRearAxle) / Setup.CentreOfMassHeight,
         1e-12, "endo threshold is g * Lf / h");
    Check(EndoDeceleration(Setup) > 4.5 && EndoDeceleration(Setup) < 6.5,
          "endo threshold is about 0.55 g, as an upright bicycle should be");

    // Low grip lengthens the stop; VEH-01-002 section 12 requires this ordering.
    const double Dry = StoppingDistance(0.35, 0.6, From, 0.85);
    const double Wet = StoppingDistance(0.35, 0.6, From, 0.55);
    const double Gravelly = StoppingDistance(0.35, 0.6, From, 0.40);
    Check(Dry < Wet && Wet < Gravelly, "wet and gravel stops are progressively longer");
}

// ---------------------------------------------------------------------------
static void TestGradesAndHolding()
{
    // Climbing is slower than the flat, descending is faster, both bounded.
    FSim Flat, Climb, Descend;
    Climb.Surface.Grade = std::atan(0.06);
    Descend.Surface.Grade = -std::atan(0.06);
    Flat.Run(Pedalling(3), 300.0);
    Climb.Run(Pedalling(0), 300.0);
    Descend.Run(Pedalling(3), 300.0);
    Check(Climb.State.Speed < Flat.State.Speed, "a 6 percent climb is slower than the flat");
    Check(Descend.State.Speed > Flat.State.Speed, "a 6 percent descent is faster than the flat");
    Check(Descend.State.Speed < 25.0, "a descent reaches a terminal speed rather than running away");

    // Freewheeling downhill accelerates to terminal speed and holds there.
    FSim Roll;
    Roll.Surface.Grade = -std::atan(0.08);
    FRiderInput Coast;
    Coast.Gear = 3;
    Roll.Run(Coast, 200.0);
    const double Terminal = Roll.State.Speed;
    Roll.Run(Coast, 20.0);
    Near(Roll.State.Speed, Terminal, 0.05, "terminal speed on a grade is stable");
    Check(Terminal > 3.0, "an 8 percent grade does roll the bike downhill");

    // Brakes hold on a hill; rolling resistance alone does not.
    FSim Held;
    Held.Surface.Grade = std::atan(0.10);
    FRiderInput Brakes;
    Brakes.Gear = 3;
    Brakes.FrontBrake = Brakes.RearBrake = 1.0;
    Held.Run(Brakes, 10.0);
    Near(Held.State.Speed, 0.0, 1e-9, "brakes hold the bike on a 10 percent hill");
    FSim Released;
    Released.Surface.Grade = std::atan(0.10);
    Released.Run(Coast, 10.0);
    Check(Released.State.Speed < -0.5, "released on a hill, the bike rolls backwards");

    // Standing on the flat does not creep.
    FSim Parked;
    Parked.Run(Coast, 10.0);
    Near(Parked.State.Speed, 0.0, 1e-12, "a parked bike on the flat does not creep");
    Near(Parked.State.DistanceTravelled, 0.0, 1e-12, "a parked bike does not travel");
}

// ---------------------------------------------------------------------------
static void TestSteeringAndLean()
{
    const FSetup Setup;
    // Full lock at walking pace gives the specification sheet's turning circle.
    FSim Slow;
    Slow.State.Speed = 1.5;
    Slow.State.RearWheelRate = Slow.State.FrontWheelRate = 1.5 / Setup.WheelRadius;
    FRiderInput Turn;
    Turn.Gear = 0;
    Turn.Pedal = 0.25;
    Turn.Steer = 1.0;
    Slow.Run(Turn, 3.0);
    Near(Slow.Last.TurnRadius, 1.7, 0.35, "full lock turns inside the specified 1.7 m radius");

    // At speed the steering lock is grip-limited, not a hand-tuned curve.
    FSim Fast;
    Fast.State.Speed = 8.0;
    Fast.State.RearWheelRate = Fast.State.FrontWheelRate = 8.0 / Setup.WheelRadius;
    FRiderInput Sweep;
    Sweep.Gear = 4;
    Sweep.Pedal = 0.6;
    Sweep.Steer = 1.0;
    Fast.Run(Sweep, 4.0);
    Check(Fast.State.SteerAngle < Slow.State.SteerAngle,
          "steering lock shrinks with speed because grip, not the stem, limits it");
    Check(Fast.Last.TurnRadius > 6.0, "a fast corner has a wide radius");

    // Steady-state lean is the balance condition: tan(lean) = v * yawrate / g.
    Near(std::tan(Fast.State.Lean), Fast.State.Speed * Fast.State.YawRate / Gravity, 0.05,
         "steady-state lean balances the corner");
    Check(Fast.State.Lean > 0.15, "cornering at 29 km/h leans the bike over meaningfully");
    Check(std::fabs(Fast.State.Lean) <= std::atan(0.85 * Setup.GripSafety) + 0.02,
          "lean stays inside the friction cone");

    // Steering is limited by grip, so the rider cannot simply ask for a crash. Losing
    // the front happens when the surface changes mid-corner, which section 12 requires:
    // wet paint and metal covers under a leaned-over two-wheeler.
    FSim Slide;
    Slide.State.Speed = 8.0;
    Slide.State.RearWheelRate = Slide.State.FrontWheelRate = 8.0 / Setup.WheelRadius;
    Slide.Run(Sweep, 3.0);
    Check(!Slide.Last.bSliding, "a dry corner within the grip limit does not slide");
    const double Established = Slide.State.SteerAngle;
    const double LeanedOver = Slide.State.Lean;
    Slide.Surface.Friction = 0.15;   // rider crosses a wet steel cover mid-corner
    const FTelemetry Lost = Step(Slide.Setup, Slide.State, Sweep, Slide.Surface, Slide.Dt);
    Check(Lost.bSliding, "grip lost mid-corner cannot hold the lean up");
    Check(Lost.LateralForce > Lost.LateralGrip, "the road cannot supply the force the lean needs");
    Slide.Run(Sweep, 0.3);
    Check(Slide.State.SteerAngle < Established, "the rider straightens up as grip disappears");
    Check(Slide.State.Lean > LeanedOver, "without grip the lean keeps going over");
    Slide.Run(Sweep, 2.0);
    Check(Slide.State.bCrashed, "a sustained slide ends in a fall");

    // Standing still with the bars turned must not divide by zero or lean.
    FSim Still;
    FRiderInput Parked;
    Parked.Gear = 3;
    Parked.Steer = 1.0;
    Still.Run(Parked, 2.0);
    Check(Finite(Still.State), "stationary full-lock steering stays finite");
    Near(Still.State.Lean, 0.0, 1e-3, "a stationary bike sits upright with a foot down");
    Check(Still.Last.bFootDown, "the rider puts a foot down at a standstill");
    Near(Still.Last.TurnRadius, Setup.Wheelbase / std::tan(Setup.MaxSteerAngle), 0.05,
         "geometric turn radius is defined even at zero speed");
}

// ---------------------------------------------------------------------------
static void TestShifting()
{
    FSim Sim;
    Sim.State.Speed = 6.0;
    Sim.State.RearWheelRate = Sim.State.FrontWheelRate = 6.0 / Sim.Setup.WheelRadius;

    FRiderInput Low = Pedalling(1);
    Sim.Run(Low, 2.0);
    const double LowCadence = Sim.Last.Cadence;
    const double LowSpeed = Sim.State.Speed;

    // Shift up and watch the ratio walk across the freewheel rather than snap.
    FRiderInput High = Pedalling(5);
    double PreviousRatio = Sim.Last.GearRatio;
    bool bSawShift = false;
    double WorstJump = 0.0;
    for (int I = 0; I < 240; ++I)
    {
        Sim.Last = Step(Sim.Setup, Sim.State, High, Sim.Surface, Sim.Dt);
        WorstJump = std::max(WorstJump, std::fabs(Sim.Last.GearRatio - PreviousRatio));
        PreviousRatio = Sim.Last.GearRatio;
        if (Sim.Last.bShifting) bSawShift = true;
    }
    Check(bSawShift, "the derailleur takes time to move the chain");
    Check(WorstJump < 0.05, "the ratio changes continuously through a shift");
    Near(Sim.Last.GearRatio, double(Sim.Setup.ChainringTeeth) / Sim.Setup.SprocketTeeth[5], 1e-9,
         "the shift completes on the target sprocket");
    Near(Sim.State.ChainLateral, SprocketOffset(Sim.Setup, 5), 1e-9, "the chain ends up over the sprocket");

    // Same road speed, harder gear, lower cadence.
    Check(Sim.Last.Cadence < LowCadence, "a higher gear turns the cranks more slowly");
    Near(Sim.Last.Cadence,
         Sim.State.RearWheelRate / Sim.Last.GearRatio * 60.0 / (2.0 * Pi), 1e-9,
         "cadence follows the rear wheel through the engaged ratio");
    Check(Sim.State.RearWheelRate * Sim.Setup.WheelRadius > Sim.State.Speed,
          "a driven wheel runs slightly ahead of the road, which is the thrust slip");
    Check(LowSpeed > 0.0, "the bike kept moving through the shift");

    // Shifting interrupts the drive rather than passing full torque.
    FSim Interrupt;
    Interrupt.State.Speed = 6.0;
    Interrupt.State.RearWheelRate = 6.0 / Interrupt.Setup.WheelRadius;
    Interrupt.State.ChainLateral = SprocketOffset(Interrupt.Setup, 0);
    const FTelemetry Mid = Step(Interrupt.Setup, Interrupt.State, Pedalling(5), Interrupt.Surface, Interrupt.Dt);
    Check(Mid.bShifting, "the shift is in progress");
    FSim Settled;
    Settled.State.Speed = 6.0;
    Settled.State.RearWheelRate = 6.0 / Settled.Setup.WheelRadius;
    Settled.State.ChainLateral = SprocketOffset(Settled.Setup, 0);
    const FTelemetry Clean = Step(Settled.Setup, Settled.State, Pedalling(0), Settled.Surface, Settled.Dt);
    Check(Mid.RearDriveTorque < Clean.RearDriveTorque, "a shift in progress cuts drive torque");

    // The chain travelled a real distance in real links.
    Check(Sim.State.ChainTravel > 0.0, "chain travel accumulates while pedalling");
    Near(Sim.Last.Chain.Speed, Sim.State.CrankRate * Sim.Last.Chain.ChainringRadius, 1e-12,
         "chain speed is the chainring surface speed");
    Near(Sim.Last.Chain.Speed,
         Sim.State.RearWheelRate / Sim.Last.GearRatio * Sim.Last.Chain.ChainringRadius, 1e-9,
         "chain speed also matches the driven sprocket");
}

// ---------------------------------------------------------------------------
static void TestWheelKinematics()
{
    // Wheel rotation must account for the distance travelled: this is what makes the
    // rendered wheels and cranks line up with the road instead of sliding.
    FSim Sim;
    double Revolutions = 0.0;
    double Previous = Sim.State.RearWheelRate;
    (void)Previous;
    const FRiderInput In = Pedalling(3);
    for (int I = 0; I < 240 * 30; ++I)
    {
        Step(Sim.Setup, Sim.State, In, Sim.Surface, Sim.Dt);
        Revolutions += Sim.State.RearWheelRate * Sim.Dt / (2.0 * Pi);
    }
    const double RolledDistance = Revolutions * 2.0 * Pi * Sim.Setup.WheelRadius;
    // Within the slip the tyre actually needs to transmit thrust.
    Check(std::fabs(RolledDistance - Sim.State.DistanceTravelled) < Sim.State.DistanceTravelled * 0.03,
          "wheel rotation matches ground distance to within tyre slip");

    // Crank revolutions times development is the same distance again.
    FSim Steady;
    Steady.State.Speed = 7.0;
    Steady.State.RearWheelRate = Steady.State.FrontWheelRate = 7.0 / Steady.Setup.WheelRadius;
    const FTelemetry Out = Step(Steady.Setup, Steady.State, Pedalling(4), Steady.Surface, Steady.Dt);
    Near(Out.Development * Out.Cadence / 60.0, Steady.State.Speed, 0.05,
         "development times cadence is road speed");
}

// ---------------------------------------------------------------------------
static void TestIntegratorRobustness()
{
    // The tyre model is stiff near zero slip. If the implicit wheel solve were wrong
    // this diverges instead of agreeing across step sizes.
    double Speeds[3];
    const double Steps[3] = {1.0 / 60.0, 1.0 / 240.0, 1.0 / 1000.0};
    for (int I = 0; I < 3; ++I)
    {
        FSim Sim;
        Sim.Dt = Steps[I];
        Sim.Run(Pedalling(5), 300.0);
        Speeds[I] = Sim.State.Speed;
    }
    Check(std::fabs(Speeds[0] - Speeds[2]) < 0.4, "top speed is step-size independent");
    Check(std::fabs(Speeds[1] - Speeds[2]) < 0.2, "240 Hz agrees with 1 kHz");

    // A hard stop at 60 Hz must not ring or reverse the wheel.
    FSim Hard;
    Hard.Dt = 1.0 / 60.0;
    Hard.State.Speed = 9.0;
    Hard.State.RearWheelRate = Hard.State.FrontWheelRate = 9.0 / Hard.Setup.WheelRadius;
    FRiderInput Stop;
    Stop.Gear = 5;
    Stop.FrontBrake = 0.6;
    Stop.RearBrake = 1.0;
    double PreviousSpeed = Hard.State.Speed;
    for (int I = 0; I < 600 && Hard.State.Speed > 0.01; ++I)
    {
        Step(Hard.Setup, Hard.State, Stop, Hard.Surface, Hard.Dt);
        Check(Hard.State.Speed <= PreviousSpeed + 1e-6, "braking speed decreases monotonically at 60 Hz");
        Check(Hard.State.RearWheelRate >= -1e-9 && Hard.State.FrontWheelRate >= -1e-9,
              "wheels never spin backwards under braking");
        PreviousSpeed = Hard.State.Speed;
    }
    Check(Hard.State.Speed <= 0.01, "the 60 Hz stop completes");

    // Long random abuse: every state stays finite and bounded.
    FSim Chaos;
    unsigned Seed = 20260909u;
    auto Random = [&Seed]() {
        Seed = Seed * 1664525u + 1013904223u;
        return double(Seed >> 8) / double(1u << 24);
    };
    FRiderInput In;
    for (int I = 0; I < 400000; ++I)
    {
        if (I % 97 == 0)
        {
            In.Pedal = Random();
            In.FrontBrake = Random() > 0.7 ? Random() : 0.0;
            In.RearBrake = Random() > 0.7 ? Random() : 0.0;
            In.Steer = Random() * 2.0 - 1.0;
            In.Gear = int(Random() * 6.0) % 6;
            In.bSprint = Random() > 0.5;
            Chaos.Surface.Friction = 0.25 + Random() * 0.7;
            Chaos.Surface.Grade = (Random() - 0.5) * 0.25;
            Chaos.Surface.FrontLoadScale = Random() > 0.9 ? 0.0 : 1.0;
            Chaos.Surface.RearLoadScale = Random() > 0.9 ? 0.0 : 1.0;
        }
        Step(Chaos.Setup, Chaos.State, In, Chaos.Surface, 1.0 / 240.0 * (0.5 + Random()));
        if (!Finite(Chaos.State)) { Check(false, "random abuse kept the state finite"); break; }
        if (std::fabs(Chaos.State.Speed) > 60.0) { Check(false, "random abuse kept the speed bounded"); break; }
    }
    Check(std::fabs(Chaos.State.Lean) <= Pi, "lean stays bounded under abuse");
    Check(std::fabs(Chaos.State.Pitch) <= 1.2 + 1e-9, "pitch stays bounded under abuse");

    // Determinism: the same inputs produce bit-identical state.
    FSim A, B;
    A.Run(Pedalling(2), 20.0);
    B.Run(Pedalling(2), 20.0);
    Check(A.State.Speed == B.State.Speed && A.State.CrankAngle == B.State.CrankAngle
          && A.State.DistanceTravelled == B.State.DistanceTravelled, "the simulation is deterministic");
}

// ---------------------------------------------------------------------------
static void TestAirborneWheels()
{
    // Over a speed breaker a wheel unloads. It must not generate grip in mid-air.
    FSim Sim;
    Sim.State.Speed = 6.0;
    Sim.State.RearWheelRate = Sim.State.FrontWheelRate = 6.0 / Sim.Setup.WheelRadius;
    Sim.Surface.RearLoadScale = 0.0;
    const FTelemetry Out = Step(Sim.Setup, Sim.State, Pedalling(3), Sim.Surface, Sim.Dt);
    Near(Out.RearLoad, 0.0, 1e-12, "an airborne rear wheel carries no load");
    Near(Out.RearTyreForce, 0.0, 1e-12, "an airborne wheel generates no thrust");
    Check(Out.RearDriveTorque > 0.0, "the drivetrain still turns the airborne wheel");
    Sim.Run(Pedalling(3), 0.5);
    Check(Sim.State.RearWheelRate > 6.0 / Sim.Setup.WheelRadius,
          "an airborne driven wheel spins up freely");
    Check(Finite(Sim.State), "airborne wheels keep the state finite");
}

static void TestFixedChainAndFreewheel()
{
    FSetup Setup;
    int Count = 0;
    for (int Gear = 0; Gear < Setup.GearCount; ++Gear)
    {
        auto G = ChainGeometry(Setup, SprocketRadius(Setup.SprocketTeeth[Gear]), SprocketOffset(Setup, Gear));
        auto Loop = RenderChainLoop(Setup, G);
        if (Count == 0) Count = Loop.Links;
        Check(Loop.Links == Count && Count % 2 == 0, "shifting preserves an even, fixed link count");
        Near(Loop.Length, Count * ChainPitch, 1e-12, "closed loop has exact chain pitch length");
        double Measured = 0.0;
        auto Previous = RenderChainPoint(Setup, Loop, 0.0);
        for (int I = 1; I <= 20000; ++I)
        {
            auto Point = RenderChainPoint(Setup, Loop, Loop.Length * I / 20000.0);
            const double Segment = std::sqrt(std::pow(Point.X - Previous.X, 2) + std::pow(Point.Y - Previous.Y, 2)
                                           + std::pow(Point.Z - Previous.Z, 2));
            Check(Segment < 0.0001, "chain path is continuous through wraps and tensioner");
            Measured += Segment;
            Previous = Point;
        }
        Near(Measured, Loop.Length, 0.0002, "path arc length agrees with physical link spacing");
    }
    FSim Sim;
    Sim.Run(Pedalling(3), 8.0);
    FRiderInput Coast; Coast.Gear = 3;
    Sim.Run(Coast, 6.0);
    const double Cassette = Sim.State.CassetteAngle;
    const double Chain = Sim.State.ChainTravel;
    Sim.Run(Coast, 0.4);
    Near(Sim.State.CassetteAngle, Cassette, 1e-12, "freewheel carrier stops when rider stops pedalling");
    Near(Sim.State.ChainTravel, Chain, 1e-12, "chain stops while rear wheel coasts");
    Check(Sim.State.RearWheelRate > 1.0, "rear wheel still spins with stationary cassette");
    const double Lateral = Sim.State.ChainLateral;
    Coast.Gear = 0;
    Sim.Run(Coast, 1.0);
    Near(Sim.State.ChainLateral, Lateral, 1e-12, "stationary chain cannot shift sprockets");
    Sim.Run(Pedalling(0), 1.0);
    Near(Sim.State.ChainLateral, SprocketOffset(Setup, 0), 1e-12, "pedalling completes queued shift");

    FState State;
    State.Speed = 8.0;
    State.FrontWheelRate = State.RearWheelRate = 8.0 / Setup.WheelRadius;
    FSurface Split; Split.FrontFriction = 0.2; Split.RearFriction = 0.85;
    FRiderInput Brake; Brake.FrontBrake = Brake.RearBrake = 1.0;
    for (int I = 0; I < 120; ++I)
    {
        const auto T = Step(Setup, State, Brake, Split, 1.0 / 240.0);
        Check(std::fabs(T.FrontTyreForce) <= 0.2 * T.FrontLoad + 1e-8, "front tyre obeys its own surface grip");
        Check(std::fabs(T.RearTyreForce) <= 0.85 * T.RearLoad + 1e-8, "rear tyre obeys its own surface grip");
    }
}

int main()
{
    TestFixedChainAndFreewheel();
    TestChainAndGears();
    TestChainPath();
    TestDrivetrainAndFreewheel();
    TestTopSpeedAndAcceleration();
    TestCoastDown();
    TestBraking();
    TestGradesAndHolding();
    TestSteeringAndLean();
    TestShifting();
    TestWheelKinematics();
    TestIntegratorRobustness();
    TestAirborneWheels();
    if (Failures > 0)
    {
        std::printf("%d bicycle physics check(s) failed.\n", Failures);
        return 1;
    }
    std::printf("Bicycle physics checks passed.\n");
    return 0;
}
