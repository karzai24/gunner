#include "GunnerAnimationBuilder.h"

#include "GunnerAnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_RotationOffsetBlendSpace.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimBlueprintFactory.h"
#include "HAL/FileManager.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGunnerAnimationBuilder, Log, All);

namespace GunnerAnimationBuilder
{
    UPackage* MakeNewPackage(const FString& Path)
    {
        if (!Path.StartsWith(TEXT("/Game/Gunner/")) || !FPackageName::IsValidLongPackageName(Path)
            || FPackageName::DoesPackageExist(Path) || FindPackage(nullptr, *Path))
        {
            UE_LOG(LogGunnerAnimationBuilder, Error,
                TEXT("Refusing invalid or existing asset package: %s"), *Path);
            return nullptr;
        }
        return CreatePackage(*Path);
    }

    bool SaveNewAsset(UObject* Asset)
    {
        UPackage* Package = Asset->GetOutermost();
        FAssetRegistryModule::AssetCreated(Asset);
        Package->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        const bool bSaved = UPackage::SavePackage(Package, Asset, *Filename, Args);
        if (!bSaved)
        {
            UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Could not save %s"), *Filename);
        }
        return bSaved;
    }

    template <typename T, typename Configure>
    T* Node(UEdGraph* Graph, int32 X, int32 Y, Configure Setup)
    {
        FGraphNodeCreator<T> Creator(*Graph);
        T* Result = Creator.CreateNode();
        Result->NodePosX = X;
        Result->NodePosY = Y;
        Setup(Result);
        Creator.Finalize();
        return Result;
    }

    template <typename T>
    T* Node(UEdGraph* Graph, int32 X, int32 Y)
    {
        return Node<T>(Graph, X, Y, [](T*) {});
    }

    struct FGraphBuilder
    {
        UEdGraph* Graph;
        bool bValid = true;

        void Connect(UEdGraphNode* Source, FName Output, UEdGraphNode* Target, FName Input)
        {
            UEdGraphPin* OutPin = Source->FindPin(Output, EGPD_Output);
            UEdGraphPin* InPin = Target->FindPin(Input, EGPD_Input);
            if (!OutPin || !InPin || !Graph->GetSchema()->TryCreateConnection(OutPin, InPin))
            {
                UE_LOG(LogGunnerAnimationBuilder, Error,
                    TEXT("Could not connect %s.%s to %s.%s"),
                    *Source->GetClass()->GetName(), *Output.ToString(),
                    *Target->GetClass()->GetName(), *Input.ToString());
                bValid = false;
            }
        }

        void Pose(UEdGraphNode* Source, UEdGraphNode* Target, FName Input)
        {
            Connect(Source, TEXT("Pose"), Target, Input);
        }

        void Read(FName Property, UEdGraphNode* Target, FName Input)
        {
            UK2Node_VariableGet* Getter = Node<UK2Node_VariableGet>(Graph,
                Target->NodePosX - 250, Target->NodePosY - 100,
                [Property](UK2Node_VariableGet* N) { N->VariableReference.SetSelfMember(Property); });
            Connect(Getter, Property, Target, Input);
        }

        UAnimGraphNode_BlendSpacePlayer* Locomotion(UBlendSpace* Asset, int32 X, int32 Y,
            bool bDirectional = true)
        {
            auto* Player = Node<UAnimGraphNode_BlendSpacePlayer>(Graph, X, Y,
                [Asset](UAnimGraphNode_BlendSpacePlayer* N)
                {
                    N->Node.SetBlendSpace(Asset);
                    N->Node.SetGroupName(TEXT("Locomotion"));
                    N->Node.SetGroupMethod(EAnimSyncMethod::SyncGroup);
                    N->Node.SetLoop(true);
                });
            Read(bDirectional ? FName(TEXT("ForwardSpeed")) : FName(TEXT("Speed")), Player, TEXT("X"));
            if (bDirectional) Read(TEXT("RightSpeed"), Player, TEXT("Y"));
            return Player;
        }

