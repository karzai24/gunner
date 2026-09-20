#include "Tests/GunnerCrouchReloadProbe.h"
#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerWeaponData.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AGunnerCrouchReloadProbe::AGunnerCrouchReloadProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}

void AGunnerCrouchReloadProbe::Check(bool bPass, const TCHAR* Name)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_CHECK PASS scenario=%d %s"), Scenario, Name); }
    else
    {
        ++Failures;
        UE_LOG(LogTemp, Error, TEXT("GUNNER_CROUCH_RELOAD_CHECK FAIL scenario=%d phase=%d %s"), Scenario, static_cast<int32>(Phase), Name);
        LogState(TEXT("failed_check"));
    }
}

void AGunnerCrouchReloadProbe::LogState(const TCHAR* Reason) const
{
    if (!Character) return;
    const auto* Combat = Character->GetCombat();
    const auto* Movement = Character->GetCharacterMovement();
    const auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    const auto* ActiveMontage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_STATE reason=%s probe=%s time=%.3f scenario=%d phase=%d elapsed=%.3f action=%d crouch=%d wants_crouch=%d aim=%d blind=%d blocked=%d movement=%d ammo=%d/%d shots=%d montage=%s selected_position=%.3f reload_alpha=%.3f"),
        Reason, *GetPathName(), GetWorld()->GetTimeSeconds(), Scenario, static_cast<int32>(Phase), Elapsed,
        static_cast<int32>(Combat->GetActionState()), Character->bIsCrouched, Movement->bWantsToCrouch,
        Combat->IsAiming(), Combat->IsBlindFiring(), Combat->IsCombatBlocked(), static_cast<int32>(Movement->MovementMode),
        Combat->GetMagazine(), Combat->GetReserve(), Combat->GetShotsFired(), *GetNameSafe(ActiveMontage),
        Anim && ReloadMontage ? Anim->Montage_GetPosition(ReloadMontage) : -1.f, Anim ? Anim->CrouchReloadAlpha : -1.f);
}

void AGunnerCrouchReloadProbe::HandleObservedMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != ReloadMontage) return;
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_MONTAGE_END scenario=%d phase=%d interrupted=%d montage=%s"),
        Scenario, static_cast<int32>(Phase), bInterrupted, *GetNameSafe(Montage));
    LogState(TEXT("montage_ended"));
}

void AGunnerCrouchReloadProbe::Key(FKey InputKey, bool bPressed)
{
    if (!Player) return;
    if (bPressed) HeldKeys.AddUnique(InputKey); else HeldKeys.Remove(InputKey);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void AGunnerCrouchReloadProbe::Pulse(FKey InputKey)
{
    Key(InputKey, true);
    PulseReleases.AddUnique(InputKey); // Release after a real player-input tick.
}

void AGunnerCrouchReloadProbe::Transition(EPhase Next) { Phase = Next; Elapsed = 0.f; }

void AGunnerCrouchReloadProbe::PrepareScenario()
{
    while (!HeldKeys.IsEmpty()) Key(HeldKeys.Last(), false);
    PulseReleases.Reset();
    Character->GetCombat()->StopAllActions();
    Character->GetCover()->Detach();
    Character->UnCrouch();
    Character->GetCharacterMovement()->StopMovementImmediately();
    const float Height = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
    Character->SetActorLocation(FVector(0.f, IsCoverCase() ? -850.f : -1300.f, Height),
        false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0.f, 90.f, 0.f));
    Player->SetControlRotation(FRotator(0.f, 90.f, 0.f));
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_SCENARIO %d weapon=%s context=%s"),
        Scenario, IsPistolCase() ? TEXT("pistol") : TEXT("rifle"), IsCoverCase() ? TEXT("low_cover") : TEXT("free_crouch"));
    Transition(EPhase::Settle);
}

