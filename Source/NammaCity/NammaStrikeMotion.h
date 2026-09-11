#pragma once
// Strike poses sampled from the reference-footage templates in NammaStrikeData.h.
// Plain C++ so the host tests can exercise it without the engine.
#include "NammaStrikeData.h"
#include <algorithm>
#include <cmath>

namespace NammaHuman
{
// Hooks and uppercuts are absent on purpose: the reference footage yielded no
// demonstration of either that the extractor could tell from a coach holding
// an arm out or raising the guard, and a made-up template would defeat the
// point of measuring real boxers.
enum class EStrike : unsigned char { Jab, Cross, FrontKick, RoundhouseKick, Count };
constexpr int StrikeCount=int(EStrike::Count);
static_assert(StrikeData::ClipCount==StrikeCount,"the generated strike table must hold every strike");

inline const StrikeData::FClip& StrikeClip(EStrike Strike) { return *StrikeData::Clips[int(Strike)]; }
inline const char* StrikeName(EStrike Strike) { return StrikeClip(Strike).Name; }
inline bool IsKick(EStrike Strike) { return StrikeClip(Strike).bKick; }
inline double StrikeDuration(EStrike Strike) { return StrikeClip(Strike).Duration; }
inline double StrikeImpactTime(EStrike Strike) { return StrikeClip(Strike).Duration*StrikeClip(Strike).ImpactFraction; }
// Reach of the striking limb at impact as a fraction of that limb's length.
inline double StrikeReach(EStrike Strike) { return StrikeClip(Strike).Reach; }
// The templates are orthodox: the jab and the front kick lead with the left. A
// mirrored strike swaps sides, so a right jab is the left jab reflected.
inline bool StrikesWithLeft(EStrike Strike,bool bMirror) { return StrikeClip(Strike).bLead!=bMirror; }

// Stance frame: X forward, Y right, Z up, in lengths of the limb concerned.
struct FStrikeVector { double X=0,Y=0,Z=0; };

struct FStrikePose
{
    double Weight=0;            // blend envelope, 0 outside the strike
    double Extension=0;         // striking limb, 0 at guard .. 1 at full reach
    bool bKick=false;
    int StrikingSide=0;         // 0 left, 1 right, after mirroring
    // Fist/Elbow belong to the striking side's arm (for a kick, the arm on the
    // kicking side); GuardFist/GuardElbow to the other arm. Arm vectors are
    // relative to their shoulder in arm lengths; Foot/Knee relative to the hip
    // in leg lengths; Pelvis is the hip centre's shift in leg lengths.
    FStrikeVector Fist,Elbow,GuardFist,GuardElbow,Foot,Knee,Pelvis;
    double PelvisYaw=0,SpineTwist=0,LeanForward=0,LeanSide=0,HeadYaw=0; // degrees, positive yaw to the right
};

inline double StrikeSmooth(double T) { T=std::clamp(T,0.0,1.0);return T*T*(3-2*T); }

namespace Detail
{
inline double CatmullRom(double P0,double P1,double P2,double P3,double T)
{
    return 0.5*((2*P1)+(-P0+P2)*T+(2*P0-5*P1+4*P2-P3)*T*T+(-P0+3*P1-3*P2+P3)*T*T*T);
}
struct FSampler
{
    const StrikeData::FClip& Clip;int I0,I1,I2,I3;double T;
    explicit FSampler(const StrikeData::FClip& InClip,double Fraction) : Clip(InClip)
    {
        const double Position=std::clamp(Fraction,0.0,1.0)*(StrikeData::SampleCount-1);
        I1=std::min(int(Position),StrikeData::SampleCount-2);
        T=Position-I1;
        I0=std::max(I1-1,0);I2=I1+1;I3=std::min(I2+1,StrikeData::SampleCount-1);
    }
    template<typename Get> double Scalar(Get&& Field) const
    {
        return CatmullRom(Field(Clip.Samples[I0]),Field(Clip.Samples[I1]),Field(Clip.Samples[I2]),Field(Clip.Samples[I3]),T);
    }
    template<typename Get> FStrikeVector Vector(Get&& Field,double MirrorSign) const
    {
        FStrikeVector V;
        V.X=Scalar([&](const StrikeData::FSample& S){ return double(Field(S)[0]); });
        V.Y=MirrorSign*Scalar([&](const StrikeData::FSample& S){ return double(Field(S)[1]); });
        V.Z=Scalar([&](const StrikeData::FSample& S){ return double(Field(S)[2]); });
        return V;
    }
};
inline double Length(const FStrikeVector& V) { return std::sqrt(V.X*V.X+V.Y*V.Y+V.Z*V.Z); }
}

// The pose at Elapsed seconds into a strike. Outside [0, Duration) it is empty.
inline FStrikePose StrikePose(EStrike Strike,double Elapsed,bool bMirror=false)
{
    const StrikeData::FClip& Clip=StrikeClip(Strike);
    if (!(Elapsed>=0) || Elapsed>=Clip.Duration) return {};
    const double Sign=bMirror ? -1.0 : 1.0;
    const Detail::FSampler S(Clip,Elapsed/Clip.Duration);
    FStrikePose Pose;
    Pose.bKick=Clip.bKick;
    Pose.StrikingSide=StrikesWithLeft(Strike,bMirror) ? 0 : 1;
    Pose.Weight=StrikeSmooth(Elapsed/.06)*(1-StrikeSmooth((Elapsed-(Clip.Duration-.12))/.12));
    Pose.Fist=S.Vector([](const StrikeData::FSample& X){ return X.Fist; },Sign);
    Pose.Elbow=S.Vector([](const StrikeData::FSample& X){ return X.Elbow; },Sign);
    Pose.GuardFist=S.Vector([](const StrikeData::FSample& X){ return X.GuardFist; },Sign);
    Pose.GuardElbow=S.Vector([](const StrikeData::FSample& X){ return X.GuardElbow; },Sign);
    Pose.Foot=S.Vector([](const StrikeData::FSample& X){ return X.Foot; },Sign);
    Pose.Knee=S.Vector([](const StrikeData::FSample& X){ return X.Knee; },Sign);
    Pose.Pelvis=S.Vector([](const StrikeData::FSample& X){ return X.Pelvis; },Sign);
    Pose.PelvisYaw=Sign*S.Scalar([](const StrikeData::FSample& X){ return double(X.PelvisYaw); });
    Pose.SpineTwist=Sign*S.Scalar([](const StrikeData::FSample& X){ return double(X.SpineTwist); });
    Pose.LeanForward=S.Scalar([](const StrikeData::FSample& X){ return double(X.LeanForward); });
    Pose.LeanSide=Sign*S.Scalar([](const StrikeData::FSample& X){ return double(X.LeanSide); });
    Pose.HeadYaw=Sign*S.Scalar([](const StrikeData::FSample& X){ return double(X.HeadYaw); });
    const FStrikeVector& Limb=Clip.bKick ? Pose.Foot : Pose.Fist;
    const float* Rest=Clip.bKick ? Clip.Samples[0].Foot : Clip.Samples[0].Fist;
    const double RestLength=std::sqrt(double(Rest[0]*Rest[0]+Rest[1]*Rest[1]+Rest[2]*Rest[2]));
    Pose.Extension=std::clamp((Detail::Length(Limb)-RestLength)/std::max(Clip.Reach-RestLength,1e-3),0.0,1.0);
    return Pose;
}

// The guard held between strikes and while blocking: the start of the jab.
inline FStrikePose GuardPose(bool bMirror=false)
{
    FStrikePose Pose=StrikePose(EStrike::Jab,0.0,bMirror);
    Pose.Weight=1;
    return Pose;
}
}
