#include "NammaPlayerCharacter.h"
#include "NammaBicycle.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

bool ANammaPlayerCharacter::CanFight() const
{
    return Vitals.Alive() && !bRagdoll && RecoveryTime<=0 && !bTraversing && !IsRiding()
        && !IsPaused() && GetCharacterMovement()->IsMovingOnGround();
}

void ANammaPlayerCharacter::Punch()
{
    if (!CanFight() || AttackTime>0 || !Vitals.Spend(15)) return;
    ReleaseObject(); SprintEnd(); bBlocking=false;
    bLeftPunch=!bLeftPunch; bStrikeSpent=false; AttackTime=float(NammaHuman::PunchDuration);
    if (Controller) SetActorRotation(FRotator(0,GetControlRotation().Yaw,0));
}
void ANammaPlayerCharacter::BlockStart() { if (CanFight() && AttackTime<=0) { bBlocking=true; SprintEnd(); } }
void ANammaPlayerCharacter::BlockEnd() { bBlocking=false; }

void ANammaPlayerCharacter::Strike()
{
    // Sweep the striking fist's extension rather than a chest-wide push volume.
    // Contact timing shares the animation curve; walls still stop the nearest hit.
    FVector Left,Right;
    if (!GetCombatHands(Left,Right)) return;
    const FVector Shoulder=GetMesh()->GetSocketLocation(bLeftPunch ? TEXT("upperarm_l") : TEXT("upperarm_r"));
    const FVector Elbow=GetMesh()->GetSocketLocation(bLeftPunch ? TEXT("lowerarm_l") : TEXT("lowerarm_r"));
    const FVector Wrist=GetMesh()->GetSocketLocation(bLeftPunch ? TEXT("hand_l") : TEXT("hand_r"));
    const float Reach=.98f*(FVector::Dist(Shoulder,Elbow)+FVector::Dist(Elbow,Wrist));
    const float Sign=bLeftPunch ? -1.f : 1.f;
    const FVector Start=Shoulder+GetActorForwardVector()*24.f-GetActorRightVector()*Sign*8.f+FVector(0,0,8);
    const FVector End=Shoulder+(GetActorForwardVector()-GetActorRightVector()*Sign*.10f
        -FVector(0,0,.045f)).GetSafeNormal()*Reach+GetActorForwardVector()*8.f;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NammaPunch),false,this);
    if (GetWorld()->SweepSingleByChannel(Hit,Start,End,
        FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(10.f),Query))
    {
        UGameplayStatics::ApplyDamage(Hit.GetActor(),18.f,Controller,this,UDamageType::StaticClass());
        if (auto* Part=Hit.GetComponent(); Part && Part->IsSimulatingPhysics())
            Part->AddImpulseAtLocation(GetActorForwardVector()*4500.f,Hit.ImpactPoint);
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
    else if (!Guard) LaunchCharacter(-Toward*100.f,false,false);
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
    GetCharacterMovement()->bRunPhysicsWithNoController=true;
    GetCharacterMovement()->MaxWalkSpeed=230.f;
}

void ANammaPlayerCharacter::TickCombat(float Dt)
{
    if (IsPaused()) return;
    Vitals.Tick(Dt,bBlocking || AttackTime>0 || bRagdoll || RecoveryTime>0);
    RecoveryTime=FMath::Max(0.f,RecoveryTime-Dt);
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
        if (AttackTime<=NammaHuman::PunchDuration-NammaHuman::PunchImpactTime && !bStrikeSpent) { bStrikeSpent=true; Strike(); }
    }
    if (!bSparringPartner || !CanFight()) return;
    auto* Target=CombatTarget.Get();
    if (!Target || Target->IsDead() || Target->IsDown() || Target->IsRiding()
        || FVector::Dist2D(GetActorLocation(),CombatHome)>900.f) { CombatTarget.Reset(); return; }
    const FVector Delta=Target->GetActorLocation()-GetActorLocation();
    if (Delta.Size2D()>1000.f) { CombatTarget.Reset(); return; }
    SetActorRotation(FRotator(0,Delta.Rotation().Yaw,0));
    if (Delta.Size2D()>100.f) AddMovementInput(Delta.GetSafeNormal2D(),1.f,true);
    else Punch();
}

NammaHuman::FPunchMotion ANammaPlayerCharacter::GetPunchMotion() const
{
    if (AttackTime>0) return NammaHuman::PunchMotion(NammaHuman::PunchDuration-AttackTime);
    NammaHuman::FPunchMotion Guard;
    Guard.Weight=bBlocking ? 1.0 : 0.0;
    return Guard;
}

bool ANammaPlayerCharacter::GetCombatHands(FVector& Left,FVector& Right) const
{
    if (!CanFight() || (!bBlocking && AttackTime<=0)) return false;
    const auto Motion=GetPunchMotion();
    for (int Side=0;Side<2;++Side)
    {
        const float Sign=Side==0 ? -1.f : 1.f;
        const FVector Shoulder=GetMesh()->GetSocketLocation(Side==0 ? TEXT("upperarm_l") : TEXT("upperarm_r"));
        const FVector Elbow=GetMesh()->GetSocketLocation(Side==0 ? TEXT("lowerarm_l") : TEXT("lowerarm_r"));
        const FVector Wrist=GetMesh()->GetSocketLocation(Side==0 ? TEXT("hand_l") : TEXT("hand_r"));
        const float Reach=.98f*(FVector::Dist(Shoulder,Elbow)+FVector::Dist(Elbow,Wrist));
        const FVector Guard=Shoulder+GetActorForwardVector()*24.f-GetActorRightVector()*Sign*8.f+FVector(0,0,8);
        FVector Target=Guard;
        if ((Side==0)==bLeftPunch && AttackTime>0)
        {
            const FVector Extended=Shoulder+(GetActorForwardVector()-GetActorRightVector()*Sign*.10f
                -FVector(0,0,.045f)).GetSafeNormal()*Reach;
            Target=FMath::Lerp(Guard,Extended,float(Motion.Extension))
                -GetActorForwardVector()*float(8*Motion.Load);
        }
        (Side==0 ? Left : Right)=Target;
    }
    return true;
}
