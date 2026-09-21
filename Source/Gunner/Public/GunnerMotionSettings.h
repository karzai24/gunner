#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GunnerMotionSettings.generated.h"
class UAnimInstance;

/** Authoring gates are enabled only after the corresponding animation graph is composed. */
UCLASS(BlueprintType)
class GUNNER_API UGunnerMotionSettings : public UDataAsset
{
    GENERATED_BODY()
public:
    /** Optional local profile composition; the public character keeps its portable base graph. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation") TSubclassOf<UAnimInstance> AnimationClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float WalkSpeed = 380.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float AimSpeed = 190.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float SprintSpeed = 650.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float CrouchSpeed = 140.f;
    /** Optional cover-shooter controls; the original foundation keeps its jump binding. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") bool bContextualTraversal = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement", meta=(ClampMin="60", ClampMax="360")) float SprintTurnRate = 160.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0.1", ClampMax="0.5")) float TraverseHoldTime = 0.18f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float FastCoverSpeed = 220.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float Acceleration = 1200.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") float Braking = 1600.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bCrouchReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bDirectionalCrouchReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bSprintReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation gates") bool bCoverReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float AimFOV = 58.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float AimArmLength = 220.f;
};
