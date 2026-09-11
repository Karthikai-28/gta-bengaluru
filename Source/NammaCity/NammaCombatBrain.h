#pragma once
// Which strike to throw. One pure function shared by the player's attack key
// and the sparring partner, so both fight by the same rules.
//
// The rules follow the feel of GTA's melee rather than a fighting game: one
// attack button, and the situation decides the move. In punching range the
// button walks a jab, cross, jab combination that ends in a heavier finisher.
// A little further out, where only a leg reaches, the button kicks: a push
// kick to open, a roundhouse to follow. A guarding opponent is shoved off
// with a push kick, and a downed one takes a kick.
#include "NammaStrikeMotion.h"

namespace NammaHuman
{
struct FCombatSituation
{
    bool bHasTarget=false;
    double Distance=0;          // centimetres between capsule centres, horizontal
    bool bTargetDown=false;     // ragdolling or dead on the ground
    bool bTargetGuarding=false;
    int Combo=0;                // strikes already thrown in this chain
    double PunchReach=115;      // centimetres from own centre
    double KickReach=165;
    double Stamina=100;
};

inline double StrikeCost(EStrike Strike) { return IsKick(Strike) ? 22 : 12; }
inline double StrikeDamage(EStrike Strike)
{
    switch (Strike)
    {
    case EStrike::Jab: return 10;
    case EStrike::Cross: return 16;
    case EStrike::FrontKick: return 22;
    case EStrike::RoundhouseKick: return 28;
    default: return 0;
    }
}
// Stagger builds faster from the heavy strikes, so a kick or a finisher is
// what knocks someone down.
inline double StrikeStagger(EStrike Strike) { return StrikeDamage(Strike)*(IsKick(Strike) ? 1.4 : 1.0); }

// Seconds after a strike ends within which the next press continues the chain.
constexpr double ComboWindow=.6;

inline EStrike ChooseStrike(const FCombatSituation& S)
{
    if (!S.bHasTarget)
    {
        // Shadow boxing: cycle the combination, finishing with a kick.
        constexpr EStrike Shadow[]={EStrike::Jab,EStrike::Cross,EStrike::FrontKick,EStrike::RoundhouseKick};
        return Shadow[S.Combo%4];
    }
    if (S.bTargetDown) return EStrike::FrontKick;
    if (S.Distance>S.KickReach) return EStrike::Jab;   // a step-in jab closes the gap
    if (S.Distance>S.PunchReach)
        return (S.Combo==0 || S.bTargetGuarding) ? EStrike::FrontKick : EStrike::RoundhouseKick;
    if (S.bTargetGuarding && S.Combo>0) return EStrike::FrontKick;
    switch (S.Combo%4)
    {
    case 0: return EStrike::Jab;
    case 1: return EStrike::Cross;
    case 2: return EStrike::Jab;
    default: return S.Distance>.7*S.PunchReach ? EStrike::RoundhouseKick : EStrike::Cross;
    }
}

// The strike to actually throw given stamina: the chosen one if affordable,
// otherwise a jab, otherwise nothing.
inline bool PickStrike(const FCombatSituation& S,EStrike& Out)
{
    Out=ChooseStrike(S);
    if (S.Stamina>=StrikeCost(Out)) return true;
    Out=EStrike::Jab;
    return S.Stamina>=StrikeCost(Out);
}
}