void AGunnerCrouchReloadProbe::BeginReload(bool bCancel)
{
    auto* Combat = Character->GetCombat();
    ReloadMontage = Combat->GetWeaponData() ? Combat->GetWeaponData()->ReloadMontage : nullptr;
    Check(ReloadMontage != nullptr, TEXT("Selected weapon has an authored reload montage"));
    if (!ReloadMontage) { Finish(); return; }
    ReloadDuration = ReloadMontage->GetPlayLength() / FMath::Max(0.01f, ReloadMontage->RateScale);
    Check(ReloadMontage->SlotAnimTracks.ContainsByPredicate([](const FSlotAnimationTrack& Track)
        { return Track.SlotName == FName(TEXT("UpperBody")); }), TEXT("Reload is authored on the graph's UpperBody slot"));
    MagazineBefore = LastMagazine = Combat->GetMagazine();
    ReserveBefore = LastReserve = Combat->GetReserve();
    AmmoChanges = 0;
    MaxHeadZ = -BIG_NUMBER;
    MaxLeftTravel = MaxRightTravel = FirstPosition = LastPosition = 0.f;
    bObservedMontage = bObservedSlot = bSampledHands = bEarlyTransfer = bToggleAttempted = false;
    bCapturedEarly = bCapturedMid = false;
    bCrouchPreserved = true;
    Pulse(EKeys::R);
    Key(EKeys::LeftMouseButton, false);
    Transition(bCancel ? EPhase::CancelReload : EPhase::CompleteReload);
}

void AGunnerCrouchReloadProbe::ObserveAmmo()
{
    const auto* Combat = Character->GetCombat();
    const int32 Magazine = Combat->GetMagazine(), Reserve = Combat->GetReserve();
    if (Magazine != LastMagazine || Reserve != LastReserve)
    {
        UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_AMMO scenario=%d phase=%d elapsed=%.3f before=%d/%d after=%d/%d reloading=%d"),
            Scenario, static_cast<int32>(Phase), Elapsed, LastMagazine, LastReserve, Magazine, Reserve, Combat->IsReloading());
        ++AmmoChanges;
        if (Combat->IsReloading()) bEarlyTransfer = true;
        LastMagazine = Magazine; LastReserve = Reserve;
    }
}

void AGunnerCrouchReloadProbe::ObserveReload()
{
    ObserveAmmo();
    const auto* Combat = Character->GetCombat();
    auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    if (!Anim) return;
    if (Anim->Montage_IsPlaying(ReloadMontage))
    {
        const float Position = Anim->Montage_GetPosition(ReloadMontage);
        if (!bObservedMontage) FirstPosition = Position;
        LastPosition = Position;
        bObservedMontage = true;
        bObservedSlot |= Anim->IsSlotActive(TEXT("UpperBody"));
    }
    if (Elapsed > 0.35f && Combat->IsReloading())
    {
        bCrouchPreserved &= Character->bIsCrouched &&
            FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 62.f) &&
            !Combat->IsAiming();
        MaxHeadZ = FMath::Max(MaxHeadZ, Character->GetMesh()->GetSocketLocation(TEXT("head")).Z);
    }
    // Exclude the initial blend and compare hand motion in mesh space so actor movement cannot pass it.
    if (Combat->IsReloading() && Anim->CrouchReloadAlpha > 0.9f && Elapsed > 0.35f)
    {
        const FTransform MeshTransform = Character->GetMesh()->GetComponentTransform();
        const FVector Left = MeshTransform.InverseTransformPosition(Character->GetMesh()->GetSocketLocation(TEXT("hand_l")));
        const FVector Right = MeshTransform.InverseTransformPosition(Character->GetMesh()->GetSocketLocation(TEXT("hand_r")));
        if (!bSampledHands) { FirstLeft = Left; FirstRight = Right; bSampledHands = true; }
        MaxLeftTravel = FMath::Max(MaxLeftTravel, FVector::Distance(Left, FirstLeft));
        MaxRightTravel = FMath::Max(MaxRightTravel, FVector::Distance(Right, FirstRight));
        if (Phase == EPhase::CompleteReload && !bCapturedEarly)
        {
            Capture(TEXT("early")); bCapturedEarly = true;
        }
        else if (Phase == EPhase::CompleteReload && !bCapturedMid && Elapsed > ReloadDuration * 0.5f)
        {
            Capture(TEXT("mid")); bCapturedMid = true;
        }
    }
    if (!bToggleAttempted && Elapsed > 0.2f && Combat->IsReloading())
    {
        Pulse(EKeys::C); bToggleAttempted = true;
    }
}

