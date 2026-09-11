#include "NammaHumanAnimInstance.h"
#include "NammaPlayerCharacter.h"
#include "NammaRidePose.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

struct FNammaHumanAnimProxy : FAnimInstanceProxy
{
    explicit FNammaHumanAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    float CrouchDepth = 0.f;
    float ReachAlpha = 0.f;
    float CombatAlpha = 0.f;
    FVector CombatHands[2] = {FVector::ZeroVector,FVector::ZeroVector};
    float PunchExtension=0.f;
    float PunchTwist=0.f;
    int PunchSide=0;
    FVector HandTarget = FVector::ZeroVector;
    // Riding targets, in mesh component space. The bicycle reports where its pedals
    // and grips actually are, so the legs follow the crank the drivetrain is turning
    // rather than a looping pedal animation.
    float RideAlpha = 0.f;
    float FootDownAlpha = 0.f;
    float TorsoPitch = 0.f;
    FVector RideFoot[2] = {FVector::ZeroVector, FVector::ZeroVector};
    FVector RideSaddle = FVector::ZeroVector;
    FVector RideHand[2] = {FVector::ZeroVector, FVector::ZeroVector};

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        // Read UObjects only on the game thread; Evaluate uses copied values.
        const auto* Character = Cast<ANammaPlayerCharacter>(Instance->TryGetPawnOwner());
        if (!Character) return;
        const float Blend = 1.f - FMath::Exp(-12.f * DeltaSeconds);
        const USkeletalMeshComponent* Mesh = Instance->GetSkelMeshComponent();
        CrouchDepth = FMath::Lerp(CrouchDepth, FMath::Max(Character->bIsCrouched ? 60.f : 0.f, Character->GetRecoveryDepth()), Blend);
        FVector Hands[2];
        const bool bCombat=Character->GetCombatHands(Hands[0],Hands[1]);
        const auto Motion=Character->GetPunchMotion();
        // Follow the authored timing curve directly: a 12 Hz smoothing filter
        // blunted the fast strike and made the hand drift forward like a push.
        CombatAlpha=bCombat ? float(Motion.Weight) : FMath::Lerp(CombatAlpha,0.f,Blend);
        PunchExtension=float(Motion.Extension);
        PunchTwist=float(Motion.Twist);
        PunchSide=Character->IsLeftPunch() ? 0 : 1;
        if (bCombat) for (int Side=0;Side<2;++Side)
            CombatHands[Side]=Mesh->GetComponentTransform().InverseTransformPosition(Hands[Side]);
        FVector Target;
        const bool bReach = Character->GetHandTarget(Target);
        ReachAlpha = FMath::Lerp(ReachAlpha, bReach ? 1.f : 0.f, Blend);
        if (bReach) HandTarget = Mesh->GetComponentTransform().InverseTransformPosition(Target);
        FNammaRidePose Ride;
        const bool bRiding = Character->GetRidePose(Ride);
        RideAlpha = FMath::Lerp(RideAlpha, bRiding ? Ride.Weight : 0.f, Blend);
        if (bRiding)
        {
            const FTransform& ToMesh = Mesh->GetComponentTransform();
            RideSaddle = ToMesh.InverseTransformPosition(Ride.Saddle);
            FootDownAlpha = FMath::Lerp(FootDownAlpha, Ride.bFootDown ? 1.f : 0.f, Blend);
            RideFoot[0] = ToMesh.InverseTransformPosition(FMath::Lerp(Ride.PedalLeft, Ride.FootDownTarget, FootDownAlpha));
            RideFoot[1] = ToMesh.InverseTransformPosition(Ride.PedalRight);
            RideHand[0] = ToMesh.InverseTransformPosition(Ride.GripLeft);
            RideHand[1] = ToMesh.InverseTransformPosition(Ride.GripRight);
            TorsoPitch = Ride.TorsoPitchDegrees;
        }
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
        Pelvis.SetLocation(FMath::Lerp(Pelvis.GetLocation(), RideSaddle, RideAlpha));
        TArray<FBoneTransform> Transforms;
        Transforms.Emplace(PelvisIndex, Pelvis);
        Pose.LocalBlendCSBoneTransforms(Transforms, 1.f);

