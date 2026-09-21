#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GunnerAnimInstance.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class EGunnerCrouchTransitionPhase : uint8
{
    None,
    Entering,
    Exiting
};

/** Presentation snapshot for the editable Warden animation graph. Gameplay remains authoritative. */
UCLASS(Transient, Blueprintable)
class GUNNER_API UGunnerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    /** Supplied by the owning character; these flags never initiate gameplay actions. */
    UFUNCTION(BlueprintCallable, Category="Gunner|Animation")
    void SetGameplayState(bool bNewAiming, bool bNewSprinting, bool bNewInCover, bool bNewPistol);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    float Speed = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    float ForwardSpeed = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    float RightSpeed = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    bool bInAir = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    bool bCrouched = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    bool bSprinting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    bool bInCover = false;

    /** Enabled only by a graph using authored directional crouch poses on both velocity axes. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
    bool bDirectionalCrouchPoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    bool bProtectiveLowCover = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
    float CrouchReferenceSpeed = 140.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
    float CrouchPlayRate = 1.f;

    /** Cosmetic stationary stance changes; capsule and movement remain gameplay-owned. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Transition")
    bool bCrouchTransitionPoseReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Transition")
    float CrouchEntryLength = 0.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Transition")
    float CrouchExitLength = 0.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Transition", meta=(ClampMin="1", ClampMax="5"))
    float CrouchTransitionPlayRate = 2.5f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Transition")
    EGunnerCrouchTransitionPhase CrouchTransitionPhase = EGunnerCrouchTransitionPhase::None;
    /** Source seconds, held at the last sampled time during blend-out. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Transition")
    float CrouchTransitionTime = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Transition")
    float CrouchTransitionAlpha = 0.f;
    /** Arm-only carry through free/high crouch and its authored stance transitions. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Transition")
    float CrouchWeaponCarryAlpha = 0.f;
    /** Retains the chosen evaluator until its blend-out has completed. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Transition")
    bool bCrouchTransitionEntering = false;

    /** Local graph gates: armed wall poses and support-hand correction are authored together. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cover") bool bHighCoverPoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cover") bool bHighCoverPose = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cover") FVector HighCoverRootOffset = FVector::ZeroVector;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon") bool bRifleSupportGripPoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon") bool bRifleSupportGrip = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon") FVector RifleSupportGripLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon") FRotator RifleSupportGripRotation = FRotator::ZeroRotator;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion") float RifleSprintReferenceSpeed = 250.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion") float HighCoverReferenceSpeed = 110.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion") float RifleSprintPlayRate = 1.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion") float HighCoverPlayRate = 1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    bool bAiming = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    bool bPistol = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    float AimPitch = 0.f;

    /** Epic template aim offsets encode a +/-90 degree pitch as the normalized Y axis. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    float AimPitchNormalized = 0.f;

    /** Preserve the full authored sprint/protective crouch; crouched ADS deliberately exposes the armed torso. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    float UpperBodyWeight = 1.f;

    /** Enabled only by a graph which preserves the crouch body beneath reload arms. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Reload")
    bool bCrouchReloadPoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Reload")
    bool bCrouchReloading = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Reload")
    float CrouchReloadAlpha = 0.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Handling")
    bool bCrouchEquipPoseReady = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch Handling")
    bool bCrouchDryFirePoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Handling")
    float CrouchHandlingAlpha = 0.f;
    /** Shared arm mask; its source remains the existing UpperBody action slot. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch Reload")
    float ProtectiveArmsWeight = 0.f;

    /** Enabled only on the validated graph with the protective arm IK branch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blind Fire")
    bool bBlindFirePoseReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    bool bBlindFiring = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    bool bBlindFireTargetsValid = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    float BlindFireAlpha = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FVector BlindRightHandLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FVector BlindLeftHandLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FVector BlindRightElbowLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FVector BlindLeftElbowLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FRotator BlindRightHandRotation = FRotator::ZeroRotator;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blind Fire")
    FRotator BlindLeftHandRotation = FRotator::ZeroRotator;

private:
    void ResetCrouchTransition();
    void UpdateCrouchTransition(ACharacter* Character, bool bCanPresent, float DeltaSeconds);
    TWeakObjectPtr<ACharacter> CrouchTransitionOwner;
    bool bPreviousCrouched = false;
};
