#pragma once
#include <algorithm>
#include <cmath>
namespace NammaHuman
{
struct FVitals
{
    double Health=100, Stamina=100, Stagger=0;
    bool Alive() const { return Health>0; }
    bool Spend(double Cost)
    {
        if (!Alive() || !std::isfinite(Cost) || Cost<0 || Stamina<Cost) return false;
        Stamina-=Cost; return true;
    }
    double Damage(double Amount, bool FrontalBlock=false)
    {
        if (!Alive() || !std::isfinite(Amount) || Amount<=0) return 0;
        if (FrontalBlock && Spend(Amount*.8)) Amount*=.2;
        const double Applied=std::min(Health,Amount);Health-=Applied;Stagger+=Applied;return Applied;
    }
    void Tick(double Dt, bool Busy)
    {
        if (Alive() && !Busy && std::isfinite(Dt) && Dt>0)
            Stamina=std::min(100.0,Stamina+22.0*Dt);
        if (std::isfinite(Dt) && Dt>0) Stagger=std::max(0.0,Stagger-8.0*Dt);
    }
};
// Game damage tuned above ordinary jump/landing speeds (7 m/s safe threshold).
inline double ImpactDamage(double SpeedMetresPerSecond, double SafeSpeed = 7.0)
{
    if (!std::isfinite(SpeedMetresPerSecond)) return 0;
    const double Excess=std::max(0.0,std::abs(SpeedMetresPerSecond)-SafeSpeed);
    return std::min(100.0,Excess*Excess*3.0);
}
}
