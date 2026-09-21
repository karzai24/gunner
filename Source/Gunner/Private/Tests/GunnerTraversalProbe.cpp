#include "Tests/GunnerTraversalProbe.h"
#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerDodgeComponent.h"
#include "GunnerMotionSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"

namespace
{
    constexpr float FloorZ = 400.f;
    constexpr float StartY = -370.f;
    constexpr float TargetY = -292.f; // Wall face -240 minus the unchanged 52 cm offset.
    constexpr int32 ScenarioCount = 30;
}

AGunnerTraversalProbe::AGunnerTraversalProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}
void AGunnerTraversalProbe::Key(FKey K, bool bPressed)
{
    if (!Player) return;
    if (bPressed) Held.AddUnique(K); else Held.Remove(K);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, K, bPressed ? IE_Pressed : IE_Released,
        bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
void AGunnerTraversalProbe::Check(bool bPass, const TCHAR* Label)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_CHECK PASS scenario=%d phase=%d %s"), Scenario, Phase, Label); }
    else { ++Failures; UE_LOG(LogTemp, Error, TEXT("GUNNER_TRAVERSAL_CHECK FAIL scenario=%d phase=%d %s"), Scenario, Phase, Label); }
}
void AGunnerTraversalProbe::Go(int32 Next) { Phase = Next; Elapsed = 0.f; }
bool AGunnerTraversalProbe::RequireCapability(bool bReady, const TCHAR* Name)
{
    if (bReady) return true;
    ++Skipped;
    UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_SKIP scenario=%d unavailable=%s"), Scenario, Name);
    if (FParse::Param(FCommandLine::Get(), TEXT("GunnerRequireTraversalCapabilities")))
        Check(false, TEXT("Final acceptance requires installed directional crouch and contextual controls"));
    return false;
}
FKey AGunnerTraversalProbe::DirectionKey() const
{
    switch (DirectionStep) { case 0: return EKeys::W; case 1: return EKeys::S; case 2: return EKeys::A; default: return EKeys::D; }
}
void AGunnerTraversalProbe::Capture(const TCHAR* Moment)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/
        FString::Printf(TEXT("traversal_%d_%s.png"), Scenario, Moment), false, false);
}
AStaticMeshActor* AGunnerTraversalProbe::SpawnBox(const FVector& Location, const FVector& Size, bool bStatic)
{
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* Box = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
    if (!Box) return nullptr;
    auto* Mesh = Box->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(Cube);
    Mesh->SetWorldScale3D(Size / 100.f);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    if (bStatic) Mesh->SetMobility(EComponentMobility::Static);
    Fixtures.Add(Box);
    return Box;
}
void AGunnerTraversalProbe::Prepare()
{
    RestoreSettings();
    while (!Held.IsEmpty()) Key(Held.Last(), false);
    Character->GetCover()->Detach();
    if (ExternalSourceId) Character->GetCharacterMovement()->RemoveRootMotionSourceByID(ExternalSourceId);
    ExternalSourceId = 0;
    Character->GetDodge()->CancelDodge();
    Character->GetCombat()->StopAllActions();
    Character->StopJumpPresentation();
    Character->UnCrouch();
    for (const auto& Fixture : Fixtures) if (Fixture) Fixture->Destroy();
    Fixtures.Reset(); Wall = nullptr;
    // Elevated fixtures make missing support real even though the preserved room
    // has a continuous foundation floor far below. Nothing is saved into the map.
    const bool bGap = Scenario == 4;
    const bool bOpen = Scenario == 16 || Scenario == 17 || Scenario == 20 || Scenario == 21 || Scenario == 26 || Scenario == 28;
    SpawnBox(FVector(0.f, bGap ? -517.5f : -400.f, FloorZ - 10.f),
        FVector(bOpen ? 1600.f : 800.f, bOpen ? 1600.f : (bGap ? 365.f : 600.f), 20.f));
    const bool bLow = Scenario == 2 || Scenario == 10 || Scenario == 14 || Scenario == 15 || Scenario == 19 || Scenario == 29;
    const float Height = bLow ? 115.f : 180.f;
    if (!bOpen) Wall = SpawnBox(FVector(0.f, -220.f, FloorZ + Height * .5f), FVector(400.f, 40.f, Height));
    if (Scenario == 3)
        SpawnBox(FVector(60.f, TargetY, FloorZ + 90.f), FVector(60.f, 30.f, 180.f));
    auto* Movement = Character->GetCharacterMovement();
    Movement->StopMovementImmediately();
    Character->SetActorLocation(FVector((Scenario == 18 || Scenario == 23) ? -160.f : 0.f, StartY, FloorZ + Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
        false, nullptr, ETeleportType::TeleportPhysics);
    Movement->SetMovementMode(MOVE_Walking);
    Character->SetActorRotation(FRotator(0.f, 90.f, 0.f));
    Player->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
    bObservedIntermediate = bObservedGait = bCaptured = bActionDone = false;
    bRouteSafe = true;
    bPoseSafe = bHeadingSafe = true;
    DirectionStep = 0; MaxHead = MaxHandTravel = 0.f;
    MaxFeetDrift = Total = 0.f;
    Go(1);
}
void AGunnerTraversalProbe::NextScenario()
{
    if (++Scenario == ScenarioCount) Finish(); else Prepare();
}
void AGunnerTraversalProbe::Cleanup()
{
    RestoreSettings();
    while (!Held.IsEmpty()) Key(Held.Last(), false);
    if (Character)
    {
        Character->GetCover()->Detach(); Character->GetDodge()->CancelDodge();
        Character->GetCombat()->StopAllActions(); Character->StopJumpPresentation();
        if (ExternalSourceId) Character->GetCharacterMovement()->RemoveRootMotionSourceByID(ExternalSourceId);
    }
    ExternalSourceId = 0;
    for (const auto& Fixture : Fixtures) if (Fixture) Fixture->Destroy();
    Fixtures.Reset(); Wall = nullptr;
    InputGuard.Restore();
}
void AGunnerTraversalProbe::RestoreSettings()
{
    if (bCoverReadinessOverridden && Settings) Settings->bCoverReady = bOriginalCoverReady;
    bCoverReadinessOverridden = false;
}
void AGunnerTraversalProbe::Finish()
{
    Cleanup();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_COMPLETE failures=%d scenarios=%d skipped=%d"), Failures, Scenario, Skipped);
    SetActorTickEnabled(false);
}
void AGunnerTraversalProbe::EndPlay(const EEndPlayReason::Type Reason) { Cleanup(); Super::EndPlay(Reason); }
void AGunnerTraversalProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    InputGuard.Begin(GetWorld());
    Elapsed += DeltaSeconds;
    if (Phase == 0)
    {
        if (Elapsed < 3.f) return;
        Player = GetWorld()->GetFirstPlayerController(); // Deliberately single-player fixture only.
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Check(Character != nullptr, TEXT("Possessed motion pawn exists"));
        if (!Character) { Finish(); return; }
        if (const auto* Property = FindFProperty<FObjectPropertyBase>(Character->GetClass(), TEXT("MotionSettings")))
            Settings = Cast<UGunnerMotionSettings>(Property->GetObjectPropertyValue_InContainer(Character));
        Player->FlushPressedKeys();
        Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Check(Cube != nullptr, TEXT("Transient geometry source exists"));
        if (!Cube) { Finish(); return; }
        Prepare(); return;
    }
    if (Elapsed > 8.f)
    {
        Check(false, TEXT("Phase completes before timeout")); Finish(); return;
    }
    auto* Cover = Character->GetCover();
    auto* Combat = Character->GetCombat();
    auto* Movement = Character->GetCharacterMovement();
    const FVector Location = Character->GetActorLocation();
    const float Feet = Location.Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    switch (Phase)
    {
    case 1:
        if (Elapsed < .3f) return;
        if (Scenario >= 26)
        {
            if (!RequireCapability(Anim && Anim->bCrouchTransitionPoseReady, TEXT("stationary_stance_transitions")))
            { NextScenario(); break; }
            if (Scenario == 27) Combat->EquipPistol(); else Combat->EquipRifle();
            Go(90); break;
        }
        if (Scenario == 25)
        {
            if (!RequireCapability(Settings && Settings->bContextualTraversal && Settings->bSprintReady
                && Settings->bCoverReady, TEXT("contextual_sprint_cover")))
            { NextScenario(); break; }
            bOriginalCoverReady = Settings->bCoverReady;
            bCoverReadinessOverridden = true;
            Settings->bCoverReady = false;
            Start = Location;
            Key(EKeys::W, true); Key(EKeys::LeftShift, true); Go(80); break;
        }
        if (Scenario == 22 || Scenario == 23 || Scenario == 24)
        {
            if (!RequireCapability(Anim && Anim->bHighCoverPoseReady && Anim->bRifleSupportGripPoseReady, TEXT("rifle_wall_pose")))
            { NextScenario(); break; }
            Combat->EquipRifle(); Go(Scenario == 24 ? 70 : 60); break;
        }
        if (Scenario == 21)
        {
            Key(EKeys::RightMouseButton, true); Go(50); break;
        }
        if (Scenario >= 16)
        {
            const bool bReady = Scenario == 20 ? Settings && Settings->bContextualTraversal
                : Anim && Anim->bDirectionalCrouchPoseReady && Settings && Settings->bDirectionalCrouchReady;
            if (!RequireCapability(bReady, Scenario == 20 ? TEXT("contextual_controls") : TEXT("directional_crouch")))
            { NextScenario(); break; }
            if (Scenario == 16 || Scenario == 17)
            {
                if (Scenario == 17) Combat->EquipPistol(); else Combat->EquipRifle();
                Go(20); break;
            }
            if (Scenario == 18 || Scenario == 19)
            {
                Check(Cover->TryAttach(FVector::YAxisVector), TEXT("Installed-pose fixture accepts cover approach"));
                Go(Scenario == 18 ? 30 : 35); break;
            }
            Key(EKeys::SpaceBar, true); Go(40); break;
        }
        if (Scenario == 10 && !Character->bIsCrouched)
        {
            if (!bActionDone) { bActionDone = true; Character->Crouch(); Go(1); return; }
            Check(false, TEXT("Crouched-entry fixture can enter native crouch")); NextScenario(); break;
        }
        Check(Movement->IsMovingOnGround() && FMath::Abs(Feet - FloorZ) < 5.f,
            TEXT("Fixture starts grounded at the expected floor height"));
        Start = Previous = Character->GetActorLocation();
        if (Scenario == 8)
        {
            Key(EKeys::SpaceBar, true); Go(6); break;
        }
        {
            if (Scenario == 0)
            {
                const int32 Before = Combat->GetMagazine();
                Combat->StartFire(); Combat->ReleaseFire();
                Check(Combat->GetMagazine() == Before - 1,
                    TEXT("One real setup shot makes the pending reload rejection meaningful"));
            }
            const bool bAccepted = Cover->TryAttach(FVector::YAxisVector, Scenario == 1);
            if (Scenario == 3 || Scenario == 4)
            {
                Check(!bAccepted && !Cover->IsAttached() && !Cover->IsTransitioning(),
                    TEXT("Obstructed route or unsupported destination rejects without a state change"));
                Check(FVector::Dist(Start, Character->GetActorLocation()) < .01f && !Movement->HasRootMotionSources(),
                    TEXT("Rejected admission makes no position change or movement source"));
                Go(5); break;
            }
            Check(bAccepted && Cover->IsTransitioning() && !Cover->IsAttached(),
                TEXT("Accepted approach is pending rather than an instant attachment"));
            Check(FVector::Dist(Start, Character->GetActorLocation()) < .01f,
                TEXT("Approach admission itself does not snap the character"));
            Check(!Cover->CanPeek(1.f) && Cover->ConstrainMovement(FVector::XAxisVector).IsNearlyZero(),
                TEXT("Pending approach blocks normal cover movement and peek"));
            Check(Cover->IsEnteringLowCover() == (Scenario == 2 || Scenario == 10 || Scenario == 14 || Scenario == 15),
                TEXT("Pending cover classification matches the actual fixture"));
            Check(FVector::DotProduct(Cover->GetNormal(), -FVector::YAxisVector) > .99f,
                TEXT("Pending wall normal is exposed for character facing"));
            if (!bAccepted) { NextScenario(); break; }
            if (Scenario == 0)
            {
                const int32 BeforeShots = Combat->GetShotsFired();
                const int32 BeforeMagazine = Combat->GetMagazine();
                Combat->StartAim(); Combat->StartFire(); Combat->Reload();
                Check(!Combat->IsAiming() && Combat->GetActionState() == EGunnerCombatAction::Idle
                    && Combat->GetShotsFired() == BeforeShots && Combat->GetMagazine() == BeforeMagazine,
                    TEXT("Accepted pending entry immediately rejects aim, fire and otherwise-valid reload before Character Tick"));
            }
            if (Scenario == 7)
            {
                Check(!Cover->TryAttach(FVector::YAxisVector) && !Cover->TryAttach(FVector::XAxisVector),
                    TEXT("Repeated approach requests cannot replace the active route"));
                Check(FVector::Dist(Start, Character->GetActorLocation()) < .01f,
                    TEXT("Repeated admission attempts preserve the current position"));
            }
            Go(2);
        }
        break;
    case 2:
        Total += DeltaSeconds;
        MaxFeetDrift = FMath::Max(MaxFeetDrift, FMath::Abs(Feet - FloorZ));
        if (Cover->IsTransitioning() && FVector::Dist2D(Start, Location) > 1.f)
        {
            bObservedIntermediate = true;
            bObservedGait |= Anim && Anim->Speed > 20.f;
            if (!bCaptured && (Scenario == 2 || Scenario == 10))
            { Capture(TEXT("approach")); bCaptured = true; }
        }
        bRouteSafe &= FMath::Abs(Location.X) < 2.f && Location.Y >= StartY - 2.f && Location.Y <= TargetY + 3.f;
        if (!bActionDone && Elapsed > .025f && (Scenario == 5 || Scenario == 6 || Scenario == 9 || Scenario == 12))
        {
            bActionDone = true;
            if (Scenario == 5)
            {
                SpawnBox(FVector(60.f, TargetY, FloorZ + 90.f), FVector(60.f, 30.f, 180.f), false);
                Check(Cover->IsTransitioning(), TEXT("Moving obstruction is introduced during the accepted approach"));
            }
            else if (Scenario == 6 || Scenario == 12)
            {
                if (Scenario == 12)
                {
                    TSharedPtr<FRootMotionSource_ConstantForce> External = MakeShared<FRootMotionSource_ConstantForce>();
                    External->InstanceName = TEXT("GunnerTraversalProbeUnrelated");
                    External->Priority = 10;
                    External->AccumulateMode = ERootMotionAccumulateMode::Additive;
                    External->Force = FVector::ZeroVector;
                    External->Duration = 3.f;
                    ExternalSourceId = Movement->ApplyRootMotionSource(External);
                }
                Cover->CancelTransition(); Stopped = Character->GetActorLocation();
                Check(!Cover->IsTransitioning() && !Cover->IsAttached(), TEXT("Explicit cancellation releases the pending state immediately"));
                if (Scenario == 6)
                {
                    const int32 BeforeShots = Combat->GetShotsFired();
                    Combat->StartFire(); Combat->ReleaseFire();
                    Check(Combat->GetShotsFired() == BeforeShots + 1,
                        TEXT("First fire after cancel succeeds immediately despite the prior movement-block latch"));
                    Combat->Reload();
                    Check(Combat->IsReloading(), TEXT("First reload after cancel is admitted without waiting for Character Tick"));
                    Combat->StopAllActions();
                }
                Go(4); break;
            }
            else if (Wall) Wall->Destroy();
        }
        Previous = Location;
        if (Cover->IsTransitioning()) return;
        Check(bRouteSafe, TEXT("Swept approach stays on its bounded route throughout movement"));
        if (Scenario == 5 || Scenario == 9)
        {
            Check(!Cover->IsAttached(), TEXT("Mid-route obstruction or wall destruction cannot commit attachment"));
            Check(Location.Y < TargetY - 3.f, TEXT("Blocked transition does not snap to its destination"));
            Stopped = Location; Go(4); break;
        }
        Check(Cover->IsAttached() && FMath::IsNearlyEqual(Location.Y, TargetY, 3.f),
            TEXT("Approach commits only after reaching the checked 52 cm wall offset"));
        Check(bObservedIntermediate && bObservedGait, TEXT("Rendered approach includes real intermediate movement and an evaluated gait"));
        Check(MaxFeetDrift < 5.f, TEXT("Approach preserves grounded feet without a stance-center lift"));
        Check(!Movement->HasRootMotionSources(), TEXT("Completed approach removes its owned movement source"));
        if (Scenario == 0)
        {
            NormalDuration = Total;
            // This probe ticks after Character. Recreate only its prior-frame latch
            // to exercise completion admission without depending on tick ordering.
            Combat->SetCombatBlocked(true);
            Combat->Reload();
            Check(Combat->IsReloading(), TEXT("First completed-entry reload ignores a stale interruption latch and uses live movement state"));
            Combat->StopAllActions();
        }
        if (Scenario == 1)
            Check(Total + .02f < NormalDuration, TEXT("Fast approach arrives sooner over the same checked route"));
        UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_APPROACH scenario=%d duration=%.3f feet_drift=%.3f"), Scenario, Total, MaxFeetDrift);
        Go(3); break;
    case 3:
        if (Elapsed < .5f) return;
        Check(Cover->IsAttached() && !Cover->IsTransitioning() && Movement->IsMovingOnGround(),
            TEXT("Completed anchor remains stable after transition cleanup"));
        if (Scenario == 2 || Scenario == 10 || Scenario == 14 || Scenario == 15)
            Check(Character->bIsCrouched && FMath::Abs(Feet - FloorZ) < 5.f &&
                Character->GetMesh()->GetSocketLocation(TEXT("head")).Z < FloorZ + 110.f,
                TEXT("Low-cover arrival settles into a genuine protected crouch without changing feet height"));
        Capture(TEXT("attached"));
        if (Scenario == 13 || Scenario == 14)
        {
            Start = Character->GetActorLocation();
            const bool bWasCrouched = Character->bIsCrouched;
            Check(!Character->GetDodge()->TryDodge(FVector::YAxisVector) && Cover->IsAttached()
                && Character->bIsCrouched == bWasCrouched && FVector::Dist(Start, Character->GetActorLocation()) < .01f,
                TEXT("Roll toward the wall rejects without losing cover or changing stance"));
            Check(Character->GetDodge()->TryDodge(-FVector::YAxisVector) && !Cover->IsAttached()
                && !Character->bIsCrouched && Character->GetDodge()->IsDodging(),
                TEXT("Validated roll away from high or low cover detaches with full standing clearance"));
            bCaptured = false; bObservedGait = false; Go(11); break;
        }
        if (Scenario == 15)
        {
            SpawnBox(FVector(0.f, TargetY, FloorZ + 155.f), FVector(180.f, 180.f, 20.f));
            Start = Character->GetActorLocation();
            Check(!Character->GetDodge()->TryDodge(-FVector::YAxisVector) && Cover->IsLowCover()
                && Character->bIsCrouched && FVector::Dist(Start, Character->GetActorLocation()) < .01f,
                TEXT("Low ceiling rejects crouched roll before altering stance or cover"));
            Go(12); break;
        }
        if (Scenario == 11)
        {
            auto* WallMesh = Wall->GetStaticMeshComponent();
            WallMesh->SetMobility(EComponentMobility::Movable);
            Wall->SetActorLocation(FVector(0.f, -220.f, FloorZ + 57.5f));
            WallMesh->SetWorldScale3D(FVector(4.f, .4f, 1.15f));
            WallMesh->SetMobility(EComponentMobility::Static);
            Go(7); break;
        }
        Cover->Detach();
        if (Scenario == 2)
        {
            const int32 BeforeShots = Combat->GetShotsFired();
            const int32 BeforeMagazine = Combat->GetMagazine();
            Combat->StartFire(); Combat->ReleaseFire();
            Check(Character->bIsCrouched && Combat->IsFireBlocked() && !Combat->IsBlindFiring()
                && Combat->GetActionState() == EGunnerCombatAction::Idle
                && Combat->GetShotsFired() == BeforeShots && Combat->GetMagazine() == BeforeMagazine,
                TEXT("Immediate post-detach crouched hip fire is rejected before the cached cover blocker updates"));
        }
        Stopped = Character->GetActorLocation(); Go(4); break;
    case 4:
        if (Elapsed < .5f) return;
        if (Scenario == 12 && ExternalSourceId)
        {
            Check(Movement->GetRootMotionSourceByID(ExternalSourceId).IsValid(),
                TEXT("Cancel removes only the cover source and preserves an unrelated native source"));
            Movement->RemoveRootMotionSourceByID(ExternalSourceId); ExternalSourceId = 0;
            Go(4); break;
        }
        Check(!Cover->IsTransitioning() && !Cover->IsAttached() && !Movement->HasRootMotionSources(),
            TEXT("Cancel/detach/invalidation leaves no transition or movement source"));
        Check(FVector::Dist2D(Stopped, Character->GetActorLocation()) < 1.f,
            TEXT("Transition cleanup does not resume stale displacement"));
        Check(!Movement->bConstrainToPlane, TEXT("Canceled or detached approach leaves no wall-plane lock"));
        NextScenario(); break;
    case 5:
        if (Elapsed < .25f) return;
        Check(FVector::Dist2D(Start, Character->GetActorLocation()) < 1.f && !Cover->IsTransitioning()
            && !Cover->IsAttached() && !Movement->HasRootMotionSources(),
            TEXT("Rejected request remains stationary without delayed side effects"));
        NextScenario(); break;
    case 6:
        if (Elapsed < .5f) return;
        Key(EKeys::SpaceBar, false);
        Check(Cover->IsAttached() && !Cover->IsTransitioning() && Movement->IsMovingOnGround(),
            TEXT("A held context press performs one cover entry without repeated toggles"));
        Go(8); break;
    case 7:
        if (Elapsed < .2f) return;
        Check(!Cover->IsAttached() && !Cover->IsTransitioning(),
            TEXT("A changed height class cannot retain the previous high-cover protection state"));
        NextScenario(); break;
    case 8:
        if (Elapsed < .15f) return;
        Key(EKeys::SpaceBar, true); Go(9); break;
    case 9:
        if (Elapsed < .12f) return;
        Key(EKeys::SpaceBar, false); Go(10); break;
    case 10:
        if (Elapsed < .3f) return;
        Check(!Cover->IsAttached() && !Cover->IsTransitioning() && Movement->IsMovingOnGround(),
            TEXT("A new context press exits once and release does not queue a jump"));
        Check(!Movement->HasRootMotionSources() && !Movement->bConstrainToPlane,
            TEXT("Repeated context entry/exit releases source and constraint ownership"));
        NextScenario(); break;
    case 11:
        bObservedGait |= Character->GetDodge()->IsDodging() && Anim
            && Anim->Montage_IsPlaying(Character->GetDodge()->RollMontage);
        if (!bCaptured && Elapsed > .15f) { Capture(TEXT("roll_away")); bCaptured = true; }
        if (Elapsed < .3f || Character->GetDodge()->IsDodging()) return;
        Check(bObservedGait && !Cover->IsAttached() && !Character->bIsCrouched
            && FVector::Dist2D(Start, Character->GetActorLocation()) >= 100.f
            && FVector::Dist2D(Start, Character->GetActorLocation()) <= 355.f,
            TEXT("Cover escape evaluates roll montage and completes bounded travel"));
        Go(13); break;
    case 12:
        if (Elapsed < .3f) return;
        Check(Cover->IsLowCover() && Character->bIsCrouched && !Character->GetDodge()->IsDodging()
            && !Movement->HasRootMotionSources() && FVector::Dist(Start, Character->GetActorLocation()) < 1.f,
            TEXT("Rejected low-ceiling roll remains protected without deferred standing or displacement"));
        NextScenario(); break;
    case 13:
        if (Movement->HasRootMotionSources() && Elapsed < .1f) return;
        UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_ROLL_CLEANUP scenario=%d settled=%.4f feet=%.3f mode=%d sources=%d plane=%d"),
            Scenario, Elapsed, Feet-FloorZ, static_cast<int32>(Movement->MovementMode), Movement->HasRootMotionSources(), Movement->bConstrainToPlane);
        Check(Movement->IsMovingOnGround() && FMath::Abs(Feet - FloorZ) < 5.f
            && !Movement->HasRootMotionSources() && !Movement->bConstrainToPlane,
            TEXT("Cover escape cleanup settles within 0.1 seconds with ground support and no stale source or plane"));
        NextScenario(); break;
    case 20:
        if (Elapsed < .3f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        Check(Combat->GetWeaponKind() == (Scenario == 17 ? EGunnerWeaponKind::Pistol : EGunnerWeaponKind::Rifle),
            TEXT("Directional crouch fixture selects its requested weapon"));
        Character->Crouch(); Key(EKeys::RightMouseButton, true); Go(21); break;
    case 21:
        if (Elapsed < .4f) return;
        Check(Character->bIsCrouched && Combat->IsAiming() && Anim && Anim->bDirectionalCrouchPoseReady,
            TEXT("Free crouched ADS has its actual stance and directional graph active"));
        Start = Character->GetActorLocation(); Key(DirectionKey(), true); Go(22); break;
    case 22:
        if (Elapsed < .45f) return;
        {
            const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Character->GetVelocity());
            const FVector Desired = DirectionStep == 0 ? FVector::ForwardVector : DirectionStep == 1 ? -FVector::ForwardVector
                : DirectionStep == 2 ? -FVector::RightVector : FVector::RightVector;
            const FVector GraphVelocity(Anim ? Anim->ForwardSpeed : 0.f, Anim ? Anim->RightSpeed : 0.f, 0.f);
            Check(Character->bIsCrouched && Combat->IsAiming() && !Cover->IsAttached()
                && FVector::DotProduct(LocalVelocity, Desired) > 40.f && FVector::DotProduct(GraphVelocity, Desired) > 40.f
                && FVector::Dist2D(Start, Character->GetActorLocation()) > 15.f,
                TEXT("Crouched ADS moves in the requested local direction and evaluates the matching graph axis"));
            Check(FMath::Abs(FMath::FindDeltaAngleDegrees(Character->GetActorRotation().Yaw, Player->GetControlRotation().Yaw)) < 5.f
                && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), 62.f, .1f),
                TEXT("Directional crouch preserves aim facing and the actual crouched capsule"));
            Capture(*FString::Printf(TEXT("direction_%d"), DirectionStep));
            Key(DirectionKey(), false); Go(23);
        }
        break;
    case 23:
        if (Elapsed < .2f) return;
        Check(Character->GetVelocity().Size2D() < 5.f, TEXT("Released directional crouch input stops"));
        if (++DirectionStep < 4) { Start = Character->GetActorLocation(); Key(DirectionKey(), true); Go(22); }
        else { Key(EKeys::RightMouseButton, false); NextScenario(); }
        break;
    case 30:
        if (Elapsed < .4f || Cover->IsTransitioning()) return;
        Check(Cover->IsAttached() && !Cover->IsLowCover(), TEXT("High-cover edge fixture is attached before crouching"));
        StandingHead = Character->GetMesh()->GetSocketLocation(TEXT("head")).Z;
        Character->Crouch(); Go(31); break;
    case 31:
        if (Elapsed < .4f) return;
        Check(Character->bIsCrouched && Cover->CanPeek(Character->GetShoulderSide()),
            TEXT("Installed directional pose admits the actual crouched edge route"));
        Start = Character->GetActorLocation(); MaxHead = 0.f; bPoseSafe = true;
        Key(EKeys::RightMouseButton, true); Go(32); break;
    case 32:
        MaxHead = FMath::Max(MaxHead, Character->GetMesh()->GetSocketLocation(TEXT("head")).Z);
        bPoseSafe &= Character->bIsCrouched && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), 62.f, .1f);
        if (Elapsed < .9f) return;
        Check(Cover->IsAttached() && Cover->IsPeeking() && Combat->IsAiming()
            && Character->GetActorLocation().X < Start.X - 55.f && FVector::Dist2D(Start, Character->GetActorLocation()) < 76.f,
            TEXT("Crouched ADS physically reaches exposure around the high-cover edge"));
        Check(bPoseSafe && MaxHead < StandingHead - 8.f, TEXT("Crouched edge exposure retains low capsule and head below its standing baseline"));
        Capture(TEXT("crouched_high_peek")); Key(EKeys::RightMouseButton, false); Go(33); break;
    case 33:
        MaxHead = FMath::Max(MaxHead, Character->GetMesh()->GetSocketLocation(TEXT("head")).Z);
        bPoseSafe &= Character->bIsCrouched;
        if (Cover->IsPeeking() && Elapsed < 2.6f) return;
        Check(Cover->IsAttached() && !Cover->IsPeeking() && !Combat->IsAiming() && bPoseSafe
            && MaxHead < StandingHead - 8.f && FVector::Dist2D(Start, Character->GetActorLocation()) < 3.f,
            TEXT("Crouched edge return restores the anchor without a standing head pop or movement lock"));
        NextScenario(); break;
    case 35:
        if (Elapsed < .5f || Cover->IsTransitioning()) return;
        Check(Cover->IsLowCover() && Character->bIsCrouched && Anim && Anim->bProtectiveLowCover,
            TEXT("Low cover selects its original protective graph branch despite installed directional capability"));
        DirectionStep = 0; MaxHead = 0.f; bPoseSafe = true; Start = Character->GetActorLocation();
        Key(EKeys::A, true); Go(36); break;
    case 36:
        MaxHead = FMath::Max(MaxHead, Character->GetMesh()->GetSocketLocation(TEXT("head")).Z);
        bPoseSafe &= Character->bIsCrouched && Anim && Anim->bProtectiveLowCover && Cover->IsLowCover();
        if (Elapsed < .5f) return;
        Check(FVector::Dist2D(Start, Character->GetActorLocation()) > 15.f && bPoseSafe && MaxHead < FloorZ + 110.f,
            TEXT("Both directions of low-cover movement keep the evaluated original crouch head protected"));
        Key(DirectionStep == 0 ? EKeys::A : EKeys::D, false);
        if (DirectionStep++ == 0) { Start = Character->GetActorLocation(); Key(EKeys::D, true); Go(36); }
        else { Capture(TEXT("protective_low_cover")); Go(37); }
        break;
    case 37:
        if (Elapsed < .2f) return;
        Check(Character->GetVelocity().Size2D() < 5.f && Anim && Anim->bProtectiveLowCover,
            TEXT("Protected low-cover movement releases cleanly"));
        NextScenario(); break;
    case 40:
        if (Elapsed < .05f) return;
        Key(EKeys::SpaceBar, false); bObservedGait = false; Go(41); break;
    case 41:
        bObservedGait |= Character->GetDodge()->IsDodging() && Anim
            && Anim->Montage_IsPlaying(Character->GetDodge()->RollMontage);
        if (Elapsed < 1.3f || Character->GetDodge()->IsDodging()) return;
        Check(bObservedGait && Movement->IsMovingOnGround() && !Cover->IsAttached(),
            TEXT("An open-ground context tap selects an animated roll without jumping"));
        Combat->EquipRifle(); Go(47); break;
    case 47:
        if (Elapsed < .2f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        Movement->StopMovementImmediately();
        Character->SetActorLocation(FVector(0.f, StartY, FloorZ + 92.f), false, nullptr, ETeleportType::TeleportPhysics);
        Character->SetActorRotation(FRotator(0.f, 90.f, 0.f)); Player->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
        Key(EKeys::W, true); Key(EKeys::SpaceBar, true); Go(42); break;
    case 42:
        if (Elapsed < .65f) return;
        Check(Character->IsSprinting() && Character->GetVelocity().Size2D() > 430.f && Anim && Anim->bSprinting,
            TEXT("Held context with forward input enters genuine sprint locomotion"));
        if (Anim && Anim->bRifleSupportGripPoseReady)
        {
            const auto* Visual = Combat->GetWeaponVisual();
            const auto* Weapon = Combat->GetWeaponData();
            const FTransform Grip = Weapon->LeftHandGripTransform * Visual->GetComponentTransform();
            const FTransform Hand = Character->GetMesh()->GetSocketTransform(TEXT("hand_l"));
            const float Error = FVector::Distance(Hand.GetLocation(), Grip.GetLocation());
            const float Angle = FMath::RadiansToDegrees(Hand.GetRotation().AngularDistance(Grip.GetRotation()));
            UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_SPRINT_GRIP error=%.3f angle=%.3f head=%.3f"),
                Error, Angle, Character->GetMesh()->GetSocketLocation(TEXT("head")).Z - Feet);
            Check(Anim->bRifleSupportGrip && Error < 2.f && Angle < 5.f,
                TEXT("Hunched rifle sprint keeps the support hand on its actual weapon grip"));
            Check(Character->GetMesh()->GetSocketLocation(TEXT("head")).Z - Feet < 145.f,
                TEXT("Rifle sprint evaluates the authored lowered head and torso"));
        }
        BeforeYaw = Player->GetControlRotation().Yaw; BeforePitch = Player->GetControlRotation().Pitch;
        PreviousTravelYaw = Character->GetVelocity().Rotation().Yaw; bHeadingSafe = true;
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseX, 60.f, 1.f/60.f, 1, FPlatformTime::Cycles64()));
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseY, 5.f, 1.f/60.f, 1, FPlatformTime::Cycles64()));
        Go(43); break;
    case 43:
        {
            const float TravelYaw = Character->GetVelocity().Rotation().Yaw;
            bHeadingSafe &= FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousTravelYaw, TravelYaw)) <= Settings->SprintTurnRate * DeltaSeconds + 5.f;
            PreviousTravelYaw = TravelYaw;
            if (Elapsed < .25f) return;
            const auto* Camera = Character->FindComponentByClass<UCameraComponent>();
            Check(FMath::FindDeltaAngleDegrees(BeforeYaw, Player->GetControlRotation().Yaw) > .1f
                && FMath::FindDeltaAngleDegrees(BeforePitch, Player->GetControlRotation().Pitch) > .1f
                && Camera && FMath::Abs(FMath::FindDeltaAngleDegrees(Player->GetControlRotation().Yaw, Camera->GetComponentRotation().Yaw)) < 1.f,
                TEXT("Synthetic mouse yaw and pitch stay free and the rendered sprint camera follows"));
            UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_MOUSE yaw_delta=%.3f pitch_delta=%.3f"),
                FMath::FindDeltaAngleDegrees(BeforeYaw, Player->GetControlRotation().Yaw),
                FMath::FindDeltaAngleDegrees(BeforePitch, Player->GetControlRotation().Pitch));
            // Mouse sensitivity is user tuning. Test the turn limiter against a
            // known controller target separately from the real mouse action above.
            Player->SetControlRotation(FRotator(Player->GetControlRotation().Pitch, TravelYaw + 100.f, 0.f));
            PreviousTravelYaw = TravelYaw; bHeadingSafe = true; Go(46);
        }
        break;
    case 46:
        {
            const float TravelYaw = Character->GetVelocity().Rotation().Yaw;
            bHeadingSafe &= FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousTravelYaw, TravelYaw)) <= Settings->SprintTurnRate * DeltaSeconds + 5.f;
            PreviousTravelYaw = TravelYaw;
            if (Elapsed < .15f) return;
            UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_SPRINT_TURN bounded=%d gap=%.3f speed=%.3f"),
                bHeadingSafe, FMath::Abs(FMath::FindDeltaAngleDegrees(TravelYaw, Player->GetControlRotation().Yaw)),
                Character->GetVelocity().Size2D());
            Check(Character->IsSprinting() && bHeadingSafe
                && FMath::Abs(FMath::FindDeltaAngleDegrees(TravelYaw, Player->GetControlRotation().Yaw)) > 30.f,
                TEXT("Sprint travel turns at a bounded rate instead of snapping to the new camera heading"));
            Capture(TEXT("context_sprint_turn"));
            SprintShotsBefore = Combat->GetShotsFired();
            Key(EKeys::SpaceBar, false); Key(EKeys::W, false); Key(EKeys::LeftMouseButton, true); Go(44);
        }
        break;
    case 44:
        if (Elapsed < .08f) return;
        Key(EKeys::LeftMouseButton, false);
        Check(!Character->IsSprinting() && Combat->GetShotsFired() > SprintShotsBefore,
            TEXT("Releasing contextual sprint admits the first fire press without a second click"));
        Go(45); break;
    case 45:
        if (Elapsed < .3f) return;
        Check(!Character->GetDodge()->IsDodging() && !Movement->HasRootMotionSources()
            && Character->GetVelocity().Size2D() < 5.f && !Combat->IsAiming(),
            TEXT("Held context release leaves no extra roll, sprint, aim or movement lock"));
        NextScenario(); break;
    case 50:
        if (Elapsed < .25f) return;
        Check(Character->IsAimHeld() && Combat->IsAiming() && Anim && Anim->bAiming,
            TEXT("Held-aim interruption fixture starts in evaluated standing ADS"));
        Start = Character->GetActorLocation(); bObservedGait = false; bPoseSafe = true;
        Key(EKeys::E, true); Go(51); break;
    case 51:
        if (Elapsed > .05f && Held.Contains(EKeys::E)) Key(EKeys::E, false);
        if (Character->GetDodge()->IsDodging())
        {
            bObservedGait |= Anim && Anim->Montage_IsPlaying(Character->GetDodge()->RollMontage);
            bPoseSafe &= !Combat->IsAiming();
        }
        if (Elapsed < 1.3f || Character->GetDodge()->IsDodging()) return;
        Check(bObservedGait && bPoseSafe && FVector::Dist2D(Start, Character->GetActorLocation()) > 100.f
            && FVector::Dist2D(Start, Character->GetActorLocation()) <= 355.f,
            TEXT("Dodge interrupts held ADS for the genuine roll and completes bounded travel"));
        Go(52); break;
    case 52:
        if (Elapsed < .1f) return;
        Check(Held.Contains(EKeys::RightMouseButton) && Character->IsAimHeld() && Combat->IsAiming()
            && Anim && Anim->bAiming && Movement->IsMovingOnGround() && !Movement->HasRootMotionSources(),
            TEXT("Continuously held RMB resumes evaluated ADS after dodge without a second aim or fire press"));
        Capture(TEXT("held_aim_resumed")); Key(EKeys::RightMouseButton, false); Go(53); break;
    case 53:
        if (Elapsed < .15f) return;
        Check(!Character->IsAimHeld() && !Combat->IsAiming() && Anim && !Anim->bAiming,
            TEXT("Releasing resumed RMB clears gameplay and evaluated aim state"));
        NextScenario(); break;
    case 60:
        if (Elapsed < .2f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        Check(Cover->TryAttach(FVector::YAxisVector), TEXT("Armed wall-pose fixture admits cover entry"));
        Go(61); break;
    case 61:
        if (Elapsed < .8f || Cover->IsTransitioning()) return;
        Check(Cover->IsAttached() && !Cover->IsLowCover() && Anim && Anim->bHighCoverPose
            && Anim->bRifleSupportGrip && Anim->UpperBodyWeight < .05f,
            TEXT("Standing rifle cover evaluates its full authored wall pose and weapon grip"));
        {
            const float HeadClearance = FVector::DotProduct(Character->GetMesh()->GetSocketLocation(TEXT("head")) - FVector(Location.X, -240.f, Location.Z), Cover->GetNormal());
            Check(HeadClearance > 8.f && HeadClearance < 25.f,
                TEXT("Authored wall pose is close to the surface with head clearance and unchanged physical anchor"));
        }
        Capture(TEXT("rifle_wall_idle"));
        if (Scenario == 24)
        {
            Check(Combat->GetMagazine() == 0, TEXT("Wall dry-fire fixture uses a rifle emptied by real shots"));
            DryFiresBefore = Combat->GetDryFireCount(); SprintShotsBefore = Combat->GetShotsFired();
            ReserveBefore = Combat->GetReserve(); Start = Location;
            FirstHand = Character->GetMesh()->GetSocketLocation(TEXT("hand_r"))
                - Character->GetMesh()->GetSocketLocation(TEXT("root"));
            bObservedGait = bCaptured = false; bPoseSafe = true; MaxHandTravel = 0.f;
            Key(EKeys::LeftMouseButton, true); Go(72); break;
        }
        if (Scenario == 23)
        { Start = Location; Key(EKeys::RightMouseButton, true); Go(66); break; }
        Start = Location; Key(EKeys::A, true); Go(62); break;
    case 62:
    case 63:
        if (Elapsed < .4f) return;
        {
            const auto* Visual = Combat->GetWeaponVisual();
            const auto* Weapon = Combat->GetWeaponData();
            const FTransform Grip = Weapon->LeftHandGripTransform * Visual->GetComponentTransform();
            const FTransform Hand = Character->GetMesh()->GetSocketTransform(TEXT("hand_l"));
            const float Error = FVector::Distance(Hand.GetLocation(), Grip.GetLocation());
            const float Angle = FMath::RadiansToDegrees(Hand.GetRotation().AngularDistance(Grip.GetRotation()));
            const FVector ExpectedDirection = FRotationMatrix(FRotator(0.f, Player->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y) * (Phase == 62 ? -1.f : 1.f);
            const float LocalSign = Character->GetActorRotation().UnrotateVector(ExpectedDirection).Y;
            UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_WALL_GRIP phase=%d error=%.3f angle=%.3f right_speed=%.3f"), Phase, Error, Angle, Anim ? Anim->RightSpeed : 0.f);
            Check(Anim && Anim->bHighCoverPose && Anim->RightSpeed * LocalSign > 80.f
                && FVector::DotProduct(Location - Start, ExpectedDirection) > 20.f && FMath::Abs(Location.Y - TargetY) < 3.f,
                TEXT("Armed wall gait follows the correct lateral direction at the preserved offset"));
            Check(Error < 2.f && Angle < 5.f, TEXT("Lateral cover motion keeps the support hand aligned to its weapon grip"));
            Capture(Phase == 62 ? TEXT("rifle_wall_left") : TEXT("rifle_wall_right"));
            if (Phase == 62)
            { Key(EKeys::A, false); Key(EKeys::D, true); Start = Location; Go(63); }
            else { Key(EKeys::D, false); Combat->Reload(); Go(64); }
        }
        break;
    case 64:
        if (Elapsed < .3f) return;
        Check(Combat->IsReloading() && Anim && !Anim->bHighCoverPose && !Anim->bRifleSupportGrip,
            TEXT("Reload releases authored wall arms and support-hand lock for the real weapon montage"));
        Combat->StopAllActions(); Go(65); break;
    case 65:
        if (Elapsed < .4f) return;
        Check(Cover->IsAttached() && Anim && Anim->bHighCoverPose && Anim->bRifleSupportGrip,
            TEXT("Finishing weapon action restores the armed wall pose"));
        NextScenario(); break;
    case 66:
        if (Elapsed < .9f) return;
        Check(Cover->IsPeeking() && Combat->IsAiming() && Anim && !Anim->bHighCoverPose
            && !Anim->bRifleSupportGrip && FVector::Dist2D(Start, Location) > 55.f,
            TEXT("High-cover ADS releases the wall pose and physically steps out with the aiming graph"));
        Capture(TEXT("rifle_wall_aim")); Key(EKeys::RightMouseButton, false); Go(67); break;
    case 67:
        if (Elapsed < .6f || Cover->IsPeeking()) return;
        Check(Cover->IsAttached() && !Combat->IsAiming() && Anim && Anim->bHighCoverPose
            && FVector::Dist2D(Start, Location) < 3.f,
            TEXT("Releasing high-cover ADS returns to the anchor and authored wall pose"));
        NextScenario(); break;
    case 70:
        if (Elapsed < .2f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        Key(EKeys::LeftMouseButton, true); Go(71); break;
    case 71:
        if (Combat->GetMagazine() > 0) return;
        Key(EKeys::LeftMouseButton, false);
        Combat->StopAllActions();
        Go(60); break;
    case 72:
        bPoseSafe &= Cover->IsAttached() && !Cover->IsPeeking() && !Character->bIsCrouched
            && !Combat->IsAiming() && FVector::Dist2D(Start, Location) < 2.f;
        if (Combat->IsDryFiring() && Anim && Anim->UpperBodyWeight > .2f)
        {
            bObservedGait |= Anim->IsSlotActive(TEXT("UpperBody"))
                && !Anim->bHighCoverPose && !Anim->bRifleSupportGrip;
            // Subtract the evaluated root so removing the 25 cm wall inset cannot
            // satisfy the animation check by merely translating the entire body.
            const FVector Hand = Character->GetMesh()->GetSocketLocation(TEXT("hand_r"))
                - Character->GetMesh()->GetSocketLocation(TEXT("root"));
            MaxHandTravel = FMath::Max(MaxHandTravel, FVector::Distance(FirstHand, Hand));
            if (!bCaptured && Elapsed > .12f)
            { Capture(TEXT("rifle_wall_dry_fire")); bCaptured = true; }
        }
        if (Elapsed < .4f || Combat->IsDryFiring()) return;
        UE_LOG(LogTemp, Display, TEXT("GUNNER_TRAVERSAL_WALL_DRY_FIRE observed=%d hand_travel=%.3f safe=%d"),
            bObservedGait, MaxHandTravel, bPoseSafe);
        Check(bObservedGait && MaxHandTravel > 5.f,
            TEXT("Standing wall dry fire releases its wall pose and grip lock and reaches the evaluated weapon arm"));
        Check(bPoseSafe && Combat->GetDryFireCount() == DryFiresBefore + 1
            && Combat->GetShotsFired() == SprintShotsBefore && Combat->GetMagazine() == 0
            && Combat->GetReserve() == ReserveBefore && Combat->GetActionState() == EGunnerCombatAction::Idle,
            TEXT("Held empty trigger retains cover and produces one presentation without shot, ammo or action-lock side effects"));
        Key(EKeys::LeftMouseButton, false); Go(73); break;
    case 73:
        if (Elapsed < .4f) return;
        Check(Cover->IsAttached() && Anim && Anim->bHighCoverPose && Anim->bRifleSupportGrip
            && Anim->UpperBodyWeight < .05f && !Combat->IsDryFiring(),
            TEXT("Completed wall dry fire restores its authored wall pose and support-hand grip"));
        NextScenario(); break;
    case 80:
        bRouteSafe &= !Cover->IsAttached() && !Cover->IsTransitioning();
        if (Elapsed < .65f) return;
        Check(bRouteSafe && Character->IsSprinting() && FVector::Dist2D(Start, Location) > 40.f
            && !Movement->HasRootMotionSources(),
            TEXT("Contextual sprint reaches the wall without entering cover while its authored readiness is disabled"));
        RestoreSettings();
        bObservedIntermediate = false; Go(81); break;
    case 81:
        bObservedIntermediate |= Cover->IsTransitioning();
        if (Elapsed < .8f || Cover->IsTransitioning()) return;
        Check(bObservedIntermediate && Cover->IsAttached() && !Character->IsSprinting()
            && FMath::Abs(Location.Y - TargetY) < 3.f && !Movement->HasRootMotionSources(),
            TEXT("Re-enabling cover readiness admits the held sprint through its normal swept approach and final anchor"));
        Key(EKeys::W, false); Key(EKeys::LeftShift, false);
        NextScenario(); break;
    case 90:
        if (Elapsed < .4f || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
        if (Scenario == 27 || Scenario == 29)
        {
            Check(Cover->TryAttach(FVector::YAxisVector), TEXT("Stance fixture accepts its actual cover approach"));
            Go(94); break;
        }
        Go(95); break;
    case 94:
        if (Elapsed < .5f || Cover->IsTransitioning()) return;
        Check(Cover->IsAttached(), TEXT("Stance fixture reaches its cover anchor"));
        if (Scenario == 29)
        {
            Check(Character->bIsCrouched && Anim && Anim->bProtectiveLowCover
                && Anim->CrouchTransitionAlpha == 0.f
                && Character->GetMesh()->GetSocketLocation(TEXT("head")).Z - Feet < 110.f,
                TEXT("Low-cover arrival bypasses the tall crouch-entry clip and stays protected"));
            Key(EKeys::RightMouseButton, true); Go(98); break;
        }
        Go(95); break;
    case 95:
        if (Elapsed < .2f) return;
        StandingHead = Character->GetMesh()->GetSocketLocation(TEXT("head")).Z - Feet;
        Start = Location; StanceMinHead = BIG_NUMBER; StanceMaxHead = -BIG_NUMBER;
        bObservedGait = bCaptured = false;
        Key(EKeys::C, true); Go(Scenario == 28 ? 96 : 91); break;
    case 91:
    case 92:
        if (Elapsed > .03f && Held.Contains(EKeys::C)) Key(EKeys::C, false);
        if (Anim && Anim->CrouchTransitionAlpha > .2f)
        {
            bObservedGait |= Anim->CrouchTransitionPhase == (Phase == 91
                ? EGunnerCrouchTransitionPhase::Entering : EGunnerCrouchTransitionPhase::Exiting)
                && Anim->CrouchTransitionTime > 0.f;
            const float Head = Character->GetMesh()->GetSocketLocation(TEXT("head")).Z - Feet;
            StanceMinHead = FMath::Min(StanceMinHead, Head);
            StanceMaxHead = FMath::Max(StanceMaxHead, Head);
            if (!bCaptured && Elapsed > .09f && StanceMaxHead-StanceMinHead > 20.f)
            { Capture(Phase == 91 ? TEXT("stance_down") : TEXT("stance_up")); bCaptured = true; }
        }
        if (Elapsed < .6f) return;
        UE_LOG(LogTemp, Display, TEXT("GUNNER_STANCE_POSE scenario=%d phase=%d observed=%d head_range=%.3f endpoint=%.3f"),
            Scenario, Phase, bObservedGait, StanceMaxHead-StanceMinHead,
            Character->GetMesh()->GetSocketLocation(TEXT("head")).Z-Feet);
        Check(bObservedGait && StanceMaxHead-StanceMinHead > 15.f && Anim
            && Anim->CrouchTransitionPhase == EGunnerCrouchTransitionPhase::None
            && Anim->CrouchTransitionAlpha == 0.f,
            TEXT("Stationary stance change evaluates a moving authored pose and finishes without looping"));
        Check(FVector::Dist2D(Start, Location) < 2.f && FMath::Abs(Feet-FloorZ) < 5.f
            && Character->bIsCrouched == (Phase == 91)
            && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Phase == 91 ? 62.f : 90.f, .1f),
            TEXT("Cosmetic stance transition preserves native capsule, supported feet and anchor"));
        if (Phase == 91)
        {
            Check(Character->GetMesh()->GetSocketLocation(TEXT("head")).Z-Feet < StandingHead-30.f,
                TEXT("Crouch entry reaches the genuine lowered head endpoint"));
            const FTransform Grip = Combat->GetWeaponData()->LeftHandGripTransform * Combat->GetWeaponVisual()->GetComponentTransform();
            const float GripError = FVector::Distance(Character->GetMesh()->GetSocketLocation(TEXT("hand_l")), Grip.GetLocation());
            UE_LOG(LogTemp, Display, TEXT("GUNNER_STANCE_CARRY scenario=%d grip_error=%.3f alpha=%.3f"), Scenario, GripError, Anim ? Anim->CrouchWeaponCarryAlpha : 0.f);
            Check(Anim && Anim->CrouchWeaponCarryAlpha > .95f && GripError < (Scenario == 26 ? 2.f : 10.f),
                TEXT("Crouch completion retains an armed carry pose with the support hand near its actual grip"));
            Capture(TEXT("stance_crouched_carry"));
            StanceMinHead = BIG_NUMBER; StanceMaxHead = -BIG_NUMBER;
            bObservedGait = bCaptured = false; Go(93);
        }
        else
        {
            Check(FMath::Abs(Character->GetMesh()->GetSocketLocation(TEXT("head")).Z-Feet-StandingHead) < 15.f,
                TEXT("Crouch exit recovers the prior standing pose"));
            NextScenario();
        }
        break;
    case 93:
        // Screenshot readback can span this entire short animation. Settle the
        // crouched endpoint capture before issuing the next stance input.
        if (Elapsed < .25f) return;
        Key(EKeys::C, true); Go(92); break;
    case 96:
        if (Elapsed > .03f && Held.Contains(EKeys::C)) Key(EKeys::C, false);
        if (Elapsed < .1f) return;
        Check(Anim && Anim->CrouchTransitionPhase == EGunnerCrouchTransitionPhase::Entering
            && Anim->CrouchTransitionAlpha > .2f, TEXT("Interruption fixture starts inside a real crouch-entry animation"));
        Key(EKeys::W, true); Key(EKeys::RightMouseButton, true); Start = Location; Go(97); break;
    case 97:
        if (Elapsed < .25f) return;
        Check(Anim && Anim->CrouchTransitionPhase == EGunnerCrouchTransitionPhase::None
            && Anim->CrouchTransitionAlpha == 0.f && Combat->IsAiming()
            && Character->GetVelocity().Size2D() > 40.f && FVector::Dist2D(Start,Location)>5.f,
            TEXT("Movement and ADS promptly cancel stance presentation without delaying control"));
        Key(EKeys::W,false); Key(EKeys::RightMouseButton,false); Go(99); break;
    case 98:
        bPoseSafe &= Anim && Anim->CrouchTransitionAlpha == 0.f;
        if (Elapsed < .5f) return;
        Check(bPoseSafe && Combat->IsAiming() && !Character->bIsCrouched,
            TEXT("Low-cover ADS bypasses stationary exit presentation"));
        Key(EKeys::RightMouseButton,false); Go(99); break;
    case 99:
        if (Elapsed < .6f) return;
        Check(Anim && Anim->CrouchTransitionAlpha == 0.f
            && Anim->CrouchTransitionPhase == EGunnerCrouchTransitionPhase::None
            && Character->bIsCrouched && !Combat->IsAiming(),
            TEXT("Settled unchanged crouch does not restart a cancelled or bypassed transition"));
        if (Scenario == 29)
            Check(Cover->IsLowCover() && Character->GetMesh()->GetSocketLocation(TEXT("head")).Z-Feet < 110.f,
                TEXT("Low-cover ADS release returns to the protected source pose"));
        NextScenario(); break;
    default: Check(false, TEXT("Known traversal probe phase")); Finish(); break;
    }
#endif
}
