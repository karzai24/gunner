#include "GunnerAnimInstance.h"

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
    const bool bUpperBodyReady = !bSprinting && (!bCrouched || bAiming);
    UpperBodyWeight = FMath::FInterpTo(UpperBodyWeight, bUpperBodyReady ? 1.f : 0.f, DeltaSeconds, 12.f);
}