void AGunnerCrouchReloadProbe::CheckReloadPresentation()
{
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_POSE scenario=%d head_max=%.2f cover_top=%.2f left_travel=%.2f right_travel=%.2f montage_progress=%.3f ammo_changes=%d"),
        Scenario, MaxHeadZ, CoverTop, MaxLeftTravel, MaxRightTravel, LastPosition - FirstPosition, AmmoChanges);
    Check(bObservedMontage && bObservedSlot && LastPosition > FirstPosition + 0.2f,
        TEXT("Actual selected reload montage advances through the active UpperBody slot"));
    Check(bSampledHands && MaxLeftTravel > 6.f,
        TEXT("Evaluated reload hand moves through the handling animation, not a frozen grip"));
    Check(bCrouchPreserved && bToggleAttempted,
        TEXT("Reload preserves crouch capsule and blocks stance toggling until completion"));
    if (IsCoverCase()) Check(MaxHeadZ > 0.f && MaxHeadZ + 5.f < CoverTop,
        TEXT("Reload head remains below the real low-cover top throughout sampled crouch"));
}

void AGunnerCrouchReloadProbe::BeginMove()
{
    MoveStart = Character->GetActorLocation();
    Key(EKeys::D, true);
    Transition(EPhase::Move);
}

void AGunnerCrouchReloadProbe::Capture(const TCHAR* Moment)
{
    const FString Name = FString::Printf(TEXT("crouch_reload_%s_%s_%s.png"),
        IsPistolCase() ? TEXT("pistol") : TEXT("rifle"), IsCoverCase() ? TEXT("cover") : TEXT("free"), Moment);
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name, false, false);
}

void AGunnerCrouchReloadProbe::Cleanup()
{
    while (!HeldKeys.IsEmpty()) Key(HeldKeys.Last(), false);
    PulseReleases.Reset();
    if (Character)
    {
        if (auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance()))
        {
            Anim->OnMontageEnded.RemoveDynamic(this, &AGunnerCrouchReloadProbe::HandleObservedMontageEnded);
            Anim->bCrouchReloadPoseReady = bOriginalReadiness;
        }
        Character->GetCombat()->StopAllActions();
        Character->GetCover()->Detach();
    }
    InputGuard.Restore();
}

void AGunnerCrouchReloadProbe::Finish()
{
    Cleanup();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CROUCH_RELOAD_COMPLETE failures=%d scenarios=%d"), Failures, FMath::Min(Scenario, 4));
    SetActorTickEnabled(false);
}

void AGunnerCrouchReloadProbe::EndPlay(const EEndPlayReason::Type Reason) { Cleanup(); Super::EndPlay(Reason); }

void AGunnerCrouchReloadProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    while (!PulseReleases.IsEmpty()) Key(PulseReleases.Pop(), false);
    Elapsed += DeltaSeconds;
    if (Phase == EPhase::Initialize)
    {
        InputGuard.Begin(GetWorld());
        if (Elapsed < 3.f) return;
        Player = GetWorld()->GetFirstPlayerController(); // Explicit single-player diagnostic.
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Check(Character != nullptr, TEXT("Motion pawn possessed"));
        if (!Character) { Finish(); return; }
        Player->FlushPressedKeys(); // Clear hardware state captured before the viewport guard; inject only afterward.
        auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
        bOriginalReadiness = Anim && Anim->bCrouchReloadPoseReady;
        Check(bOriginalReadiness, TEXT("Authored protective crouch-reload graph is active"));
        if (!bOriginalReadiness) { Finish(); return; }
        Anim->OnMontageEnded.AddUniqueDynamic(this, &AGunnerCrouchReloadProbe::HandleObservedMontageEnded);
        PrepareScenario(); return;
    }
    auto* Combat = Character->GetCombat();
    auto* Cover = Character->GetCover();
    auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    if (!Anim || !Player || Player->GetPawn() != Character)
    {
        Check(false, TEXT("Diagnostic retained its possessed animated pawn")); Finish(); return;
    }
    if (Elapsed > FMath::Max(8.f, ReloadDuration + 3.f))
    {
        Check(false, TEXT("Scenario phase completed before timeout")); Finish(); return;
    }
    switch (Phase)
    {
    case EPhase::Settle:
        if (Elapsed < 0.4f) return;
        Pulse(IsPistolCase() ? EKeys::Two : EKeys::One);
        Transition(EPhase::Equip); break;
    case EPhase::Equip:
        if (Elapsed < 0.35f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        Check(Combat->GetWeaponKind() == (IsPistolCase() ? EGunnerWeaponKind::Pistol : EGunnerWeaponKind::Rifle),
            TEXT("Expected weapon equipped through input"));
        SpendCount = 0; ShotsBefore = Combat->GetShotsFired();
        Pulse(EKeys::LeftMouseButton); Transition(EPhase::Spend); break;
    case EPhase::Spend:
        if (Elapsed < 0.35f) return;
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Input shot creates reloadable magazine deficit"));
        if (++SpendCount < 2)
        {
            ShotsBefore = Combat->GetShotsFired(); Pulse(EKeys::LeftMouseButton); Transition(EPhase::Spend);
        }
        else { Pulse(IsCoverCase() ? EKeys::SpaceBar : EKeys::C); Transition(EPhase::Stance); }
        break;
    case EPhase::Stance:
        if (Elapsed < 0.6f) return;
        Check(Character->bIsCrouched && (Cover->IsLowCover() == IsCoverCase()), TEXT("Requested protected crouch context is established"));
        if (IsCoverCase()) Check(Cover->GetLowCoverTop(CoverTop) && FMath::IsNearlyEqual(CoverTop, 115.f, 1.f),
            TEXT("Fixture exposes its actual 115 cm cover top"));
        MagazineBefore = Combat->GetMagazine(); ReserveBefore = Combat->GetReserve();
        Anim->bCrouchReloadPoseReady = false;
        Pulse(EKeys::R); Transition(EPhase::ReadinessGate); break;
    case EPhase::ReadinessGate:
        if (Elapsed < 0.3f) return;
        Check(!Combat->IsReloading() && Combat->GetMagazine() == MagazineBefore && Combat->GetReserve() == ReserveBefore,
            TEXT("Graph without crouch-reload readiness rejects the action without ammo transfer"));
        Anim->bCrouchReloadPoseReady = bOriginalReadiness;
        if (IsCoverCase())
        {
            ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Transition(EPhase::RaiseBlind);
        }
        else BeginReload(true);
        break;
    case EPhase::RaiseBlind:
        if (Elapsed < 0.7f) return;
        Check(Combat->IsBlindFiring() && Combat->GetShotsFired() > ShotsBefore, TEXT("Low-cover blind fire is active before reload handoff"));
        BeginReload(true); break;
    case EPhase::CancelReload:
        ObserveReload();
        if (Elapsed < FMath::Max(0.6f, ReloadDuration * 0.4f)) return;
        Check(Combat->IsReloading() && Anim->bCrouchReloading && Anim->CrouchReloadAlpha > 0.9f,
            TEXT("Crouched reload activates its visible arm layer"));
        Check(!Combat->IsBlindFiring() && Anim->BlindFireAlpha < 0.05f,
            TEXT("Reload clears blind-fire arm IK instead of pinning the handling pose"));
        Check(AmmoChanges == 0 && !bEarlyTransfer, TEXT("Incomplete reload has not moved ammunition"));
        Check(Character->bIsCrouched && bCrouchPreserved, TEXT("Crouch input cannot stand during reload"));
        // Deliberately interrupt the actual montage: exercise its cancellation delegate, not a direct ammo reset.
        Anim->Montage_Stop(0.1f, ReloadMontage);
        Transition(EPhase::CancelSettled); break;
    case EPhase::CancelSettled:
        ObserveAmmo();
        if (Elapsed < ReloadDuration + 0.6f) return;
        Check(!Combat->IsReloading() && AmmoChanges == 0 && Combat->GetMagazine() == MagazineBefore && Combat->GetReserve() == ReserveBefore,
            TEXT("Interrupted reload and stale completion/timeout never transfer ammunition"));
        Check(Anim->CrouchReloadAlpha < 0.05f, TEXT("Interrupted reload releases the protective arm layer"));
        if (IsCoverCase()) { Key(EKeys::RightMouseButton, true); Transition(EPhase::AimBeforeReload); }
        else BeginReload(false);
        break;
    case EPhase::AimBeforeReload:
        if (Elapsed < 0.6f) return;
        Check(Combat->IsAiming() && !Character->bIsCrouched && Cover->IsLowCover(), TEXT("Held ADS exposes low-cover pawn before reload"));
        BeginReload(false); break;
    case EPhase::CompleteReload:
        ObserveReload();
        if (Combat->IsReloading() || Elapsed < 0.3f) return;
        CheckReloadPresentation();
        Check(Combat->GetWeaponData() && Combat->GetMagazine() == Combat->GetWeaponData()->MagazineCapacity &&
            Combat->GetMagazine() + Combat->GetReserve() == MagazineBefore + ReserveBefore && AmmoChanges == 1 && !bEarlyTransfer,
            TEXT("Uninterrupted montage completion commits exactly one conserved ammo transfer"));
        Transition(EPhase::CompletionSettled); break;
    case EPhase::CompletionSettled:
        ObserveAmmo();
        if (Elapsed < 0.75f) return;
        Check(AmmoChanges == 1 && Combat->GetMagazine() == LastMagazine && Combat->GetReserve() == LastReserve,
            TEXT("Completed reload does not commit a second transfer"));
        Check(Anim->CrouchReloadAlpha < 0.05f, TEXT("Completed reload releases its arm layer"));
        if (IsCoverCase()) Check(Combat->IsAiming() && !Character->bIsCrouched && Cover->IsLowCover(),
            TEXT("Held ADS resumes standing cover aim only after reload completion"));
        Pulse(EKeys::R); Transition(EPhase::FullMagazine); break;
    case EPhase::FullMagazine:
        ObserveAmmo();
        if (Elapsed < 0.3f) return;
        Check(!Combat->IsReloading() && !Anim->Montage_IsActive(ReloadMontage) && AmmoChanges == 1,
            TEXT("Full magazine reload input is a no-op"));
        Key(EKeys::RightMouseButton, false); Transition(EPhase::ReleaseAim); break;
    case EPhase::ReleaseAim:
        if (Elapsed < 0.45f) return;
        if (IsCoverCase()) BeginMove();
        else { Pulse(EKeys::C); Transition(EPhase::ToggleStand); }
        break;
    case EPhase::ToggleStand:
        if (Elapsed < 0.4f) return;
        Check(!Character->bIsCrouched, TEXT("Crouch toggle can stand again after reload"));
        Pulse(EKeys::C); Transition(EPhase::ToggleCrouch); break;
    case EPhase::ToggleCrouch:
        if (Elapsed < 0.4f) return;
        Check(Character->bIsCrouched, TEXT("Crouch toggle can crouch again after reload"));
        BeginMove(); break;
    case EPhase::Move:
        if (Elapsed < 0.35f) return;
        Key(EKeys::D, false);
        Check(Character->bIsCrouched && FVector::Dist2D(MoveStart, Character->GetActorLocation()) > 5.f,
            TEXT("Crouched travel resumes after reload"));
        Key(EKeys::RightMouseButton, true); Transition(EPhase::AimAfterReload); break;
    case EPhase::AimAfterReload:
        if (Elapsed < 0.6f) return;
        Check(Combat->IsAiming() && (Character->bIsCrouched != IsCoverCase()), TEXT("Aim input works after reload in its intended free/cover stance"));
        Key(EKeys::RightMouseButton, false);
        if (++Scenario == 4) Finish(); else PrepareScenario();
        break;
    default: Check(false, TEXT("Unexpected diagnostic phase")); Finish(); break;
    }
#endif
}