        UAnimGraphNode_SequencePlayer* Sequence(UAnimSequence* Asset, int32 X, int32 Y)
        {
            return Node<UAnimGraphNode_SequencePlayer>(Graph, X, Y,
                [Asset](UAnimGraphNode_SequencePlayer* N)
                {
                    N->Node.SetSequence(Asset);
                    N->Node.SetLoopAnimation(true);
                });
        }

        UAnimGraphNode_BlendListByBool* Choose(FName Property, UEdGraphNode* TruePose,
            UEdGraphNode* FalsePose, int32 X, int32 Y, float BlendTime = 0.18f)
        {
            auto* Blend = Node<UAnimGraphNode_BlendListByBool>(Graph, X, Y);
            Read(Property, Blend, TEXT("bActiveValue"));
            Pose(TruePose, Blend, TEXT("BlendPose_0"));
            Pose(FalsePose, Blend, TEXT("BlendPose_1"));
            for (const FName PinName : {FName(TEXT("BlendTime_0")), FName(TEXT("BlendTime_1"))})
            {
                if (UEdGraphPin* Pin = Blend->FindPin(PinName))
                {
                    Graph->GetSchema()->TrySetDefaultValue(*Pin, FString::SanitizeFloat(BlendTime));
                }
            }
            return Blend;
        }

        UAnimGraphNode_UseCachedPose* Use(UAnimGraphNode_SaveCachedPose* Saved, int32 X, int32 Y)
        {
            return Node<UAnimGraphNode_UseCachedPose>(Graph, X, Y,
                [Saved](UAnimGraphNode_UseCachedPose* N) { N->SaveCachedPoseNode = Saved; });
        }

        UAnimGraphNode_SaveCachedPose* Cache(FName Name, UEdGraphNode* Source, int32 X, int32 Y)
        {
            auto* Saved = Node<UAnimGraphNode_SaveCachedPose>(Graph, X, Y,
                [Name](UAnimGraphNode_SaveCachedPose* N) { N->CacheName = Name.ToString(); });
            Pose(Source, Saved, TEXT("Pose"));
            return Saved;
        }

        UAnimGraphNode_LayeredBoneBlend* Arms(UEdGraphNode* Base, UEdGraphNode* ArmsPose,
            int32 X, int32 Y, FName WeightProperty = NAME_None)
        {
            auto* Blend = Node<UAnimGraphNode_LayeredBoneBlend>(Graph, X, Y,
                [](UAnimGraphNode_LayeredBoneBlend* N)
                {
                    N->Node.bMeshSpaceRotationBlend = true;
                    N->Node.BlendWeights[0] = 1.f;
                    for (const FName Bone : {FName(TEXT("clavicle_l")), FName(TEXT("clavicle_r"))})
                    {
                        FBranchFilter Filter;
                        Filter.BoneName = Bone;
                        Filter.BlendDepth = 1;
                        N->Node.LayerSetup[0].BranchFilters.Add(Filter);
                    }
                });
            Pose(Base, Blend, TEXT("BasePose"));
            Pose(ArmsPose, Blend, TEXT("BlendPoses_0"));
            if (!WeightProperty.IsNone()) Read(WeightProperty, Blend, TEXT("BlendWeights_0"));
            return Blend;
        }

