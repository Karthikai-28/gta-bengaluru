#include "NammaHumanAnimInstance.h"
#include "NammaPlayerCharacter.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

struct FNammaHumanAnimProxy : FAnimInstanceProxy
{
    explicit FNammaHumanAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    float CrouchDepth = 0.f;
    float ReachAlpha = 0.f;
    FVector HandTarget = FVector::ZeroVector;

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        // Read UObjects only on the game thread; Evaluate uses copied values.
        const auto* Character = Cast<ANammaPlayerCharacter>(Instance->TryGetPawnOwner());
        if (!Character) return;
        const float Blend = 1.f - FMath::Exp(-12.f * DeltaSeconds);
        CrouchDepth = FMath::Lerp(CrouchDepth, Character->bIsCrouched ? 60.f : 0.f, Blend);
        FVector Target;
        const bool bReach = Character->GetHandTarget(Target);
        ReachAlpha = FMath::Lerp(ReachAlpha, bReach ? 1.f : 0.f, Blend);
        if (bReach) HandTarget = Instance->GetSkelMeshComponent()->GetComponentTransform().InverseTransformPosition(Target);
    }

    virtual bool Evaluate(FPoseContext& Output) override
    {
        FAnimInstanceProxy::EvaluateAnimationNode(Output);
        FCSPose<FCompactPose> Pose;
        Pose.InitPose(Output.Pose);
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        auto Index = [&Bones](const TCHAR* Name) {
            return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(Name)));
        };
        const auto PelvisIndex = Index(TEXT("pelvis"));
        if (PelvisIndex == INDEX_NONE) return true;
        // Capture planted feet before lowering the pelvis. IK preserves limb lengths.
        FVector Feet[2];
        const TCHAR* FootNames[] = {TEXT("foot_l"), TEXT("foot_r")};
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const auto Foot = Index(FootNames[Side]);
            if (Foot == INDEX_NONE) return true;
            Feet[Side] = Pose.GetComponentSpaceTransform(Foot).GetLocation();
        }
        FTransform Pelvis = Pose.GetComponentSpaceTransform(PelvisIndex);
        Pelvis.AddToTranslation(FVector(0, 0, -CrouchDepth));
        TArray<FBoneTransform> Transforms;
        Transforms.Emplace(PelvisIndex, Pelvis);
        Pose.LocalBlendCSBoneTransforms(Transforms, 1.f);

        auto SolveLimb = [&](const TCHAR* UpperName, const TCHAR* LowerName, const TCHAR* EndName,
                             const FVector& Goal, const FVector& PoleOffset, float Alpha) {
            const auto UpperIndex = Index(UpperName);
            const auto LowerIndex = Index(LowerName);
            const auto EndIndex = Index(EndName);
            if (UpperIndex == INDEX_NONE || LowerIndex == INDEX_NONE || EndIndex == INDEX_NONE) return;
            FTransform Upper = Pose.GetComponentSpaceTransform(UpperIndex);
            FTransform Lower = Pose.GetComponentSpaceTransform(LowerIndex);
            FTransform End = Pose.GetComponentSpaceTransform(EndIndex);
            const FVector Target = FMath::Lerp(End.GetLocation(), Goal, Alpha);
            AnimationCore::SolveTwoBoneIK(Upper, Lower, End, Upper.GetLocation() + PoleOffset,
                Target, false, 1.0, 1.0);
            Transforms.Reset();
            Transforms.Emplace(UpperIndex, Upper);
            Transforms.Emplace(LowerIndex, Lower);
            Transforms.Emplace(EndIndex, End);
            Pose.LocalBlendCSBoneTransforms(Transforms, 1.f);
        };
        if (CrouchDepth > 0.01f)
        {
            // Manny faces +Y in skeletal component space. Knees bend forwards.
            SolveLimb(TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), Feet[0], FVector(0, 100, 0), 1.f);
            SolveLimb(TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), Feet[1], FVector(0, 100, 0), 1.f);
        }
        if (ReachAlpha > 0.01f)
            SolveLimb(TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), HandTarget,
                FVector(60, 0, -50), ReachAlpha);
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(Pose, Output.Pose);
        return true;
    }
};

FAnimInstanceProxy* UNammaHumanAnimInstance::CreateAnimInstanceProxy() { return new FNammaHumanAnimProxy(this); }
void UNammaHumanAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
