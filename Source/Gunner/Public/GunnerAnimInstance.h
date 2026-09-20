#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GunnerAnimInstance.generated.h"

/** Presentation snapshot for the editable Warden animation graph. Gameplay remains authoritative. */
UCLASS(Transient, Blueprintable)
class GUNNER_API UGunnerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
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
};