        UEdGraphNode* BlindArms(UEdGraphNode* Base, UEdGraphNode* ArmedPose, int32 X, int32 Y)
        {
            auto* Grip = Arms(Base, ArmedPose, X, Y);
            auto* Component = Node<UAnimGraphNode_LocalToComponentSpace>(Graph, X + 400, Y);
            Pose(Grip, Component, TEXT("LocalPose"));
            UEdGraphNode* Previous = Component;
            FName PreviousPin = TEXT("ComponentPose");
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const bool bRight = Side == 0;
                const FName Bone = bRight ? FName(TEXT("hand_r")) : FName(TEXT("hand_l"));
                auto* IK = Node<UAnimGraphNode_TwoBoneIK>(Graph, X + 900 + Side * 1000, Y,
                    [Bone](UAnimGraphNode_TwoBoneIK* N)
                    {
                        N->Node.IKBone.BoneName = Bone;
                        N->Node.EffectorLocationSpace = BCS_ComponentSpace;
                        N->Node.JointTargetLocationSpace = BCS_ComponentSpace;
                        N->Node.bAllowStretching = false;
                        N->Node.bMaintainEffectorRelRot = false;
                        N->Node.bTakeRotationFromEffectorSpace = false;
                        N->Node.Alpha = 1.f;
                    });
                Connect(Previous, PreviousPin, IK, TEXT("ComponentPose"));
                Read(bRight ? FName(TEXT("BlindRightHandLocation")) : FName(TEXT("BlindLeftHandLocation")), IK, TEXT("EffectorLocation"));
                Read(bRight ? FName(TEXT("BlindRightElbowLocation")) : FName(TEXT("BlindLeftElbowLocation")), IK, TEXT("JointTargetLocation"));
                auto* Hand = Node<UAnimGraphNode_ModifyBone>(Graph, X + 1400 + Side * 1000, Y,
                    [Bone](UAnimGraphNode_ModifyBone* N)
                    {
                        N->Node.BoneToModify.BoneName = Bone;
                        N->Node.RotationMode = BMM_Replace;
                        N->Node.RotationSpace = BCS_ComponentSpace;
                        N->Node.Alpha = 1.f;
                    });
                Pose(IK, Hand, TEXT("ComponentPose"));
                Read(bRight ? FName(TEXT("BlindRightHandRotation")) : FName(TEXT("BlindLeftHandRotation")), Hand, TEXT("Rotation"));
                Previous = Hand;
                PreviousPin = TEXT("Pose");
            }
            auto* Local = Node<UAnimGraphNode_ComponentToLocalSpace>(Graph, X + 2900, Y);
            Pose(Previous, Local, TEXT("ComponentPose"));
            return Local;
        }

        UEdGraphNode* Aim(UBlendSpace* Asset, UEdGraphNode* Base, int32 X, int32 Y)
        {
            if (!Asset) return Base;
            auto* Offset = Node<UAnimGraphNode_RotationOffsetBlendSpace>(Graph, X, Y,
                [Asset](UAnimGraphNode_RotationOffsetBlendSpace* N)
                {
                    N->Node.SetBlendSpace(Asset);
                    N->Node.Alpha = 1.f;
                });
            Pose(Base, Offset, TEXT("BasePose"));
            // The installed template's 2D offsets have unnamed normalized axes and all
            // samples on Y. Infer the occupied axis instead of assuming X or degrees.
            float ExtentX = 0.f;
            float ExtentY = 0.f;
            for (const FBlendSample& Sample : Asset->GetBlendSamples())
            {
                ExtentX = FMath::Max(ExtentX, static_cast<float>(FMath::Abs(Sample.SampleValue.X)));
                ExtentY = FMath::Max(ExtentY, static_cast<float>(FMath::Abs(Sample.SampleValue.Y)));
            }
            const bool bPitchOnY = ExtentY > ExtentX ||
                Asset->GetBlendParameter(1).DisplayName.Contains(TEXT("Pitch"));
            const FBlendParameter& Axis = Asset->GetBlendParameter(bPitchOnY ? 1 : 0);
            const bool bNormalized = FMath::Max(FMath::Abs(Axis.Min), FMath::Abs(Axis.Max)) <= 1.1f;
            Read(bNormalized ? FName(TEXT("AimPitchNormalized")) : FName(TEXT("AimPitch")),
                Offset, bPitchOnY ? FName(TEXT("Y")) : FName(TEXT("X")));
            return Offset;
        }

        UAnimGraphNode_Slot* Slot(FName Name, UEdGraphNode* Source, int32 X, int32 Y)
        {
            auto* Result = Node<UAnimGraphNode_Slot>(Graph, X, Y,
                [Name](UAnimGraphNode_Slot* N)
                {
                    N->Node.SlotName = Name;
                    N->Node.bAlwaysUpdateSourcePose = true;
                });
            Pose(Source, Result, TEXT("Source"));
            return Result;
        }
    };
}

