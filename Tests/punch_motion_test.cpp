#include "../Source/NammaCity/NammaPunchMotion.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace NammaHuman;
static void Check(bool OK,const char* What) { if (!OK) { std::fprintf(stderr,"FAIL: %s\n",What);std::exit(1); } }
int main()
{
    Check(PunchMotion(0).Weight==0 && PunchMotion(PunchDuration).Weight==0,"pose enters and exits without a snap");
    Check(PunchMotion(.10).Extension==0 && PunchMotion(.10).Load==1,"wind-up precedes extension");
    Check(PunchMotion(.175).Extension==1,"strike reaches peak in 75 milliseconds");
    Check(PunchMotion(PunchImpactTime).Extension>.95,"damage occurs near full extension");
    Check(PunchMotion(.285).Extension<1e-9,"hand recoils promptly to guard");
    Check(PunchMotion(.08).Twist<0 && PunchMotion(.175).Twist>0,"torso loads then rotates into the strike");
    for (double Boundary : {.06,.10,.175,.285,.355,.38,.50})
    {
        auto A=PunchMotion(Boundary-1e-7),B=PunchMotion(Boundary+1e-7);
        Check(std::abs(A.Extension-B.Extension)<1e-4 && std::abs(A.Weight-B.Weight)<1e-4 && std::abs(A.Twist-B.Twist)<1e-4,"phase boundaries are continuous");
    }
    for (int Hz : {30,60,120,240})
    {
        double Time=0, Previous=0;int Hits=0;
        while (Time<PunchDuration) { Previous=Time;Time+=1.0/Hz;if (Previous<PunchImpactTime && Time>=PunchImpactTime)++Hits; }
        Check(Hits==1,"exactly one contact event at every tested frame rate");
    }
    std::puts("Punch timing, recoil and contact synchronization checks passed.");
}
