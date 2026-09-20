#include "GunnerCombatComponent.h"

#include "GunnerTarget.h"
#include "GunnerCoverComponent.h"
#include "GunnerAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UGunnerCombatComponent::UGunnerCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGunnerCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    Cover = Character->FindComponentByClass<UGunnerCoverComponent>();

    const UGunnerWeaponData* Definitions[2] = { RifleData, PistolData };
    for (int32 Slot = 0; Slot < 2; ++Slot)
    {
        if (Definitions[Slot])
        {
            Magazines[Slot] = FMath::Clamp(Definitions[Slot]->MagazineCapacity, 1, 200);
            Reserves[Slot] = FMath::Max(0, Definitions[Slot]->InitialReserve);
        }
    }
    ActiveData = RifleData ? RifleData : PistolData;
    ActiveSlot = RifleData ? 0 : 1;
    WeaponVisual = NewObject<UStaticMeshComponent>(Character, TEXT("GunnerWeaponVisual"));
    Character->AddInstanceComponent(WeaponVisual);
    WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponVisual->SetCanEverAffectNavigation(false);
    WeaponVisual->RegisterComponent();
    UpdateWeaponVisual();
}

void UGunnerCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAllActions();
    if (WeaponVisual) WeaponVisual->DestroyComponent();
    Super::EndPlay(EndPlayReason);
}

bool UGunnerCombatComponent::HasCombatAuthority() const
{
    return Character && Character->HasAuthority();
}

bool UGunnerCombatComponent::CanStartAction() const
{
    return HasCombatAuthority() && ActiveData && !bCombatBlocked && Character->GetController() &&
        Character->GetCharacterMovement()->IsMovingOnGround() &&
        (ActionState == EGunnerCombatAction::Idle || ActionState == EGunnerCombatAction::Firing);
}

UAnimInstance* UGunnerCombatComponent::GetAnimInstance() const
{
    return Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
}

EGunnerWeaponKind UGunnerCombatComponent::GetWeaponKind() const
{
    return ActiveData ? ActiveData->WeaponKind : EGunnerWeaponKind::Rifle;
}

int32 UGunnerCombatComponent::GetMagazine() const { return Magazines[ActiveSlot]; }
int32 UGunnerCombatComponent::GetReserve() const { return Reserves[ActiveSlot]; }

void UGunnerCombatComponent::StartAim()
{
    if (bBlindFiring) StopFire();
    if (CanStartAction() && !bFireBlocked && ActiveData->GunMesh) bAiming = true;
}

void UGunnerCombatComponent::StopAim() { bAiming = false; }

void UGunnerCombatComponent::SetCombatBlocked(bool bBlocked)
{
    if (bCombatBlocked == bBlocked) return;
    bCombatBlocked = bBlocked;
    if (bBlocked) StopAllActions();
}

void UGunnerCombatComponent::SetFireBlocked(bool bBlocked)
{
    if (bFireBlocked == bBlocked) return;
    bFireBlocked = bBlocked;
    if (bBlocked)
    {
        StopFire();
        StopAim();
    }
}

void UGunnerCombatComponent::StartFire()
{
    if (!CanStartAction() || bFireBlocked || !ActiveData->FireMontage || !ActiveData->GunMesh || bFireHeld) return;
    if (bBlindFirePending)
    {
        // A second press during the same raise keeps automatic fire held, without
        // restarting the pose or queuing several rounds behind one animation.
        bFireHeld = true;
        return;
    }
    if (Cover && Cover->IsLowCover() && !bAiming)
    {
        const auto* Anim = Cast<UGunnerAnimInstance>(GetAnimInstance());
        float CoverTop = 0.f;
        if (!Anim || !Anim->bBlindFirePoseReady || !Cover->GetLowCoverTop(CoverTop) || GetMagazine() <= 0)
        {
            BlindFireWaitReason = TEXT("MissingPoseOrCover");
            return;
        }
        bFireHeld = bBlindFiring = bBlindFirePending = true;
        bBlindFireTimedOut = false;
        BlindFireRaiseStartTime = BlindFireWaitSince = GetWorld()->GetTimeSeconds();
        BlindFireWaitReason = TEXT("Raising");
        ActionState = EGunnerCombatAction::Firing;
        Character->ConsumeMovementInputVector();
        Character->GetCharacterMovement()->StopMovementImmediately();
        // Active only while raising/holding blind fire. The first round waits for
        // evaluated hands and muzzle, even when the input was a brief click.
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UGunnerCombatComponent::FireOnce, 0.03f, true);
        return;
    }
    bFireHeld = true;
    ActionState = EGunnerCombatAction::Firing;
    FireOnce();
    if (bFireHeld && ActiveData->bAutomatic && GetMagazine() > 0)
    {
        GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UGunnerCombatComponent::FireOnce,
            FMath::Max(0.03f, ActiveData->FireInterval), true);
    }
}

