#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GunnerMotionSettings.generated.h"

/** Authoring gates are enabled only after the corresponding animation graph is composed. */
UCLASS(BlueprintType)
class GUNNER_API UGunnerMotionSettings : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float WalkSpeed = 380.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float AimSpeed = 190.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float SprintSpeed = 650.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float CrouchSpeed = 140.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bCrouchReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bDirectionalCrouchReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bSprintReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bCoverReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float AimFOV = 58.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float AimArmLength = 220.f;
};
