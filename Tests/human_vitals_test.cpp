#include "../Source/NammaCity/NammaVitals.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace NammaHuman;
static void Check(bool OK,const char* Message) { if (!OK) { std::fprintf(stderr,"FAIL: %s\n",Message);std::exit(1); } }
int main()
{
    FVitals V;
    Check(ImpactDamage(5.56,3)>19 && ImpactDamage(0,3)==0,"moving cycle crashes hurt; stationary dismount does not");
    Check(ImpactDamage(4.2)==0 && ImpactDamage(7)==0,"ordinary jump and soft fall are safe");
    Check(ImpactDamage(10)==27 && ImpactDamage(50)==100,"hard impact damage scales and caps");
    Check(V.Damage(18,true)>3.59 && V.Health>96,"frontal guard reduces damage");
    Check(V.Stamina<86,"guard spends stamina");
    V.Stamina=0;Check(V.Damage(18,true)==18,"exhausted guard cannot absorb hits");
    Check(!V.Spend(15),"exhaustion prevents punching");
    V.Tick(1,false);Check(V.Spend(15),"rest restores an attack");
    const double Before=V.Stamina;V.Tick(1,true);Check(V.Stamina==Before,"guarding cannot regenerate stamina");
    V.Tick(100,false);Check(V.Stamina==100,"stamina is capped");
    Check(V.Damage(std::numeric_limits<double>::quiet_NaN())==0,"invalid damage is rejected");
    Check(V.Damage(-10)==0 && !V.Spend(-10),"negative damage and costs cannot heal");
    V.Damage(1000);Check(!V.Alive() && V.Health==0,"lethal damage clamps health to zero");
    Check(!V.Spend(0) && V.Damage(10)==0,"dead character cannot act or take repeat damage");
    V=FVitals();V.Damage(18);V.Tick(.6,false);V.Damage(18);
    Check(V.Stagger>=30,"two quick unguarded blows stagger");
    V.Tick(5,false);Check(V.Stagger==0,"stagger decays between fights");
    std::puts("Human health, stamina, impact and guard checks passed.");
}