UBlendSpace* UGunnerAnimationBuilder::CreateDirectionalBlendSpace(const FString& PackagePath,
    USkeleton* Skeleton, USkeletalMesh* PreviewMesh, const TArray<UAnimSequence*>& Clips,
    const TArray<FVector>& SamplePositions, float MaxAxisSpeed)
{
    using namespace GunnerAnimationBuilder;
    if (!Skeleton || !PreviewMesh || PreviewMesh->GetSkeleton() != Skeleton || Clips.IsEmpty()
        || Clips.Num() != SamplePositions.Num() || !FMath::IsFinite(MaxAxisSpeed) || MaxAxisSpeed <= 0.f)
    {
        UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Invalid directional Blend Space arguments"));
        return nullptr;
    }
    for (int32 Index = 0; Index < Clips.Num(); ++Index)
    {
        if (!Clips[Index] || Clips[Index]->GetSkeleton() != Skeleton
            || SamplePositions[Index].ContainsNaN() || FMath::Abs(SamplePositions[Index].X) > MaxAxisSpeed
            || FMath::Abs(SamplePositions[Index].Y) > MaxAxisSpeed)
        {
            UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Invalid or unretargeted sample %d"), Index);
            return nullptr;
        }
    }
    UPackage* Package = MakeNewPackage(PackagePath);
    if (!Package) return nullptr;
    UBlendSpace* Blend = NewObject<UBlendSpace>(Package, *FPackageName::GetLongPackageAssetName(PackagePath),
        RF_Public | RF_Standalone | RF_Transactional);
    Blend->SetSkeleton(Skeleton);
    Blend->SetPreviewMesh(PreviewMesh);
    // BlendParameters is an editor-editable static array without a public setter in UE 5.8.
    FStructProperty* Parameters = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
    if (!Parameters) return nullptr;
    for (int32 Axis = 0; Axis < 2; ++Axis)
    {
        FBlendParameter* Parameter = Parameters->ContainerPtrToValuePtr<FBlendParameter>(Blend, Axis);
        Parameter->DisplayName = Axis == 0 ? TEXT("Forward Speed") : TEXT("Right Speed");
        Parameter->Min = -MaxAxisSpeed;
        Parameter->Max = MaxAxisSpeed;
        Parameter->GridNum = 8;
        Blend->InterpolationParam[Axis].InterpolationTime = 0.08f;
    }
    Blend->TargetWeightInterpolationSpeedPerSec = 8.f;
    for (int32 Index = 0; Index < Clips.Num(); ++Index)
    {
        if (Blend->AddSample(Clips[Index], SamplePositions[Index]) == INDEX_NONE)
        {
            UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Rejected Blend Space sample %d"), Index);
            return nullptr;
        }
    }
    Blend->ValidateSampleData();
    Blend->ResampleData();
    Blend->PostEditChange();
    return SaveNewAsset(Blend) ? Blend : nullptr;
}

