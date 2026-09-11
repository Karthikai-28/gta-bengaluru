#include "../Source/NammaCity/NammaStrikeMotion.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace NammaHuman;
static void Check(bool OK,const char* What) { if (!OK) { std::fprintf(stderr,"FAIL: %s\n",What);std::exit(1); } }
static const EStrike All[]={EStrike::Jab,EStrike::Cross,EStrike::FrontKick,EStrike::RoundhouseKick};
int main()
{
    for (EStrike Strike : All)
    {
        const auto& Clip=StrikeClip(Strike);
        const bool Kick=IsKick(Strike);
        Check(Clip.Duration>=(Kick ? .59 : .29) && Clip.Duration<=(Kick ? 1.01 : .61),"strike lasts as long as a real one");
        Check(Clip.ImpactFraction>.15 && Clip.ImpactFraction<.75,"impact lands inside the strike");
        Check(Clip.Reach>.7 && Clip.Reach<=1.0,"striking limb reaches most of its length");
        Check(StrikePose(Strike,0).Weight==0 && StrikePose(Strike,Clip.Duration).Weight==0,"pose enters and exits without a snap");
        Check(StrikePose(Strike,-.01).Weight==0 && StrikePose(Strike,Clip.Duration*2).Weight==0,"nothing outside the strike");
        const auto AtImpact=StrikePose(Strike,StrikeImpactTime(Strike));
        Check(AtImpact.Extension>.7,"damage occurs near full extension");
        Check(AtImpact.bKick==Kick,"pose knows whether it kicks");
        const auto Start=StrikePose(Strike,1e-4),End=StrikePose(Strike,Clip.Duration-1e-4);
        auto Near=[](const FStrikeVector& A,const FStrikeVector& B){ return std::abs(A.X-B.X)<.05 && std::abs(A.Y-B.Y)<.05 && std::abs(A.Z-B.Z)<.05; };
        Check(Near(Start.Fist,End.Fist) && Near(Start.Foot,End.Foot) && Near(Start.Pelvis,End.Pelvis),"strike returns to the guard it left");
        // Sampling continuity at every frame rate the game might run at. A
        // fist legitimately crosses a third of an arm in one 30 Hz frame, so
        // the smoothness bound applies from 60 Hz up; 30 Hz only counts hits.
        for (int Hz : {30,60,120,240})
        {
            double Time=0;int Hits=0;FStrikePose Previous=StrikePose(Strike,0);
            while (Time<Clip.Duration)
            {
                const double Next=Time+1.0/Hz;
                if (Time<StrikeImpactTime(Strike) && Next>=StrikeImpactTime(Strike)) ++Hits;
                const FStrikePose Pose=StrikePose(Strike,std::min(Next,Clip.Duration-1e-6));
                Check(Hz==30 || (std::abs(Pose.Fist.X-Previous.Fist.X)<.35 && std::abs(Pose.Foot.Z-Previous.Foot.Z)<.35),"no teleporting limbs between frames");
                Previous=Pose;Time=Next;
            }
            Check(Hits==1,"exactly one contact event at every tested frame rate");
        }
        // Mirroring reflects across the body and swaps the striking limb.
        const auto Plain=StrikePose(Strike,StrikeImpactTime(Strike)),Mirror=StrikePose(Strike,StrikeImpactTime(Strike),true);
        Check(Plain.StrikingSide!=Mirror.StrikingSide,"mirrored strike uses the other limb");
        Check(std::abs(Plain.Fist.Y+Mirror.Fist.Y)<1e-9 && std::abs(Plain.Fist.X-Mirror.Fist.X)<1e-9,"mirror reflects sideways only");
        Check(std::abs(Plain.PelvisYaw+Mirror.PelvisYaw)<1e-9,"mirror reverses the turn");
        if (!Kick)
        {
            // The complaint that started this: elbows drifting across the body.
            // A tucked elbow sits a little inboard of its shoulder, but the
            // midline is about a third of an arm length in, and the reference
            // boxers' elbows never cross it towards the other shoulder.
            for (int I=0;I<StrikeData::SampleCount;++I)
            {
                const auto& S=Clip.Samples[I];
                Check(S.Elbow[1]<.35,"striking elbow never crosses the midline");
                Check(S.GuardElbow[1]>-.35,"guard elbow never crosses the midline");
            }
            // Outward is -Y for the left arm and +Y for the right.
            const double Outward=StrikesWithLeft(Strike,false) ? -1.0 : 1.0;
            Check((AtImpact.Elbow.Y-AtImpact.Fist.Y)*Outward>=-.05,"elbow stays outside the line to the fist at impact");
            Check(AtImpact.Fist.X>.55,"punch lands in front");
        }
        else
        {
            Check(AtImpact.Foot.Z>-.75,"kicking foot rises well above the floor");
            Check(AtImpact.Foot.X>.3,"kick lands in front");
        }
    }
    Check(StrikesWithLeft(EStrike::Jab,false) && !StrikesWithLeft(EStrike::Cross,false) && !StrikesWithLeft(EStrike::Jab,true),"orthodox lead is the left");
    Check(StrikeReach(EStrike::RoundhouseKick)>.8 && IsKick(EStrike::FrontKick) && !IsKick(EStrike::Cross),"clip table is in strike order");
    Check(GuardPose().Weight==1 && GuardPose().Extension<1e-6,"guard is held at rest");
    std::puts("Strike template, timing, mirroring and elbow-side checks passed.");
}