        auto SolveLimb = [&](const TCHAR* UpperName, const TCHAR* LowerName, const TCHAR* EndName,
                             const FVector& Goal, const FVector& PoleOffset, float Alpha, float ReachLimit=1.f) {
            const auto UpperIndex = Index(UpperName);
            const auto LowerIndex = Index(LowerName);
            const auto EndIndex = Index(EndName);
            if (UpperIndex == INDEX_NONE || LowerIndex == INDEX_NONE || EndIndex == INDEX_NONE) return;
            FTransform Upper = Pose.GetComponentSpaceTransform(UpperIndex);
            FTransform Lower = Pose.GetComponentSpaceTransform(LowerIndex);
            FTransform End = Pose.GetComponentSpaceTransform(EndIndex);
            FVector Target = FMath::Lerp(End.GetLocation(), Goal, Alpha);
            if (ReachLimit<1.f)
            {
                const float Length=FVector::Dist(Upper.GetLocation(),Lower.GetLocation())
                    +FVector::Dist(Lower.GetLocation(),End.GetLocation());
                Target=Upper.GetLocation()+(Target-Upper.GetLocation()).GetClampedToMaxSize(Length*ReachLimit);
            }
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
        if (RideAlpha > 0.01f)
        {
            // Fold the torso over the bars first; the arms are its children, so they
            // are solved onto the grips afterwards.
            const auto SpineIndex = Index(TEXT("spine_01"));
            const auto ChestIndex = Index(TEXT("spine_02"));
            const float Radians = FMath::DegreesToRadians(TorsoPitch * RideAlpha * 0.5f);
            for (const auto Bone : {SpineIndex, ChestIndex})
            {
                if (Bone == INDEX_NONE) continue;
                FTransform Segment = Pose.GetComponentSpaceTransform(Bone);
                Segment.SetRotation(FQuat(FVector::XAxisVector, -Radians) * Segment.GetRotation());
                Transforms.Reset();
                Transforms.Emplace(Bone, Segment);
                Pose.LocalBlendCSBoneTransforms(Transforms, 1.f);
            }
            SolveLimb(TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), RideFoot[0], FVector(0, 100, 0), RideAlpha);
            SolveLimb(TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), RideFoot[1], FVector(0, 100, 0), RideAlpha);
            SolveLimb(TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"), RideHand[0], FVector(-60, 0, -50), RideAlpha);
            SolveLimb(TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), RideHand[1], FVector(60, 0, -50), RideAlpha);
        }
        else if (CombatAlpha > 0.01f)
        {
            auto RotateBone=[&](FCompactPoseBoneIndex Bone,const FQuat& Delta,float Weight)
            {
                if (Bone==INDEX_NONE) return;
                FTransform T=Pose.GetComponentSpaceTransform(Bone);
                T.SetRotation((Delta*T.GetRotation()).GetNormalized());
                Transforms.Reset();Transforms.Emplace(Bone,T);
                Pose.LocalBlendCSBoneTransforms(Transforms,Weight);
            };
            const float SideSign=PunchSide==0 ? -1.f : 1.f;
            const float Twist=FMath::DegreesToRadians(SideSign*14.f*PunchTwist);
            for (const TCHAR* Spine : {TEXT("spine_01"),TEXT("spine_02"),TEXT("spine_03")})
                RotateBone(Index(Spine),FQuat(FVector::ZAxisVector,Twist/3.f),CombatAlpha);
            RotateBone(Index(TEXT("head")),FQuat(FVector::ZAxisVector,-Twist*.75f),CombatAlpha);
            RotateBone(Index(PunchSide==0 ? TEXT("clavicle_l") : TEXT("clavicle_r")),
                FQuat(FVector::ZAxisVector,FMath::DegreesToRadians(SideSign*5.f*PunchExtension)),CombatAlpha);
            for (int Side=0;Side<2;++Side)
            {
                const bool Left=Side==0;
                const float Sign=Left ? -1.f : 1.f;
                const float Extension=Side==PunchSide ? PunchExtension : 0.f;
                // Elbows stay down and close to the ribs, then track behind the
                // fist. Retain two percent reach margin to avoid a locked/flipping elbow.
                const FVector Pole(Sign*(8.f+5.f*Extension),15.f,-45.f+20.f*Extension);
                SolveLimb(Left ? TEXT("upperarm_l") : TEXT("upperarm_r"),
                    Left ? TEXT("lowerarm_l") : TEXT("lowerarm_r"),
                    Left ? TEXT("hand_l") : TEXT("hand_r"),CombatHands[Side],Pole,CombatAlpha,.98f);
                const auto Hand=Index(Left ? TEXT("hand_l") : TEXT("hand_r"));
                const auto Elbow=Index(Left ? TEXT("lowerarm_l") : TEXT("lowerarm_r"));
                const auto Middle=Index(Left ? TEXT("middle_01_l") : TEXT("middle_01_r"));
                const auto Forefinger=Index(Left ? TEXT("index_01_l") : TEXT("index_01_r"));
                const auto Pinky=Index(Left ? TEXT("pinky_01_l") : TEXT("pinky_01_r"));
                if (Hand==INDEX_NONE || Elbow==INDEX_NONE || Middle==INDEX_NONE
                    || Forefinger==INDEX_NONE || Pinky==INDEX_NONE) continue;
                const FVector Wrist=Pose.GetComponentSpaceTransform(Hand).GetLocation();
                const FVector OldForward=(Pose.GetComponentSpaceTransform(Middle).GetLocation()-Wrist).GetSafeNormal();
                const FVector Across=Pose.GetComponentSpaceTransform(Forefinger).GetLocation()
                    -Pose.GetComponentSpaceTransform(Pinky).GetLocation();
                FVector OldNormal=(FVector::CrossProduct(Across,OldForward)*(Left ? 1.f : -1.f)).GetSafeNormal();
                const auto Middle2=Index(Left ? TEXT("middle_02_l") : TEXT("middle_02_r"));
                const auto Middle3=Index(Left ? TEXT("middle_03_l") : TEXT("middle_03_r"));
                if (Middle2!=INDEX_NONE && Middle3!=INDEX_NONE)
                {
                    const FVector Prox=(Pose.GetComponentSpaceTransform(Middle2).GetLocation()-Pose.GetComponentSpaceTransform(Middle).GetLocation()).GetSafeNormal();
                    const FVector Dist=(Pose.GetComponentSpaceTransform(Middle3).GetLocation()-Pose.GetComponentSpaceTransform(Middle2).GetLocation()).GetSafeNormal();
                    // The existing relaxed finger curvature identifies the palm
                    // side, avoiding a 180-degree roll on mirrored hand rigs.
                    const FVector Bend=Dist-Prox*FVector::DotProduct(Dist,Prox);
                    if (FVector::DotProduct(Bend,OldNormal)<0) OldNormal=-OldNormal;
                }
                const FVector Aim=(Wrist-Pose.GetComponentSpaceTransform(Elbow).GetLocation()).GetSafeNormal();
                // Build a stable frame around the forearm. Interpolating two
                // palm normals can cross a parallel singularity and flip 180 deg.
                FVector PalmBase=FVector::CrossProduct(Aim,FVector::XAxisVector).GetSafeNormal();
                if (PalmBase.IsNearlyZero()) PalmBase=FVector::CrossProduct(Aim,FVector::ZAxisVector).GetSafeNormal();
                const FVector Palm=FQuat(Aim,Sign*HALF_PI*(1.f-Extension)).RotateVector(PalmBase);
                if (OldForward.IsNearlyZero() || OldNormal.IsNearlyZero() || Aim.IsNearlyZero()) continue;
                const FQuat Before=FRotationMatrix::MakeFromXZ(OldForward,OldNormal).ToQuat();
                const FQuat After=FRotationMatrix::MakeFromXZ(Aim,Palm).ToQuat();
                RotateBone(Hand,After*Before.Inverse(),CombatAlpha);
                // Derive the hinge axis from the hand frame, not a fixed local
                // Euler axis: Manny's left and right finger bones are mirrored.
                const FVector CurlAxis=FVector::CrossProduct(Aim,Palm).GetSafeNormal();
                if (CurlAxis.IsNearlyZero()) continue;
                for (const TCHAR* Finger : {TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky"),TEXT("thumb")})
                {
                    const bool Thumb=FCString::Strcmp(Finger,TEXT("thumb"))==0;
                    const float Angles[3]={Thumb ? 25.f : 65.f,Thumb ? 35.f : 85.f,Thumb ? 20.f : 45.f};
                    const FString Prefix=FString::Printf(TEXT("%s_"),Finger);
                    const auto First=Index(*(Prefix+TEXT("01_")+(Left ? TEXT("l") : TEXT("r"))));
                    const auto Second=Index(*(Prefix+TEXT("02_")+(Left ? TEXT("l") : TEXT("r"))));
                    if (First==INDEX_NONE || Second==INDEX_NONE) continue;
                    const FVector Base=Thumb ? (Pose.GetComponentSpaceTransform(Second).GetLocation()
                        -Pose.GetComponentSpaceTransform(First).GetLocation()).GetSafeNormal() : Aim;
                    const FVector Axis=Thumb ? FVector::CrossProduct(Base,Palm).GetSafeNormal() : CurlAxis;
                    float TotalAngle=0;
                    for (int Joint=0;Joint<3;++Joint)
                    {
                        const FString Name=FString::Printf(TEXT("%s_%02d_%s"),Finger,Joint+1,Left ? TEXT("l") : TEXT("r"));
                        const auto Bone=Index(*Name);
                        if (Bone==INDEX_NONE) continue;
                        FVector Current;
                        if (Joint<2)
                        {
                            const FString ChildName=FString::Printf(TEXT("%s_%02d_%s"),Finger,Joint+2,Left ? TEXT("l") : TEXT("r"));
                            const auto Child=Index(*ChildName);
                            if (Child==INDEX_NONE) continue;
                            Current=(Pose.GetComponentSpaceTransform(Child).GetLocation()-Pose.GetComponentSpaceTransform(Bone).GetLocation()).GetSafeNormal();
                        }
                        else
                        {
                            const FTransform Parent=Pose.GetComponentSpaceTransform(Second);
                            const FTransform Tip=Pose.GetComponentSpaceTransform(Bone);
                            const FVector LocalAxis=Parent.InverseTransformVectorNoScale((Tip.GetLocation()-Parent.GetLocation()).GetSafeNormal());
                            Current=Tip.TransformVectorNoScale(LocalAxis).GetSafeNormal();
                        }
                        TotalAngle+=Angles[Joint];
                        const FVector Desired=FQuat(Axis,FMath::DegreesToRadians(TotalAngle)).RotateVector(Base);
                        if (!Current.IsNearlyZero() && !Axis.IsNearlyZero())
                            RotateBone(Bone,FQuat::FindBetweenNormals(Current,Desired),CombatAlpha);
                    }
                }
            }
        }
        else if (ReachAlpha > 0.01f)
            SolveLimb(TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), HandTarget,
                FVector(60, 0, -50), ReachAlpha);
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(Pose, Output.Pose);
        return true;
    }
};

FAnimInstanceProxy* UNammaHumanAnimInstance::CreateAnimInstanceProxy() { return new FNammaHumanAnimProxy(this); }
void UNammaHumanAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
