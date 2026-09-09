#pragma once

namespace NammaBicycle
{
struct FWheelSupport
{
    double Height;
    double Velocity;
    bool Grounded;
};

// SI units. Unilateral, zero-restitution wheel contact for the rigid roadster.
// Position correction is never converted into velocity: even deep initial overlap
// cannot store spring energy or launch the bike. Without contact, gravity remains
// ballistic. A continuous slope can supply its actual tangent vertical velocity.
inline FWheelSupport ResolveWheelSupport(double Height, double Velocity,
    bool HasGround, double GroundHeight, double Dt, double SlopeVelocity = 0.0)
{
    Velocity -= 9.81 * Dt;
    const double PredictedHeight = Height + Velocity * Dt;
    if (HasGround && PredictedHeight <= GroundHeight)
        return {GroundHeight, SlopeVelocity, true};
    return {PredictedHeight, Velocity, false};
}
}
