#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GunnerAnimationBuilder.generated.h"

class UAnimBlueprint;
class UAnimSequence;
class UBlendSpace;
class USkeleton;
class USkeletalMesh;

/** Editor-only authoring helpers. Refuse existing packages so authored assets are never overwritten. */
UCLASS()
class GUNNEREDITOR_API UGunnerAnimationBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Retargeted local derivatives must not reimport source-skeleton FBX directly.
     * Clears only import metadata; source files/provenance stay in external reports. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static bool ClearDerivedAnimationReimportSource(UAnimSequence* Sequence);

    /** Read every editable frame of a named bone through the current data-model API.
     * UE 5.8 sequencer models intentionally return no deprecated FBoneAnimationTracks. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static TArray<FTransform> GetEditableBoneTrackTransforms(UAnimSequence* Sequence, FName Bone);

    /** Local-only derivative of a non-additive sequence with identical reference bones.
     * Refuses paths outside /Game/Gunner/LicensedLocal and existing packages.
     * Root-lock derivatives preserve raw tracks while CharacterMovement owns displacement. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static UAnimSequence* DuplicateCompatibleSequence(const FString& PackagePath,
        UAnimSequence* Source, USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
        bool bRootLock = true);

    /** Sample coordinates are (forward cm/s, right cm/s, 0). Produces an editable 2D Blend Space. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static UBlendSpace* CreateDirectionalBlendSpace(const FString& PackagePath,
        USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
        const TArray<UAnimSequence*>& Clips, const TArray<FVector>& SamplePositions,
        float MaxAxisSpeed = 450.f);

    /** Null optional crouch/sprint assets omit those branches; never substitute standing crouch.
     * Default crouch uses X=Speed and faces travel. Directional crouch requires authored
     * signed forward/right samples and uses X=ForwardSpeed, Y=RightSpeed. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static UAnimBlueprint* CreateLocomotionBlueprint(const FString& PackagePath,
        USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
        UBlendSpace* RifleLocomotion, UBlendSpace* PistolLocomotion,
        UAnimSequence* RifleFall, UAnimSequence* PistolFall,
        UBlendSpace* CrouchLocomotion, UAnimSequence* Sprint,
        UBlendSpace* RifleAimOffset, UBlendSpace* PistolAimOffset,
        bool bIncludeBlindFire = false, bool bIncludeCrouchReload = false, bool bIncludeCrouchHandling = false,
        bool bDirectionalCrouch = false, UBlendSpace* ProtectiveCoverLocomotion = nullptr,
        UBlendSpace* HighCoverLocomotion = nullptr, UAnimSequence* PistolSprint = nullptr,
        bool bIncludeRifleSupportGrip = false,
        UAnimSequence* CrouchEntry = nullptr, UAnimSequence* CrouchExit = nullptr);

    /** Repair only the generated blind-fire graph after verifying both IK chains and arm masks. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static bool RepairBlindFireReadiness(UAnimBlueprint* Blueprint);
};