void UGunnerCombatComponent::ReleaseFire()
{
    bFireHeld = false;
    if (!bBlindFiring || !bBlindFirePending) StopFire();
}

void UGunnerCombatComponent::StopFire()
{
    bFireHeld = false;
    bBlindFiring = bBlindFirePending = false;
    BlindFireWaitSince = -1.f;
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FireTimer);
    if (ActionState == EGunnerCombatAction::Firing) ActionState = EGunnerCombatAction::Idle;
}

void UGunnerCombatComponent::WaitForBlindFirePose(FName Reason)
{
    BlindFireWaitReason = Reason;
    const float Now = GetWorld()->GetTimeSeconds();
    if (BlindFireWaitSince < 0.f) BlindFireWaitSince = Now;
    if (Now - BlindFireWaitSince >= 0.9f)
    {
        bBlindFireTimedOut = true;
        StopFire();
    }
}

bool UGunnerCombatComponent::IsBlindFirePoseReady(float CoverTop)
{
    const auto* Anim = Cast<UGunnerAnimInstance>(GetAnimInstance());
    const auto* Mesh = Character->GetMesh();
    if (!Character->bIsCrouched || !Anim || !Anim->bBlindFireTargetsValid || Anim->BlindFireAlpha < 0.95f)
    {
        WaitForBlindFirePose(TEXT("Raising"));
        return false;
    }
    if (!Mesh->DoesSocketExist(TEXT("hand_r")) || !Mesh->DoesSocketExist(TEXT("hand_l")) || !Mesh->DoesSocketExist(TEXT("head")))
    {
        WaitForBlindFirePose(TEXT("MissingPoseBones"));
        return false;
    }
    const FVector Hand = Mesh->GetSocketLocation(TEXT("hand_r"));
    const FVector LeftHand = Mesh->GetSocketLocation(TEXT("hand_l"));
    const FVector LeftTarget = Mesh->GetComponentTransform().TransformPosition(Anim->BlindLeftHandLocation);
    if (Hand.Z <= CoverTop + 4.f || GetMuzzleLocation().Z <= CoverTop + 4.f)
    {
        WaitForBlindFirePose(TEXT("WeaponBelowCover"));
        return false;
    }
    if (Mesh->GetSocketLocation(TEXT("head")).Z > CoverTop - 5.f)
    {
        WaitForBlindFirePose(TEXT("HeadExposed"));
        return false;
    }
    if (FVector::DistSquared(LeftHand, LeftTarget) > FMath::Square(5.f))
    {
        WaitForBlindFirePose(TEXT("SupportHandNotReady"));
        return false;
    }
    const FVector ViewForward = FRotator(0.f, Character->GetController()->GetControlRotation().Yaw, 0.f).Vector();
    if (FVector::DotProduct(Character->GetActorForwardVector(), ViewForward) < 0.866f ||
        FVector::DotProduct(ViewForward, -Cover->GetNormal()) < 0.5f)
    {
        WaitForBlindFirePose(TEXT("FacingAwayFromCover"));
        return false;
    }
    return true;
}

