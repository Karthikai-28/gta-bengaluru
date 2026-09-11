#include "NammaPlayerCharacter.h"
#include "NammaBicycle.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

using namespace NammaHuman;

namespace
{
const TCHAR* LimbRoot(bool bKick,int Side) { return bKick ? (Side==0 ? TEXT("thigh_l") : TEXT("thigh_r")) : (Side==0 ? TEXT("upperarm_l") : TEXT("upperarm_r")); }
const TCHAR* LimbMiddle(bool bKick,int Side) { return bKick ? (Side==0 ? TEXT("calf_l") : TEXT("calf_r")) : (Side==0 ? TEXT("lowerarm_l") : TEXT("lowerarm_r")); }
const TCHAR* LimbEnd(bool bKick,int Side) { return bKick ? (Side==0 ? TEXT("foot_l") : TEXT("foot_r")) : (Side==0 ? TEXT("hand_l") : TEXT("hand_r")); }
}

bool ANammaPlayerCharacter::CanFight() const
{
    return Vitals.Alive() && !bRagdoll && RecoveryTime<=0 && !bTraversing && !IsRiding()
        && !IsPaused() && GetCharacterMovement()->IsMovingOnGround();
}

// The nearest other character roughly in front, near enough that a strike
// decision makes sense. Downed characters count: they can still be kicked.
ANammaPlayerCharacter* ANammaPlayerCharacter::FindCombatTarget() const
{
    if (bSparringPartner) return CombatTarget.Get();
    ANammaPlayerCharacter* Best=nullptr;float BestDistance=260.f;
    for (TActorIterator<ANammaPlayerCharacter> It(GetWorld());It;++It)
    {
        if (*It==this || It->IsRiding()) continue;
        const FVector Delta=It->GetActorLocation()-GetActorLocation();
        const float Distance=float(Delta.Size2D());
        if (Distance>=BestDistance || FVector::DotProduct(GetActorForwardVector(),Delta.GetSafeNormal2D())<.34f) continue;
        Best=*It;BestDistance=Distance;
    }
    return Best;
}

FCombatSituation ANammaPlayerCharacter::ReadSituation(const ANammaPlayerCharacter* Target) const
{
    FCombatSituation S;
    S.Combo=ComboTime>0 ? Combo : 0;
    S.Stamina=Vitals.Stamina;
    // Reach from the capsule centre: shoulder or hip offset plus the limb,
    // measured on this skeleton so a differently sized body still connects.
    const USkeletalMeshComponent* Mesh=GetMesh();
    const float Arm=float(FVector::Dist(Mesh->GetSocketLocation(TEXT("upperarm_l")),Mesh->GetSocketLocation(TEXT("lowerarm_l")))
        +FVector::Dist(Mesh->GetSocketLocation(TEXT("lowerarm_l")),Mesh->GetSocketLocation(TEXT("hand_l"))));
    const float Leg=float(FVector::Dist(Mesh->GetSocketLocation(TEXT("thigh_l")),Mesh->GetSocketLocation(TEXT("calf_l")))
        +FVector::Dist(Mesh->GetSocketLocation(TEXT("calf_l")),Mesh->GetSocketLocation(TEXT("foot_l"))));
    // Centre to centre: the limb's root sits a little ahead of our centre, the
    // sweep runs a fist beyond the landing point, and the target's own capsule
    // radius counts towards it.
    const float TargetRadius=GetCapsuleComponent()->GetScaledCapsuleRadius();
    S.PunchReach=15.f+Arm*float(StrikeReach(EStrike::Jab))+TargetRadius+15.f;
    S.KickReach=15.f+Leg*float(StrikeReach(EStrike::FrontKick))+TargetRadius+15.f;
    if (Target)
    {
        S.bHasTarget=true;
        S.Distance=FVector::Dist2D(GetActorLocation(),Target->GetActorLocation());
        S.bTargetDown=Target->IsDown() || Target->IsDead();
        S.bTargetGuarding=Target->IsGuarding();
    }
    return S;
}

void ANammaPlayerCharacter::Attack()
{
    if (!CanFight() || AttackTime>0) return;
    ANammaPlayerCharacter* Target=FindCombatTarget();
    EStrike Strike;
    if (!PickStrike(ReadSituation(Target),Strike)) return;
    BeginStrike(Strike,Target);
}

bool ANammaPlayerCharacter::BeginStrike(EStrike Strike,ANammaPlayerCharacter* Target)
{
    if (!CanFight() || AttackTime>0 || !Vitals.Spend(StrikeCost(Strike))) return false;
    ReleaseObject(); SprintEnd(); bBlocking=false;
    if (ComboTime<=0) Combo=0;
    CurrentStrike=Strike; bStrikeSpent=false;
    AttackTime=float(StrikeDuration(Strike));
    ComboTime=AttackTime+float(ComboWindow);
    ++Combo;
    // Square up to the target, as a lock-on would; otherwise strike where the camera looks.
    if (Target) SetActorRotation(FRotator(0,(Target->GetActorLocation()-GetActorLocation()).Rotation().Yaw,0));
    else if (Controller) SetActorRotation(FRotator(0,GetControlRotation().Yaw,0));
    return true;
}

