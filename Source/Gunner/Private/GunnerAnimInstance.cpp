#include "GunnerAnimInstance.h"

#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerWeaponData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGunnerAnimInstance::SetGameplayState(bool bNewAiming, bool bNewSprinting,
    bool bNewInCover, bool bNewPistol)
{
    bAiming = bNewAiming;
    bSprinting = bNewSprinting;
    bInCover = bNewInCover;
    bPistol = bNewPistol;
}

void UGunnerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // This callback runs on the game thread. The AnimGraph reads this snapshot during evaluation.
    const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
    if (!Character)
    {
        Speed = ForwardSpeed = RightSpeed = AimPitch = AimPitchNormalized = 0.f;
        bInAir = bCrouched = bAiming = bSprinting = bInCover = bPistol = false;
        UpperBodyWeight = 1.f;
        BlindFireAlpha = 0.f;
        CrouchReloadAlpha = CrouchHandlingAlpha = ProtectiveArmsWeight = 0.f;
        bCrouchReloading = false;
        bBlindFiring = bBlindFireTargetsValid = false;
        return;
    }

    const FVector Velocity = Character->GetVelocity();
    Speed = Velocity.Size2D();
    const FRotator Facing(0.f, Character->GetActorRotation().Yaw, 0.f);
    const FVector LocalVelocity = Facing.UnrotateVector(Velocity);
    ForwardSpeed = LocalVelocity.X;
    RightSpeed = LocalVelocity.Y;
    bInAir = Character->GetCharacterMovement()->IsFalling();
    bCrouched = Character->bIsCrouched;
    const float TargetPitch = FMath::Clamp(
        FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch), -60.f, 60.f);
    AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, 18.f);
    AimPitchNormalized = AimPitch / 90.f;
    const bool bUpperBodyReady = !bSprinting && !bInAir && (!bCrouched || bAiming);
    UpperBodyWeight = FMath::FInterpTo(UpperBodyWeight, bUpperBodyReady ? 1.f : 0.f, DeltaSeconds, 12.f);

    const AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Character);
    const UGunnerCombatComponent* Combat = Gunner ? Gunner->GetCombat() : nullptr;
    const UGunnerCoverComponent* Cover = Gunner ? Gunner->GetCover() : nullptr;
    const UGunnerWeaponData* Weapon = Combat ? Combat->GetWeaponData() : nullptr;
    float CoverTop = 0.f;
    const bool bBlindFireRequested = bBlindFirePoseReady && Combat && Combat->IsBlindFiring() && bCrouched && !bInAir;
    bBlindFireTargetsValid = bBlindFireRequested && Weapon && Weapon->bBlindFireGripReady && Cover &&
        Cover->GetLowCoverTop(CoverTop);
    if (bBlindFireTargetsValid)
    {
        const FVector Forward = Character->GetActorForwardVector();
        const FVector Right = Character->GetActorRightVector();
        const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f,
            Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        // The rifle's support grip is 36 cm forward of its trigger hand. Keep
        // that hand closer to the center/behind the head so both authored arm
        // lengths can reach without stretching. The pistol grip is much shorter.
        const bool bRifle = Weapon->WeaponKind == EGunnerWeaponKind::Rifle;
        FVector HandPosition = Feet + Forward * (bRifle ? -10.f : 20.f) +
            Right * (bRifle ? 12.f : 18.f);
        HandPosition.Z = CoverTop + 10.f;
        // The installed weapon meshes point along local +Y. Preserve their authored
        // hand-relative grip and derive both targets from one rigid weapon transform.
        const FQuat WeaponRotation = FRotationMatrix::MakeFromYZ(Forward, FVector::UpVector).ToQuat();
        const FQuat RightHandRotation = WeaponRotation * Weapon->GripTransform.GetRotation().Inverse();
        const FTransform RightHandWorld(RightHandRotation, HandPosition);
        const FTransform WeaponWorld = Weapon->GripTransform * RightHandWorld;
        const FTransform LeftHandWorld = Weapon->LeftHandGripTransform * WeaponWorld;
        const FTransform& MeshWorld = Character->GetMesh()->GetComponentTransform();
        const FTransform RightHandComponent = RightHandWorld.GetRelativeTransform(MeshWorld);
        const FTransform LeftHandComponent = LeftHandWorld.GetRelativeTransform(MeshWorld);
        BlindRightHandLocation = RightHandComponent.GetLocation();
        BlindLeftHandLocation = LeftHandComponent.GetLocation();
        BlindRightHandRotation = RightHandComponent.Rotator();
        BlindLeftHandRotation = LeftHandComponent.Rotator();
        FVector ElbowCenter = Feet + Forward * 3.f;
        ElbowCenter.Z = CoverTop - 20.f;
        BlindRightElbowLocation = MeshWorld.InverseTransformPosition(ElbowCenter + Right * 65.f);
        BlindLeftElbowLocation = MeshWorld.InverseTransformPosition(ElbowCenter - Right * 65.f);
        bBlindFireTargetsValid = !BlindRightHandLocation.ContainsNaN() && !BlindLeftHandLocation.ContainsNaN();
    }
    // The graph masks only arm branches. Neither this alpha nor either IK node
    // modifies the authored crouch pelvis, spine or head.
    BlindFireAlpha = FMath::FInterpConstantTo(BlindFireAlpha,
        bBlindFireTargetsValid ? 1.f : 0.f, DeltaSeconds, 5.f);
    // A reload montage supplies the complete arm motion. Its non-additive pose
    // must remain free to move the hands instead of using the blind-fire IK base.
    const bool bReloading = Combat && Combat->IsReloading();
    const bool bEquipping = Combat && Combat->GetActionState() == EGunnerCombatAction::Equipping;
    const bool bDryFiring = Combat && Combat->IsDryFiring();
    bBlindFiring = BlindFireAlpha > KINDA_SMALL_NUMBER && !bReloading && !bEquipping && !bDryFiring;
    const bool bCrouchReloadRequested = bCrouchReloadPoseReady && bCrouched && !bInAir && bReloading;
    // IsReloading remains true through the montage's authored blend-out. Fade the
    // protective arm layer only after its ended callback (or an interruption).
    CrouchReloadAlpha = FMath::FInterpConstantTo(CrouchReloadAlpha,
        bCrouchReloadRequested ? 1.f : 0.f, DeltaSeconds, 12.f);
    bCrouchReloading = CrouchReloadAlpha > KINDA_SMALL_NUMBER;
    const bool bHandling = bCrouched && !bInAir &&
        ((bCrouchEquipPoseReady && bEquipping) || (bCrouchDryFirePoseReady && bDryFiring));
    CrouchHandlingAlpha = FMath::FInterpConstantTo(CrouchHandlingAlpha, bHandling ? 1.f : 0.f, DeltaSeconds, 12.f);
    ProtectiveArmsWeight = FMath::Max3(BlindFireAlpha, CrouchReloadAlpha, CrouchHandlingAlpha);
}
