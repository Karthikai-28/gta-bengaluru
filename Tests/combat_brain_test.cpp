#include "../Source/NammaCity/NammaCombatBrain.h"
#include <cstdio>
#include <cstdlib>
using namespace NammaHuman;
static void Check(bool OK,const char* What) { if (!OK) { std::fprintf(stderr,"FAIL: %s\n",What);std::exit(1); } }
int main()
{
    FCombatSituation S;
    Check(ChooseStrike(S)==EStrike::Jab,"shadow boxing opens with a jab");
    S.Combo=3;Check(ChooseStrike(S)==EStrike::RoundhouseKick,"shadow boxing finishes with a kick");
    S=FCombatSituation{};S.bHasTarget=true;S.Distance=90;
    const EStrike Chain[]={EStrike::Jab,EStrike::Cross,EStrike::Jab,EStrike::Cross};
    S.Distance=70;
    for (int I=0;I<4;++I) { S.Combo=I;Check(ChooseStrike(S)==Chain[I],"close-range combination is jab, cross, jab, cross"); }
    S.Combo=3;S.Distance=105;Check(ChooseStrike(S)==EStrike::RoundhouseKick,"finisher becomes a kick with room to swing");
    S.Combo=0;S.Distance=140;Check(ChooseStrike(S)==EStrike::FrontKick,"a push kick opens at kick range");
    S.Combo=1;Check(ChooseStrike(S)==EStrike::RoundhouseKick,"a roundhouse follows at kick range");
    S.Distance=200;Check(ChooseStrike(S)==EStrike::Jab,"out of reach, step in with a jab");
    S.Distance=90;S.Combo=1;S.bTargetGuarding=true;Check(ChooseStrike(S)==EStrike::FrontKick,"a push kick shoves a guard away");
    S.Combo=0;Check(ChooseStrike(S)==EStrike::Jab,"a guard is first tested with a jab");
    S.bTargetGuarding=false;S.bTargetDown=true;S.Distance=120;Check(ChooseStrike(S)==EStrike::FrontKick,"a downed opponent is kicked");
    EStrike Out;
    S=FCombatSituation{};S.bHasTarget=true;S.Distance=140;S.Stamina=15;
    Check(PickStrike(S,Out) && Out==EStrike::Jab,"too tired to kick falls back to a jab");
    S.Stamina=5;Check(!PickStrike(S,Out),"too tired to punch throws nothing");
    Check(StrikeCost(EStrike::FrontKick)>StrikeCost(EStrike::Jab) && StrikeDamage(EStrike::RoundhouseKick)>StrikeDamage(EStrike::Jab),"kicks cost and hurt more");
    Check(StrikeStagger(EStrike::RoundhouseKick)>StrikeDamage(EStrike::RoundhouseKick),"kicks stagger beyond their damage");
    std::puts("Combat decision checks passed.");
}