void UGunnerCombatComponent::FireOnce()
{
    if (!CanStartAction() || bFireBlocked || (!bFireHeld && !bBlindFirePending) || !ActiveData->FireMontage || GetMagazine() <= 0)
    {
        StopFire();
        return;
    }
    const float Now = GetWorld()->GetTimeSeconds();
    const bool bBlindShot = bBlindFiring;
    if (bBlindShot)
    {
        const auto* BlindAnim = Cast<UGunnerAnimInstance>(GetAnimInstance());
        float CoverTop = 0.f;
        if (!Cover || !Cover->IsLowCover() || bAiming || !BlindAnim || !BlindAnim->bBlindFirePoseReady ||
            !Cover->GetLowCoverTop(CoverTop))
        {
            BlindFireWaitReason = TEXT("CoverOrPoseLost");
            StopFire();
            return;
        }
        if (!IsBlindFirePoseReady(CoverTop)) return;
        // Keep validating the held pistol's context, but never repeat its shot.
        if (!ActiveData->bAutomatic && !bBlindFirePending)
        {
            BlindFireWaitSince = -1.f;
            BlindFireWaitReason = TEXT("Ready");
            return;
        }
    }
    if (Now - LastFireTime + KINDA_SMALL_NUMBER < FMath::Max(0.03f, ActiveData->FireInterval)) return;
    UAnimInstance* Anim = GetAnimInstance();
    if (!Anim)
    {
        StopFire();
        return;
    }
    FVector CameraLocation;
    FRotator CameraRotation;
    Character->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
    const FVector ViewDirection = CameraRotation.Vector();
    const float ShotRange = FMath::Clamp(ActiveData->Range, 100.f, 50000.f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerShot), true, Character);
    FHitResult CameraHit;
    const FVector CameraEnd = CameraLocation + ViewDirection * ShotRange;
    const bool bCameraHit = GetWorld()->LineTraceSingleByChannel(CameraHit, CameraLocation, CameraEnd,
        ECC_Visibility, Query);
    const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : CameraEnd;
    const FVector Muzzle = GetMuzzleLocation();
    const FVector Body = Character->GetMesh()->DoesSocketExist(TEXT("spine_03"))
        ? Character->GetMesh()->GetSocketLocation(TEXT("spine_03")) : Character->GetActorLocation();

    if (bBlindShot)
    {
        const FVector Hand = Character->GetMesh()->GetSocketLocation(TEXT("hand_r"));
        FHitResult RaisedPathHit;
        // Follow the real raised arm/barrel route. A direct torso-to-muzzle ray
        // would cut diagonally through the cover despite the grip clearing it.
        if (GetWorld()->LineTraceSingleByChannel(RaisedPathHit, Body, Hand, ECC_Visibility, Query) ||
            GetWorld()->LineTraceSingleByChannel(RaisedPathHit, Hand, Muzzle, ECC_Visibility, Query))
        {
            WaitForBlindFirePose(TEXT("RaisedWeaponPathBlocked"));
            return;
        }
        if (FVector::DotProduct(AimPoint - Muzzle, ViewDirection) <= 0.f ||
            FVector::DotProduct((AimPoint - Muzzle).GetSafeNormal2D(), -Cover->GetNormal()) < 0.5f)
        {
            WaitForBlindFirePose(TEXT("AimBlockedByCover"));
            return;
        }
        BlindFireWaitSince = -1.f;
        BlindFireWaitReason = TEXT("Ready");
    }
    if (Anim->Montage_Play(ActiveData->FireMontage, 1.f) <= 0.f)
    {
        StopFire();
        return;
    }
    LastFireTime = Now;
    --Magazines[ActiveSlot];
    ++ShotsFired;
    bLastShotObstructed = false;
    if (bBlindShot) bBlindFirePending = false;

    // A protruding barrel must never originate a shot through a wall or behind the shooter.
    FHitResult BodyHit;
    if (!bBlindShot && (GetWorld()->LineTraceSingleByChannel(BodyHit, Body, Muzzle, ECC_Visibility, Query) ||
        FVector::DotProduct(AimPoint - Muzzle, ViewDirection) <= 0.f))
    {
        bLastShotObstructed = true;
        DrawShot(Body, BodyHit.bBlockingHit ? BodyHit.ImpactPoint : Muzzle, false);
        return;
    }

    const FVector AimDirection = (AimPoint - Muzzle).GetSafeNormal();
    const FVector ShotDirection = bBlindShot ? FMath::VRandCone(AimDirection, FMath::DegreesToRadians(4.f)) : AimDirection;
    const FVector ShotEnd = Muzzle + ShotDirection * (bBlindShot ? ShotRange : FMath::Min(ShotRange, FVector::Distance(Muzzle, AimPoint) + 3.f));
    FHitResult MuzzleHit;
    const bool bMuzzleHit = GetWorld()->LineTraceSingleByChannel(MuzzleHit, Muzzle, ShotEnd, ECC_Visibility, Query);
    bool bDamaged = false;
    // Only range fixtures have a damage model in this sandbox; ordinary geometry must not
    // report a successful hit just because AActor::TakeDamage returns its input by default.
    if (bMuzzleHit && Cast<AGunnerTarget>(MuzzleHit.GetActor()))
    {
        const float Applied = UGameplayStatics::ApplyPointDamage(MuzzleHit.GetActor(), FMath::Max(0.f, ActiveData->Damage),
            ShotDirection, MuzzleHit, Character->GetController(), Character, nullptr);
        bDamaged = Applied > 0.f;
        if (bDamaged) LastHitTime = Now;
    }
    bLastShotObstructed = bMuzzleHit && !bDamaged && (!bCameraHit || MuzzleHit.GetActor() != CameraHit.GetActor());
    DrawShot(Muzzle, bMuzzleHit ? MuzzleHit.ImpactPoint : ShotEnd, bDamaged);
    if (bBlindShot && !bFireHeld) StopFire();
}