UAnimBlueprint* UGunnerAnimationBuilder::CreateLocomotionBlueprint(const FString& PackagePath,
    USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
    UBlendSpace* RifleLocomotion, UBlendSpace* PistolLocomotion,
    UAnimSequence* RifleFall, UAnimSequence* PistolFall,
    UBlendSpace* CrouchLocomotion, UAnimSequence* Sprint,
    UBlendSpace* RifleAimOffset, UBlendSpace* PistolAimOffset, bool bIncludeBlindFire,
    bool bIncludeCrouchReload)
{
    using namespace GunnerAnimationBuilder;
    const bool bIncludeProtectiveArms = bIncludeBlindFire || bIncludeCrouchReload;
    if (bIncludeProtectiveArms && !CrouchLocomotion)
    {
        UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Protective weapon actions require authored crouch locomotion"));
        return nullptr;
    }
    if (!Skeleton || !PreviewMesh || !RifleLocomotion || !PistolLocomotion || !RifleFall || !PistolFall
        || PreviewMesh->GetSkeleton() != Skeleton)
    {
        UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Animation Blueprint requires the canonical mesh and armed locomotion/fall assets"));
        return nullptr;
    }
    const UAnimationAsset* Assets[] = {RifleLocomotion, PistolLocomotion, RifleFall, PistolFall,
        CrouchLocomotion, Sprint, RifleAimOffset, PistolAimOffset};
    for (const UAnimationAsset* Asset : Assets)
    {
        if (Asset && Asset->GetSkeleton() != Skeleton)
        {
            UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Retarget %s to the canonical skeleton before graph creation"), *Asset->GetName());
            return nullptr;
        }
    }
    UPackage* Package = MakeNewPackage(PackagePath);
    if (!Package) return nullptr;
    auto* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->ParentClass = UGunnerAnimInstance::StaticClass();
    Factory->TargetSkeleton = Skeleton;
    Factory->PreviewSkeletalMesh = PreviewMesh;
    auto* Blueprint = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), Package,
        *FPackageName::GetLongPackageAssetName(PackagePath), RF_Public | RF_Standalone | RF_Transactional,
        nullptr, GWarn));
    if (!Blueprint) return nullptr;
    UEdGraph* Graph = nullptr;
    for (UEdGraph* Candidate : Blueprint->FunctionGraphs)
    {
        if (Candidate && Candidate->GetFName() == UEdGraphSchema_K2::GN_AnimGraph)
        {
            Graph = Candidate;
            break;
        }
    }
    if (!Graph)
    {
        Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, UEdGraphSchema_K2::GN_AnimGraph,
            UAnimationGraph::StaticClass(), UAnimationGraphSchema::StaticClass());
        Blueprint->FunctionGraphs.Add(Graph);
        Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
    }
    TArray<UAnimGraphNode_Root*> Roots;
    Graph->GetNodesOfClass(Roots);
    if (Roots.Num() != 1) return nullptr;
    auto* Root = Roots[0];
    Root->NodePosX = 3000;
    Root->NodePosY = 0;

    FGraphBuilder B{Graph};
    auto* Rifle = B.Locomotion(RifleLocomotion, -2200, -300);
    auto* Pistol = B.Locomotion(PistolLocomotion, -2200, 300);
    auto* Armed = B.Choose(TEXT("bPistol"), Pistol, Rifle, -1800, 0, 0.22f);
    auto* ArmedCache = Node<UAnimGraphNode_SaveCachedPose>(Graph, -1400, 0,
        [](UAnimGraphNode_SaveCachedPose* N) { N->CacheName = TEXT("Armed locomotion"); });
    B.Pose(Armed, ArmedCache, TEXT("Pose"));

    UEdGraphNode* Ground = B.Use(ArmedCache, -1100, 400);
    if (CrouchLocomotion)
    {
        // Current source pack has a genuine forward crouch gait only. The character
        // faces travel; speed magnitude keeps feet moving through its turn interpolation.
        auto* Crouch = B.Locomotion(CrouchLocomotion, -1400, 850, false);
        Ground = B.Choose(TEXT("bCrouched"), Crouch, Ground, -700, 550, 0.2f);
    }
    if (Sprint)
    {
        auto* SprintPlayer = B.Sequence(Sprint, -700, 1000);
        Ground = B.Choose(TEXT("bSprinting"), SprintPlayer, Ground, -250, 600, 0.22f);
    }
    auto* RifleAir = B.Sequence(RifleFall, -750, 1400);
    auto* PistolAir = B.Sequence(PistolFall, -750, 1700);
    auto* Air = B.Choose(TEXT("bPistol"), PistolAir, RifleAir, -300, 1500);
    auto* Body = B.Choose(TEXT("bInAir"), Air, Ground, 200, 700, 0.12f);

    auto* RifleAim = B.Aim(RifleAimOffset, B.Use(ArmedCache, -1000, -800), -500, -800);
    auto* PistolAim = B.Aim(PistolAimOffset, B.Use(ArmedCache, -1000, -350), -500, -350);
    auto* Aim = B.Choose(TEXT("bPistol"), PistolAim, RifleAim, 0, -500, 0.22f);
    UEdGraphNode* BodySource = Body;
    UEdGraphNode* SlotSource = Aim;
    if (bIncludeProtectiveArms)
    {
        auto* BodyCache = B.Cache(TEXT("Protective body"), Body, 500, 700);
        auto* AimCache = B.Cache(TEXT("Armed aim"), Aim, 400, -850);
        SlotSource = B.Use(AimCache, 4700, 500);
        if (bIncludeBlindFire)
        {
            auto* Blind = B.BlindArms(B.Use(BodyCache, 900, 1700),
                B.Use(AimCache, 900, 1400), 1400, 1500);
            // Recoil evaluates after the IK solve. During reload this branch is
            // bypassed and the montage supplies the unpinned handling motion.
            SlotSource = B.Choose(TEXT("bBlindFiring"), Blind, SlotSource, 5100, 750, 0.f);
        }
        BodySource = B.Use(BodyCache, 5200, 1500);
    }
    auto* UpperSlot = B.Slot(TEXT("UpperBody"), SlotSource, bIncludeProtectiveArms ? 5600 : 500, -500);
    UEdGraphNode* UpperPose = UpperSlot;
    UAnimGraphNode_SaveCachedPose* SlotCache = nullptr;
    if (bIncludeProtectiveArms)
    {
        SlotCache = B.Cache(TEXT("Weapon action"), UpperSlot, 6000, -500);
        UpperPose = B.Use(SlotCache, 6300, -500);
    }
    auto* UpperBlend = Node<UAnimGraphNode_LayeredBoneBlend>(Graph, 1100, 0,
        [](UAnimGraphNode_LayeredBoneBlend* N)
        {
            N->Node.bMeshSpaceRotationBlend = true;
            FBranchFilter Filter;
            Filter.BoneName = TEXT("spine_01");
            Filter.BlendDepth = 3;
            N->Node.LayerSetup[0].BranchFilters.Add(Filter);
        });
    B.Pose(BodySource, UpperBlend, TEXT("BasePose"));
    B.Pose(UpperPose, UpperBlend, TEXT("BlendPoses_0"));
    B.Read(TEXT("UpperBodyWeight"), UpperBlend, TEXT("BlendWeights_0"));
    UEdGraphNode* FinalBody = UpperBlend;
    if (bIncludeProtectiveArms)
    {
        UpperBlend->NodePosX = 6700;
        FinalBody = B.Arms(UpperBlend, B.Use(SlotCache, 6800, 600), 7200, 0,
            bIncludeCrouchReload ? FName(TEXT("ProtectiveArmsWeight")) : FName(TEXT("BlindFireAlpha")));
        Root->NodePosX = 8100;
    }
    auto* FullBody = B.Slot(TEXT("FullBody"), FinalBody, bIncludeProtectiveArms ? 7700 : 1900, 0);
    B.Pose(FullBody, Root, TEXT("Result"));
    if (!B.bValid) return nullptr;

    // Slot names, not "group.slot" strings, go on montage tracks. Separate groups allow
    // authored FullBody actions to blend over the UpperBody layer; gameplay guards interruption.
    Skeleton->RegisterSlotNode(TEXT("UpperBody"));
    Skeleton->SetSlotGroupName(TEXT("UpperBody"), TEXT("Weapon"));
    Skeleton->RegisterSlotNode(TEXT("FullBody"));
    Skeleton->SetSlotGroupName(TEXT("FullBody"), TEXT("Traversal"));
    Skeleton->MarkPackageDirty();

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
    if (Results.NumErrors > 0 || Blueprint->Status == BS_Error)
    {
        UE_LOG(LogGunnerAnimationBuilder, Error, TEXT("Animation Blueprint compilation failed with %d errors"), Results.NumErrors);
        return nullptr;
    }
    auto* Defaults = Cast<UGunnerAnimInstance>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!Defaults) return nullptr;
    Defaults->Modify();
    Defaults->bBlindFirePoseReady = bIncludeBlindFire;
    Defaults->bCrouchReloadPoseReady = bIncludeCrouchReload;
    Blueprint->Modify();
    return SaveNewAsset(Blueprint) ? Blueprint : nullptr;
}

