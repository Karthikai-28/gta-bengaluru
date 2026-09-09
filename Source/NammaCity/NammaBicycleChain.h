#pragma once
#include "NammaBicyclePhysics.h"

namespace NammaBicycle
{
// Rendering uses a fixed, even number of half-inch links. An ideal lower-run
// tensioner takes up the changing wrap length as the derailleur shifts. The loaded
// upper run remains exactly tangent to both pitch circles, including chain-line skew.
struct FChainLoop
{
    FChain Geometry;
    double UpperLength = 0.0;
    double LowerLength = 0.0;
    double Length = 0.0;
    double TensionerDrop = 0.0;
    int Links = 0;
};
inline FChainLoop RenderChainLoop(const FSetup& Setup, const FChain& Chain)
{
    FChainLoop Loop;
    Loop.Geometry = Chain;
    double Longest = 0.0;
    for (int Gear = 0; Gear < Setup.GearCount; ++Gear)
    {
        const FChain G = ChainGeometry(Setup, SprocketRadius(Setup.SprocketTeeth[Gear]), SprocketOffset(Setup, Gear));
        Longest = std::max(Longest, G.TotalLength + 2.0 * (std::hypot(G.SpanLength, G.LateralOffset) - G.SpanLength));
    }
    Loop.Links = int(std::ceil((Longest + 0.03) / (2.0 * ChainPitch))) * 2;
    Loop.Length = Loop.Links * ChainPitch;
    Loop.UpperLength = std::hypot(Chain.SpanLength, Chain.LateralOffset);
    Loop.LowerLength = Loop.Length - Loop.UpperLength - Chain.SprocketRadius * Chain.WrapSprocket
                       - Chain.ChainringRadius * Chain.WrapChainring;
    Loop.TensionerDrop = 0.5 * std::sqrt(std::max(0.0,
        Loop.LowerLength * Loop.LowerLength - Loop.UpperLength * Loop.UpperLength));
    return Loop;
}
inline FChainPoint RenderChainPoint(const FSetup& Setup, const FChainLoop& Loop, double Distance)
{
    const auto& G = Loop.Geometry;
    double D = std::fmod(Distance, Loop.Length);
    if (D < 0.0) D += Loop.Length;
    if (D < Loop.UpperLength)
        return ChainPathPoint(Setup, G, D * G.SpanLength / Loop.UpperLength);
    D -= Loop.UpperLength;
    const double RearArc = G.SprocketRadius * G.WrapSprocket;
    if (D < RearArc) return ChainPathPoint(Setup, G, G.SpanLength + D);
    D -= RearArc;
    if (D < Loop.LowerLength)
    {
        const FChainPoint A = ChainPathPoint(Setup, G, G.SpanLength + RearArc);
        const FChainPoint B = ChainPathPoint(Setup, G, 2.0 * G.SpanLength + RearArc);
        const double Along = (G.ChainringRadius - G.SprocketRadius) / Setup.ChainstayLength;
        const double Across = std::sqrt(std::max(0.0, 1.0 - Along * Along));
        // Perpendicular to the lower span (including its height difference), so the
        // two equal segments sum to LowerLength without stretching the chain.
        const FChainPoint Mid{(A.X + B.X) * 0.5 - Along * Loop.TensionerDrop,
                              (A.Y + B.Y) * 0.5,
                              (A.Z + B.Z) * 0.5 - Across * Loop.TensionerDrop};
        const double T = D / Loop.LowerLength * 2.0;
        const FChainPoint& From = T < 1.0 ? A : Mid;
        const FChainPoint& To = T < 1.0 ? Mid : B;
        const double Alpha = T < 1.0 ? T : T - 1.0;
        return {From.X + (To.X - From.X) * Alpha, From.Y + (To.Y - From.Y) * Alpha,
                From.Z + (To.Z - From.Z) * Alpha};
    }
    return ChainPathPoint(Setup, G, 2.0 * G.SpanLength + RearArc + D - Loop.LowerLength);
}
}