void UGunnerCombatComponent::Reload()
{
    if (!CanStartAction() || !ActiveData->ReloadMontage || GetReserve() <= 0 ||
        GetMagazine() >= FMath::Clamp(ActiveData->MagazineCapacity, 1, 200)) return;
    // The acquired reload is an upright weapon action. Protected crouch keeps its
    // authored torso, so never commit ammo for an action that cannot be displayed.
    if (Character->bIsCrouched || Character->GetCharacterMovement()->bWantsToCrouch) return;
    if (Cover && Cover->IsLowCover()) return;
    StopFire();
    StopAim();
    BeginAction(EGunnerCombatAction::Reloading, ActiveData->ReloadMontage);
}

void UGunnerCombatComponent::EquipRifle() { EquipSlot(0); }
void UGunnerCombatComponent::EquipPistol() { EquipSlot(1); }

void UGunnerCombatComponent::EquipSlot(int32 Slot)
{
    UGunnerWeaponData* NewData = Slot == 0 ? RifleData.Get() : PistolData.Get();
    if (!CanStartAction() || ActiveSlot == Slot || !NewData || !NewData->GunMesh || !NewData->EquipMontage) return;
    if (Character->bIsCrouched || Character->GetCharacterMovement()->bWantsToCrouch) return;
    if (Cover && Cover->IsLowCover()) return;
    StopFire();
    StopAim();
    if (!BeginAction(EGunnerCombatAction::Equipping, NewData->EquipMontage)) return;
    ActiveSlot = Slot;
    ActiveData = NewData;
    UpdateWeaponVisual();
}

void UGunnerCombatComponent::UpdateWeaponVisual()
{
    if (!Character || !WeaponVisual) return;
    WeaponVisual->SetStaticMesh(ActiveData ? ActiveData->GunMesh.Get() : nullptr);
    if (!ActiveData) return;
    WeaponVisual->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform,
        ActiveData->HandSocket);
    WeaponVisual->SetRelativeTransform(ActiveData->GripTransform);
}

FVector UGunnerCombatComponent::GetMuzzleLocation() const
{
    return WeaponVisual && ActiveData
        ? WeaponVisual->GetComponentTransform().TransformPosition(ActiveData->MuzzleOffset)
        : (Character ? Character->GetActorLocation() : FVector::ZeroVector);
}

bool UGunnerCombatComponent::BeginAction(EGunnerCombatAction NewAction, UAnimMontage* Montage)
{
    UAnimInstance* Anim = GetAnimInstance();
    if (!HasCombatAuthority() || !Anim || !Montage) return false;
    const float Duration = Anim->Montage_Play(Montage, 1.f, EMontagePlayReturnType::Duration);
    if (Duration <= 0.f) return false;
    ActionState = NewAction;
    ActionMontage = Montage;
    ++ActionSerial;
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UGunnerCombatComponent::HandleMontageEnded, ActionSerial);
    Anim->Montage_SetEndDelegate(EndDelegate, Montage);
    GetWorld()->GetTimerManager().SetTimer(ActionTimeoutTimer, this, &UGunnerCombatComponent::HandleActionTimeout,
        Duration + 0.5f, false);
    return true;
}

void UGunnerCombatComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ExpectedSerial)
{
    if (ExpectedSerial != ActionSerial || Montage != ActionMontage) return;
    FinishAction(!bInterrupted);
}

void UGunnerCombatComponent::HandleActionTimeout()
{
    // A missing/endless/interrupted animation must never silently complete reload or retain a lock.
    StopAllActions();
}