void ANammaPlayerCharacter::BlockStart() { if (CanFight() && AttackTime<=0) { bBlocking=true; SprintEnd(); } }
void ANammaPlayerCharacter::BlockEnd() { bBlocking=false; }

void ANammaPlayerCharacter::Strike()
{
    // Sweep the striking limb out to where the reference footage lands it.
    // Contact timing shares the template; walls still stop the nearest hit.
    const FStrikePose Pose=StrikePose(CurrentStrike,StrikeImpactTime(CurrentStrike),bMirrorStance);
    const int Side=Pose.StrikingSide;
    const USkeletalMeshComponent* Mesh=GetMesh();
    const FVector Root=Mesh->GetSocketLocation(LimbRoot(Pose.bKick,Side));
    const float Length=float(FVector::Dist(Root,Mesh->GetSocketLocation(LimbMiddle(Pose.bKick,Side)))
        +FVector::Dist(Mesh->GetSocketLocation(LimbMiddle(Pose.bKick,Side)),Mesh->GetSocketLocation(LimbEnd(Pose.bKick,Side))));
    const FStrikeVector& Limb=Pose.bKick ? Pose.Foot : Pose.Fist;
    const FVector Land=Root+(GetActorForwardVector()*float(Limb.X)+GetActorRightVector()*float(Limb.Y)+FVector::UpVector*float(Limb.Z))*Length;
    const FVector Start=Root+GetActorForwardVector()*20.f;
    const FVector End=Land+(Land-Start).GetSafeNormal()*10.f;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NammaStrike),false,this);
    if (GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Pawn,
        FCollisionShape::MakeSphere(Pose.bKick ? 14.f : 10.f),Query))
    {
        const float Damage=float(StrikeDamage(CurrentStrike));
        UGameplayStatics::ApplyDamage(Hit.GetActor(),Damage,Controller,this,UDamageType::StaticClass());
        if (auto* Other=Cast<ANammaPlayerCharacter>(Hit.GetActor()))
            Other->AddStagger(float(StrikeStagger(CurrentStrike))-Damage);
        if (auto* Part=Hit.GetComponent(); Part && Part->IsSimulatingPhysics())
            Part->AddImpulseAtLocation(GetActorForwardVector()*(Pose.bKick ? 7500.f : 4500.f),Hit.ImpactPoint);
    }
}

void ANammaPlayerCharacter::AddStagger(float Amount)
{
    if (IsPaused() || !Vitals.Alive() || bRagdoll || RecoveryTime>0 || Amount<=0) return;
    Vitals.Stagger+=Amount;
    if (Vitals.Stagger>=30.f)
    {
        Vitals.Stagger=0;
        if (ANammaBicycle* Bike=Riding.Get()) Bike->Dismount(true);
        EnterRagdoll(-GetActorForwardVector()*250.f);
    }
}

float ANammaPlayerCharacter::TakeDamage(float Amount,const FDamageEvent& Event,AController* DamageInstigator,AActor* Causer)
{
    if (IsPaused() || !Vitals.Alive() || bRagdoll || RecoveryTime>0) return 0.f;
    const FVector Toward=Causer ? (Causer->GetActorLocation()-GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
    const bool Guard=bBlocking && FVector::DotProduct(GetActorForwardVector(),Toward)>.4f;
    const float Applied=float(Vitals.Damage(Amount,Guard));
    if (Applied<=0) return 0;
    Super::TakeDamage(Applied,Event,DamageInstigator,Causer);
    if (bSparringPartner) CombatTarget=Cast<ANammaPlayerCharacter>(Causer);
    if (!Vitals.Alive() || Vitals.Stagger>=30.f)
    {
        Vitals.Stagger=0;
        if (ANammaBicycle* Bike=Riding.Get()) Bike->Dismount(true);
        EnterRagdoll(-Toward*250.f);
    }
    else if (!Guard) LaunchCharacter(-Toward*FMath::Clamp(Applied*7.f,60.f,220.f),false,false);
    return Applied;
}

void ANammaPlayerCharacter::Landed(const FHitResult& Hit)
{
    const FVector ImpactVelocity=GetCharacterMovement()->Velocity;
    const float Damage=float(NammaHuman::ImpactDamage(ImpactVelocity.Z/100.0));
    Super::Landed(Hit);
    if (Damage>0)
    {
        Vitals.Damage(Damage);
        if (Damage>=20.f || !Vitals.Alive()) EnterRagdoll(ImpactVelocity*.15f);
    }
}

bool ANammaPlayerCharacter::TryGetUp()
{
    if (!bRagdoll || !Vitals.Alive() || IsPaused() || DownTime<1.f || SettledTime<.35f) return false;
    const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
    const float StandingHalfHeight=GetClass()->GetDefaultObject<ANammaPlayerCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    const float ScaledHalfHeight=StandingHalfHeight*GetActorScale3D().Z;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NammaGetUp),false,this);
    for (const FVector Offset : {FVector::ZeroVector,FVector(55,0,0),FVector(-55,0,0),FVector(0,55,0),FVector(0,-55,0)})
    {
        FHitResult Floor,Barrier;
        const FVector Point=Pelvis+Offset;
        if (!GetWorld()->LineTraceSingleByChannel(Floor,Point+FVector(0,0,40),Point-FVector(0,0,180),ECC_Visibility,Query)
            || !GetCharacterMovement()->IsWalkable(Floor)) continue;
        const FVector Stand=Floor.ImpactPoint+FVector(0,0,ScaledHalfHeight+3.f);
        if (GetWorld()->OverlapBlockingTestByChannel(Stand,FQuat::Identity,ECC_Pawn,
            FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius()-2.f,ScaledHalfHeight-2.f),Query) || GetWorld()->LineTraceSingleByChannel(Barrier,Pelvis+FVector(0,0,20),
            Floor.ImpactPoint+FVector(0,0,35),ECC_Visibility,Query)) continue;
        GetMesh()->SetSimulatePhysics(false);
        SetActorLocation(Stand,false,nullptr,ETeleportType::TeleportPhysics);
        SetActorRotation(FRotator(0,GetActorRotation().Yaw,0));
        GetMesh()->AttachToComponent(GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);
        GetMesh()->SetRelativeTransform(StandingMeshTransform);
        GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
        GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
        GetCapsuleComponent()->SetCapsuleHalfHeight(StandingHalfHeight);
        bIsCrouched=false; GetCharacterMovement()->bWantsToCrouch=false; RecalculateBaseEyeHeight();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Boom->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
        Boom->SetRelativeLocation(FVector::ZeroVector);
        bRagdoll=false; RecoveryTime=.8f;
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        return true;
    }
    return false;
}