bool UGunnerAnimationBuilder::RepairBlindFireReadiness(UAnimBlueprint* Blueprint)
{
    if (!Blueprint || Blueprint->GetOutermost()->GetName() !=
        TEXT("/Game/Gunner/Motion/Animation/ABP_WardenBlindFire") ||
        !Blueprint->GeneratedClass || Blueprint->Status == BS_Error)
        return false;
    UEdGraph* Graph = nullptr;
    for (UEdGraph* Candidate : Blueprint->FunctionGraphs)
        if (Candidate && Candidate->GetFName() == UEdGraphSchema_K2::GN_AnimGraph) Graph = Candidate;
    if (!Graph) return false;
    TArray<UAnimGraphNode_TwoBoneIK*> IKNodes;
    TArray<UAnimGraphNode_ModifyBone*> HandNodes;
    TArray<UAnimGraphNode_LayeredBoneBlend*> Layers;
    TArray<UAnimGraphNode_Slot*> Slots;
    Graph->GetNodesOfClass(IKNodes);
    Graph->GetNodesOfClass(HandNodes);
    Graph->GetNodesOfClass(Layers);
    Graph->GetNodesOfClass(Slots);
    if (IKNodes.Num() != 2 || HandNodes.Num() != 2 || Layers.Num() != 3 || Slots.Num() != 2)
        return false;
    for (const FName Hand : {FName(TEXT("hand_r")), FName(TEXT("hand_l"))})
    {
        const auto* IK = IKNodes.FindByPredicate([Hand](const UAnimGraphNode_TwoBoneIK* N)
            { return N->Node.IKBone.BoneName == Hand; });
        const auto* Rotation = HandNodes.FindByPredicate([Hand](const UAnimGraphNode_ModifyBone* N)
            { return N->Node.BoneToModify.BoneName == Hand; });
        if (!IK || !Rotation || (*IK)->Node.bAllowStretching ||
            (*IK)->Node.EffectorLocationSpace != BCS_ComponentSpace ||
            (*Rotation)->Node.RotationMode != BMM_Replace ||
            (*Rotation)->Node.RotationSpace != BCS_ComponentSpace)
            return false;
        for (const FName PinName : {FName(TEXT("ComponentPose")), FName(TEXT("EffectorLocation")), FName(TEXT("JointTargetLocation"))})
        {
            const UEdGraphPin* Pin = (*IK)->FindPin(PinName, EGPD_Input);
            if (!Pin || Pin->LinkedTo.Num() != 1) return false;
        }
    }
    int32 ArmMasks = 0;
    bool bDrivenArmMask = false;
    for (const auto* Layer : Layers)
    {
        if (Layer->Node.LayerSetup.Num() != 1) return false;
        const auto& Filters = Layer->Node.LayerSetup[0].BranchFilters;
        if (Filters.Num() == 2 &&
            Filters.ContainsByPredicate([](const FBranchFilter& F) { return F.BoneName == TEXT("clavicle_l"); }) &&
            Filters.ContainsByPredicate([](const FBranchFilter& F) { return F.BoneName == TEXT("clavicle_r"); }))
        {
            ++ArmMasks;
            const UEdGraphPin* Weight = Layer->FindPin(TEXT("BlendWeights_0"), EGPD_Input);
            if (Weight && Weight->LinkedTo.Num() == 1)
            {
                const auto* Getter = Cast<UK2Node_VariableGet>(Weight->LinkedTo[0]->GetOwningNode());
                bDrivenArmMask |= Getter && Getter->VariableReference.GetMemberName() == TEXT("BlindFireAlpha");
            }
        }
    }
    if (ArmMasks != 2 || !bDrivenArmMask ||
        !Slots.ContainsByPredicate([](const UAnimGraphNode_Slot* N) { return N->Node.SlotName == TEXT("UpperBody"); }) ||
        !Slots.ContainsByPredicate([](const UAnimGraphNode_Slot* N) { return N->Node.SlotName == TEXT("FullBody"); }))
        return false;
    auto* Defaults = Cast<UGunnerAnimInstance>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!Defaults) return false;
    Defaults->Modify();
    Blueprint->Modify();
    Defaults->bBlindFirePoseReady = true;
    Blueprint->MarkPackageDirty();
    return true;
}