void UGunnerCombatComponent::FinishAction(bool bCompleted)
{
    if (bCompleted && HasCombatAuthority() && Character->GetController() && !bCombatBlocked && IsReloading() && ActiveData)
    {
        const int32 Needed = FMath::Max(0, FMath::Clamp(ActiveData->MagazineCapacity, 1, 200) - GetMagazine());
        const int32 Transfer = FMath::Min(Needed, GetReserve());
        Magazines[ActiveSlot] += Transfer;
        Reserves[ActiveSlot] -= Transfer;
    }
    GetWorld()->GetTimerManager().ClearTimer(ActionTimeoutTimer);
    GetWorld()->GetTimerManager().ClearTimer(MeleeImpactTimer);
    ActionMontage = nullptr;
    ActionState = EGunnerCombatAction::Idle;
    ++ActionSerial;
}

void UGunnerCombatComponent::StopAllActions()
{
    StopFire();
    StopAim();
    UAnimMontage* ToStop = ActionMontage;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ActionTimeoutTimer);
        GetWorld()->GetTimerManager().ClearTimer(MeleeImpactTimer);
    }
    ActionMontage = nullptr;
    ActionState = EGunnerCombatAction::Idle;
    ++ActionSerial;
    if (ToStop)
    {
        if (UAnimInstance* Anim = GetAnimInstance()) Anim->Montage_Stop(0.1f, ToStop);
    }
}

void UGunnerCombatComponent::Melee()
{
    // The acquired jab is a standing, planted full-body action. A crouched attack
    // requires its own clip; never stand the mesh inside a crouched capsule.
    if (!CanStartAction() || Character->bIsCrouched || Character->GetCharacterMovement()->bWantsToCrouch || !ActiveData->MeleeMontage) return;
    // Low-cover stance automatically crouches when aim stops; it cannot host the standing jab.
    if (Cover && (Cover->IsLowCover() || Cover->IsPeeking())) return;
    StopFire();
    StopAim();
    if (!BeginAction(EGunnerCombatAction::Melee, ActiveData->MeleeMontage)) return;
    Character->GetCharacterMovement()->StopMovementImmediately();
    bMeleeCommitted = false;
    const float Duration = ActionMontage->GetPlayLength() / FMath::Max(0.01f, ActionMontage->RateScale);
    GetWorld()->GetTimerManager().SetTimer(MeleeImpactTimer, this, &UGunnerCombatComponent::CommitMelee,
        FMath::Max(0.05f, Duration * FMath::Clamp(ActiveData->MeleeImpactFraction, 0.1f, 0.85f)), false);
}

void UGunnerCombatComponent::CommitMelee()
{
    UAnimInstance* Anim = GetAnimInstance();
    if (!HasCombatAuthority() || !Character->GetController() || bCombatBlocked || !IsMeleeing() || bMeleeCommitted || !ActiveData ||
        !Anim || !Anim->Montage_IsPlaying(ActionMontage)) return;
    bMeleeCommitted = true;
    const FVector Direction = Character->GetActorForwardVector();
    const FVector Start = Character->GetActorLocation() + FVector(0.f, 0.f, 15.f);
    const FVector End = Start + Direction * FMath::Clamp(ActiveData->MeleeReach, 30.f, 200.f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerMelee), false, Character);
    FHitResult Hit;
    if (!GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(FMath::Clamp(ActiveData->MeleeRadius, 5.f, 60.f)), Query) || !Cast<AGunnerTarget>(Hit.GetActor())) return;
    if (FVector::DotProduct((Hit.ImpactPoint - Start).GetSafeNormal(), Direction) < 0.55f) return;
    FHitResult Occlusion;
    if (GetWorld()->LineTraceSingleByChannel(Occlusion, Start, Hit.ImpactPoint, ECC_Visibility, Query) &&
        Occlusion.GetActor() != Hit.GetActor()) return;
    if (UGameplayStatics::ApplyPointDamage(Hit.GetActor(), FMath::Max(0.f, ActiveData->MeleeDamage), Direction,
        Hit, Character->GetController(), Character, nullptr) > 0.f)
    {
        LastHitTime = GetWorld()->GetTimeSeconds();
    }
}

void UGunnerCombatComponent::DrawShot(const FVector& Start, const FVector& End, bool bHit) const
{
    // Short-lived development cosmetics; no persistent debug geometry or per-shot actors.
    DrawDebugLine(GetWorld(), Start, End, bHit ? FColor(130, 240, 220) : FColor(255, 195, 85), false, 0.06f, 0, 1.f);
    DrawDebugPoint(GetWorld(), End, 5.f, FColor(255, 215, 140), false, 0.12f);
}