void ANammaPlayerCharacter::SetSparringPartner()
{
    bSparringPartner=true; CombatHome=GetActorLocation();
    // A southpaw partner, so the player faces a mirrored stance and the
    // mirrored templates get exercised.
    bMirrorStance=true;
    GetCharacterMovement()->bRunPhysicsWithNoController=true;
    GetCharacterMovement()->MaxWalkSpeed=230.f;
}

void ANammaPlayerCharacter::TickCombat(float Dt)
{
    if (IsPaused()) return;
    Vitals.Tick(Dt,bBlocking || AttackTime>0 || bRagdoll || RecoveryTime>0);
    RecoveryTime=FMath::Max(0.f,RecoveryTime-Dt);
    ThinkTime=FMath::Max(0.f,ThinkTime-Dt);
    if (bRagdoll)
    {
        DownTime+=Dt;
        const float Speed=GetMesh()->GetPhysicsLinearVelocity(TEXT("pelvis")).Size();
        SettledTime=Speed<100.f && GetMesh()->GetPhysicsAngularVelocityInDegrees(TEXT("pelvis")).Size()<90.f
            ? SettledTime+Dt : 0.f;
        if (DownTime>1.5f) TryGetUp();
        return;
    }
    if (AttackTime>0)
    {
        AttackTime=FMath::Max(0.f,AttackTime-Dt);
        if (AttackTime<=StrikeDuration(CurrentStrike)-StrikeImpactTime(CurrentStrike) && !bStrikeSpent) { bStrikeSpent=true; Strike(); }
        if (AttackTime<=0) ThinkTime=.3f;
    }
    if (ComboTime>0)
    {
        ComboTime=FMath::Max(0.f,ComboTime-Dt);
        if (ComboTime<=0) Combo=0;
    }
    if (!bSparringPartner || !CanFight() || AttackTime>0 || ThinkTime>0) return;
    auto* Target=CombatTarget.Get();
    if (!Target || Target->IsDead() || Target->IsDown() || Target->IsRiding()
        || FVector::Dist2D(GetActorLocation(),CombatHome)>900.f) { CombatTarget.Reset(); return; }
    const FVector Delta=Target->GetActorLocation()-GetActorLocation();
    if (Delta.Size2D()>1000.f) { CombatTarget.Reset(); return; }
    SetActorRotation(FRotator(0,Delta.Rotation().Yaw,0));
    // Fight by the same rules as the player: punch in close, kick from range
    // now and then, otherwise close the distance.
    const FCombatSituation S=ReadSituation(Target);
    const bool bKickRange=S.Distance<=S.KickReach && S.Distance>S.PunchReach && (Combo%3)==0 && ComboTime>0;
    if (S.Distance<=S.PunchReach || bKickRange) Attack();
    else AddMovementInput(Delta.GetSafeNormal2D(),1.f,true);
}

bool ANammaPlayerCharacter::GetCombatPose(FStrikePose& Out) const
{
    if (!CanFight() || (!bBlocking && AttackTime<=0 && ComboTime<=0)) return false;
    if (AttackTime>0) Out=StrikePose(CurrentStrike,StrikeDuration(CurrentStrike)-AttackTime,bMirrorStance);
    else Out=GuardPose(bMirrorStance);
    return true;
}
