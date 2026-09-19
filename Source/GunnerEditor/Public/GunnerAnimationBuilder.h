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
    /** Sample coordinates are (forward cm/s, right cm/s, 0). Produces an editable 2D Blend Space. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static UBlendSpace* CreateDirectionalBlendSpace(const FString& PackagePath,
        USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
        const TArray<UAnimSequence*>& Clips, const TArray<FVector>& SamplePositions,
        float MaxAxisSpeed = 450.f);

    /** Null optional crouch/sprint assets omit those branches; never substitute standing crouch.
     * Crouch expects the verified forward-only gait on X=Speed, with the character facing travel. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Authoring")
    static UAnimBlueprint* CreateLocomotionBlueprint(const FString& PackagePath,
        USkeleton* Skeleton, USkeletalMesh* PreviewMesh,
        UBlendSpace* RifleLocomotion, UBlendSpace* PistolLocomotion,
        UAnimSequence* RifleFall, UAnimSequence* PistolFall,
        UBlendSpace* CrouchLocomotion, UAnimSequence* Sprint,
        UBlendSpace* RifleAimOffset, UBlendSpace* PistolAimOffset);
};
