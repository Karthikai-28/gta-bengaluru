#pragma once
#include <algorithm>
namespace NammaHuman
{
constexpr double PunchDuration=.50;
constexpr double PunchImpactTime=.165;
struct FPunchMotion { double Extension=0, Load=0, Twist=0, Weight=0; };
inline double Smooth(double T) { T=std::clamp(T,0.0,1.0);return T*T*(3-2*T); }
inline FPunchMotion PunchMotion(double Elapsed)
{
    if (Elapsed<0 || Elapsed>=PunchDuration) return {};
    FPunchMotion P;
    P.Weight=Smooth(Elapsed/.06)*(1-Smooth((Elapsed-.38)/.12));
    if (Elapsed<.10) { P.Load=Smooth(Elapsed/.10);P.Twist=-.25*P.Load; }
    else if (Elapsed<.175)
    {
        P.Extension=Smooth((Elapsed-.10)/.075);
        P.Load=1-P.Extension;P.Twist=-.25+1.25*P.Extension;
    }
    else
    {
        P.Extension=1-Smooth((Elapsed-.175)/.11);
        P.Twist=1-Smooth((Elapsed-.175)/.18);
    }
    return P;
}
}
