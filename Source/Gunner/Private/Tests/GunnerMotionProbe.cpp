#include "Tests/GunnerMotionProbe.h"

#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerDodgeComponent.h"
#include "GunnerInputConfig.h"
#include "GunnerTarget.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AGunnerMotionProbe::AGunnerMotionProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}

void AGunnerMotionProbe::Check(bool bPass, const TCHAR* Name)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_MOTION_CHECK PASS %s"), Name); }
    else
    {
        ++Failures;
        UE_LOG(LogTemp, Error, TEXT("GUNNER_MOTION_CHECK FAIL stage=%d %s"), Stage, Name);
    }
}

void AGunnerMotionProbe::Key(const FKey& InputKey, bool bPressed)
{
    if (!Player) return;
    if (bPressed) HeldKeys.AddUnique(InputKey); else HeldKeys.Remove(InputKey);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void AGunnerMotionProbe::Capture(const TCHAR* Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name, false, false);
}

void AGunnerMotionProbe::Advance(float Delay)
{
    ++Stage;
    Elapsed = 0.f;
    NextDelay = Delay;
}

void AGunnerMotionProbe::Teleport(const FVector& Location, float Yaw)
{
    bTrackTarget = false;
    Character->GetCover()->Detach();
    Character->GetCombat()->StopAllActions();
    Character->GetCombat()->SetCombatBlocked(false);
    Character->UnCrouch();
    Character->GetCharacterMovement()->StopMovementImmediately();
    FVector SettledLocation = Location;
    SettledLocation.Z = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
    Character->SetActorLocation(SettledLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0.f, Yaw, 0.f));
    Player->SetControlRotation(FRotator(0.f, Yaw, 0.f));
}

AStaticMeshActor* AGunnerMotionProbe::SpawnBox(const FVector& Location, const FVector& Size)
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
    Mesh->SetMobility(EComponentMobility::Static);
    return Box;
}

void AGunnerMotionProbe::Cleanup()
{
    InputGuard.Restore();
    while (!HeldKeys.IsEmpty())
    {
        const FKey ToRelease = HeldKeys.Last();
        Key(ToRelease, false);
    }
    if (Blocker) Blocker->Destroy();
    if (Ceiling) Ceiling->Destroy();
    if (Target) Target->Destroy();
    Blocker = nullptr;
    Ceiling = nullptr;
    Target = nullptr;
}

void AGunnerMotionProbe::Finish()
{
    Cleanup();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_MOTION_COMPLETE failures=%d"), Failures);
    SetActorTickEnabled(false);
}

void AGunnerMotionProbe::EndPlay(const EEndPlayReason::Type Reason)
{
    Cleanup();
    Super::EndPlay(Reason);
}

void AGunnerMotionProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    InputGuard.Begin(GetWorld());
    if (bTrackTarget && Player && Character && Target)
    {
        // Recompute against the live shoulder camera while its focus distance converges.
        if (const auto* Camera = Character->FindComponentByClass<UCameraComponent>())
            Player->SetControlRotation((Target->GetActorLocation() - Camera->GetComponentLocation()).Rotation());
    }
    Elapsed += DeltaSeconds;
    if (Elapsed < NextDelay) return;
    if (Stage == 0)
    {
        Player = GetWorld()->GetFirstPlayerController(); // Explicitly single-player diagnostic only.
        if (Player) Player->FlushPressedKeys();
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Check(Character && Character->GetCombat() && Character->GetCover(), TEXT("Motion pawn and components ready"));
        if (!Character) { Finish(); return; }
        auto* Combat = Character->GetCombat();
        const bool bReady = Combat && Character->GetMesh()->GetAnimInstance() && Combat->RifleData && Combat->PistolData &&
            Combat->RifleData->FireMontage && Combat->RifleData->ReloadMontage && Combat->RifleData->EquipMontage &&
            Combat->PistolData->FireMontage && Combat->PistolData->ReloadMontage && Combat->PistolData->EquipMontage &&
            Combat->PistolData->MeleeMontage;
        Check(bReady, TEXT("Both authored weapon and action data assets present"));
        if (!bReady) { Finish(); return; }
        Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Check(Cube != nullptr, TEXT("Transient test fixture mesh loads"));
        if (!Cube) { Finish(); return; }
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Target = GetWorld()->SpawnActor<AGunnerTarget>(FVector(-600.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
        if (!Target) { Check(false, TEXT("Transient target spawned")); Finish(); return; }
        Target->TargetMesh->SetStaticMesh(Cube);
        Target->TargetMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.6f));
        Target->MaxDurability = 10000.f;
        Target->ResetTarget();
        Teleport(FVector(-1500.f, 0.f, 92.f));
        Advance(1.f);
        return;
    }

    auto* Combat = Character->GetCombat();
    auto* Cover = Character->GetCover();
    auto* Movement = Character->GetCharacterMovement();
    auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    const auto MontageDuration = [](const UAnimMontage* Montage)
    {
        return Montage ? Montage->GetPlayLength() / FMath::Max(0.01f, Montage->RateScale) : 0.f;
    };
    switch (Stage)
    {
    case 1:
        Check(Anim != nullptr, TEXT("Native motion animation instance active"));
        Check(Movement->IsMovingOnGround() && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 90.f),
            TEXT("Standing capsule grounded at 90 cm"));
        Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle, TEXT("Rifle equipped at spawn"));
        Capture(TEXT("motion_idle.png"));
        Start = Character->GetActorLocation(); Key(EKeys::W, true); Advance(1.f); break;
    case 2:
        Check(Character->GetActorLocation().X > Start.X + 150.f && Anim && Anim->Speed > 100.f,
            TEXT("W drives armed movement and live animation speed"));
        Capture(TEXT("motion_move.png")); Key(EKeys::LeftShift, true); Advance(0.65f); break;
    case 3:
        Check(Character->IsSprinting() && Character->GetVelocity().Size2D() > 430.f && Anim && Anim->bSprinting,
            TEXT("Shift drives faster authored sprint state"));
        Capture(TEXT("motion_sprint.png"));
        Key(EKeys::LeftShift, false); Key(EKeys::W, false);
        Advance(0.5f); break;
    case 4:
        Check(Character->GetVelocity().Size2D() < 1.f, TEXT("Released movement stops"));
        Teleport(FVector(-1500.f, 0.f, 92.f)); bTrackTarget = true;
        Key(EKeys::RightMouseButton, true); Advance(0.6f); break;
    case 5:
        Check(Combat->IsAiming() && Anim && Anim->bAiming && Character->FindComponentByClass<UCameraComponent>()->FieldOfView < 65.f,
            TEXT("RMB enables aim pose and focused camera"));
        Capture(TEXT("motion_aim.png"));
        ShotsBefore = Combat->GetShotsFired(); HitsBefore = Target->GetTotalHitCount();
        Key(EKeys::LeftMouseButton, true); Advance(0.7f); break;
    case 6:
        Check(Combat->GetShotsFired() >= ShotsBefore + 3, TEXT("Held rifle fires repeatedly"));
        Check(Target->GetTotalHitCount() > HitsBefore, TEXT("Two-stage rifle trace damages visible target"));
        Capture(TEXT("motion_rifle_fire.png")); Key(EKeys::LeftMouseButton, false); Key(EKeys::RightMouseButton, false);
        HandBefore = Character->GetMesh()->GetSocketTransform(TEXT("hand_l"), RTS_Component).GetLocation();
        MagazineBefore = Combat->GetMagazine(); ReserveBefore = Combat->GetReserve(); Key(EKeys::R, true); Advance(0.85f); break;
    case 7:
    {
        Key(EKeys::R, false);
        Check(Combat->IsReloading() && Combat->GetMagazine() == MagazineBefore, TEXT("Reload plays before ammunition commits"));
        const float ReloadSlotWeight = Anim ? Anim->GetSlotMontageGlobalWeight(TEXT("UpperBody")) : 0.f;
        const float ReloadHandDelta = FVector::Distance(HandBefore,
            Character->GetMesh()->GetSocketTransform(TEXT("hand_l"), RTS_Component).GetLocation());
        UE_LOG(LogTemp, Display, TEXT("GUNNER_ANIM_PROBE reload_slot=%.4f hand_delta=%.4f montage_time=%.4f"),
            ReloadSlotWeight, ReloadHandDelta, Anim ? Anim->Montage_GetPosition(Combat->GetWeaponData()->ReloadMontage) : -1.f);
        Check(Anim && ReloadSlotWeight > 0.5f && ReloadHandDelta > 4.f,
            TEXT("Reload slot evaluates and visibly moves the authored left-hand pose"));
        Capture(TEXT("motion_reload.png")); Key(EKeys::C, true);
        Advance(MontageDuration(Combat->GetWeaponData()->ReloadMontage) + 0.2f); break;
    }
    case 8:
        Key(EKeys::C, false);
        Check(!Character->bIsCrouched && !Movement->bWantsToCrouch,
            TEXT("Upright reload rejects crouching into an unsupported action pose"));
        Check(!Combat->IsReloading() && Combat->GetMagazine() == Combat->GetWeaponData()->MagazineCapacity &&
            Combat->GetMagazine() - MagazineBefore == ReserveBefore - Combat->GetReserve(), TEXT("Completed reload transfers conserved reserve rounds"));
        Key(EKeys::Two, true); Advance(); break;
    case 9:
        Key(EKeys::Two, false); Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol, TEXT("2 equips pistol"));
        Advance(MontageDuration(Combat->PistolData->EquipMontage) + 0.2f); break;
    case 10:
        Check(Combat->GetActionState() == EGunnerCombatAction::Idle && Anim && Anim->bPistol, TEXT("Pistol equip completes into pistol animation"));
        Key(EKeys::RightMouseButton, true); Advance(0.35f); break;
    case 11:
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.65f); break;
    case 12:
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Held semi-automatic pistol fires exactly once"));
        Capture(TEXT("motion_pistol_fire.png")); Key(EKeys::LeftMouseButton, false); Key(EKeys::RightMouseButton, false); Advance(); break;
    case 13: Key(EKeys::LeftMouseButton, true); Advance(); break;
    case 14:
        Key(EKeys::LeftMouseButton, false);
        Check(Combat->GetShotsFired() == ShotsBefore + 2, TEXT("Second pistol press fires one additional shot"));
        MagazineBefore = Combat->GetMagazine(); ReserveBefore = Combat->GetReserve();
        Key(EKeys::R, true); Advance(); break;
    case 15:
        Key(EKeys::R, false); Check(Combat->IsReloading(), TEXT("Partial pistol magazine can reload"));
        Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.05f, Combat->PistolData->ReloadMontage);
        Advance(0.3f); break;
    case 16:
        Check(!Combat->IsReloading() && Combat->GetMagazine() == MagazineBefore && Combat->GetReserve() == ReserveBefore,
            TEXT("Interrupted reload commits no ammunition and unlocks state"));
        Key(EKeys::R, true); Advance(); break;
    case 17: Key(EKeys::R, false); Key(EKeys::LeftShift, true); Key(EKeys::W, true); Advance(0.3f); break;
    case 18:
        Check(Character->IsSprinting() && !Combat->IsReloading() && Combat->GetMagazine() == MagazineBefore && Combat->GetReserve() == ReserveBefore,
            TEXT("Sprint cancels reload without transferring ammunition"));
        Key(EKeys::LeftShift, false); Key(EKeys::W, false); Teleport(FVector(-1500.f, 0.f, 92.f)); Advance(0.3f); break;
    case 19: Key(EKeys::C, true); Advance(0.15f); break;
    case 20: Key(EKeys::C, false); Advance(0.4f); break;
    case 21:
        Check(Character->bIsCrouched && Anim && Anim->bCrouched &&
            FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 62.f), TEXT("C enables real crouch state and 62 cm capsule"));
        Capture(TEXT("motion_crouch.png")); Start = Character->GetActorLocation(); Key(EKeys::W, true); Advance(0.6f); break;
    case 22:
        Check(Character->bIsCrouched && FVector::Dist2D(Character->GetActorLocation(), Start) > 40.f && Character->GetVelocity().Size2D() <= 145.f,
            TEXT("Crouched movement uses reduced speed"));
        Key(EKeys::W, false);
        Ceiling = SpawnBox(FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y, 155.f), FVector(240.f, 240.f, 20.f));
        Check(Ceiling != nullptr, TEXT("Clearance fixture spawned")); Key(EKeys::C, true); Advance(0.15f); break;
    case 23: Key(EKeys::C, false); Advance(0.4f); break;
    case 24:
        Check(Character->bIsCrouched && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 62.f),
            TEXT("Low ceiling rejects standing without capsule overlap"));
        if (Ceiling) Ceiling->Destroy(); Ceiling = nullptr; Key(EKeys::C, true); Advance(0.15f); break;
    case 25: Key(EKeys::C, false); Advance(0.4f); break;
    case 26:
        Check(!Character->bIsCrouched && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 90.f),
            TEXT("Standing succeeds after clearance returns"));
        Teleport(FVector(0.f, -950.f, 92.f), 90.f); Key(EKeys::SpaceBar, true); Advance(); break;
    case 27:
        Key(EKeys::SpaceBar, false); Check(!Cover->IsAttached() && Movement->IsFalling(), TEXT("Out-of-reach cover rejects attach and falls back to jump"));
        Advance(0.9f); break;
    case 28: Teleport(FVector(0.f, -850.f, 92.f), 90.f); Advance(0.3f); break;
    case 29: Key(EKeys::SpaceBar, true); Advance(0.5f); break;
    case 30:
        Key(EKeys::SpaceBar, false);
        Check(Cover->IsLowCover() && Character->bIsCrouched && FMath::IsNearlyEqual(Character->GetActorLocation().Y, -802.f, 4.f),
            TEXT("Space attaches low cover at 52 cm and crouches"));
        Check(Character->GetMesh()->GetSocketLocation(TEXT("head")).Z < 110.f,
            TEXT("Protected low-cover animation keeps the head bone below the 115 cm fixture"));
        Check(Anim && Anim->UpperBodyWeight < 0.05f,
            TEXT("Protected crouch preserves the acquired torso pose without standing weapon overlay"));
        Capture(TEXT("motion_low_cover.png")); Start = Character->GetActorLocation(); Key(EKeys::D, true); Advance(1.35f); break;
    case 31:
        Check(Cover->IsAttached() && FMath::IsNearlyEqual(Character->GetActorLocation().Y, -802.f, 4.f) &&
            Character->GetActorLocation().X >= -165.f && FVector::Dist2D(Character->GetActorLocation(), Start) > 80.f,
            TEXT("Cover shuffle keeps offset and stops before wall endpoint"));
        Key(EKeys::D, false); Key(EKeys::RightMouseButton, true); Advance(0.5f); break;
    case 32:
        Check(Combat->IsAiming() && !Character->bIsCrouched, TEXT("Low-cover aim exposes the standing firing pose"));
        Capture(TEXT("motion_cover_aim.png")); Key(EKeys::RightMouseButton, false); Key(EKeys::SpaceBar, true); Advance(); break;
    case 33:
        Key(EKeys::SpaceBar, false); Check(!Cover->IsAttached(), TEXT("Second Space safely detaches cover"));
        Teleport(FVector(500.f, 550.f, 92.f), 90.f); Advance(0.3f); break;
    case 34: Key(EKeys::SpaceBar, true); Advance(); break;
    case 35:
        Key(EKeys::SpaceBar, false);
        Check(Cover->IsAttached() && !Cover->IsLowCover(), TEXT("180 cm fixture classified as high cover"));
        Capture(TEXT("motion_high_cover.png")); ShotsBefore = Combat->GetShotsFired();
        Key(EKeys::RightMouseButton, true); Key(EKeys::LeftMouseButton, true); Advance(0.3f); break;
    case 36:
        Check(!Combat->IsAiming() && Combat->GetShotsFired() == ShotsBefore, TEXT("High-cover center rejects blocked aim and fire"));
        Key(EKeys::RightMouseButton, false); Key(EKeys::LeftMouseButton, false);
        MagazineBefore = Combat->GetMagazine(); ReserveBefore = Combat->GetReserve(); Key(EKeys::R, true);
        Stage = 50; Elapsed = 0.f; NextDelay = 0.2f; break;
    case 37:
        Key(EKeys::SpaceBar, false); Teleport(FVector(-1500.f, 0.f, 92.f)); bTrackTarget = true; Advance(0.4f); break;
    case 38:
        Blocker = SpawnBox(FVector(Character->GetActorLocation().X + 45.f, Character->GetActorLocation().Y, 110.f), FVector(10.f, 250.f, 220.f));
        Check(Blocker != nullptr, TEXT("Muzzle obstruction fixture spawned"));
        HitsBefore = Target->GetTotalHitCount(); ShotsBefore = Combat->GetShotsFired();
        Key(EKeys::LeftMouseButton, true); Advance(0.25f); break;
    case 39:
        Key(EKeys::LeftMouseButton, false);
        Check(Combat->GetShotsFired() == ShotsBefore + 1 && Target->GetTotalHitCount() == HitsBefore && Combat->WasLastShotObstructed(),
            TEXT("Barrel obstruction consumes shot but cannot damage target through cover"));
        Capture(TEXT("motion_obstruction.png")); if (Blocker) Blocker->Destroy(); Blocker = nullptr;
        bTrackTarget = false; Player->SetControlRotation(FRotator::ZeroRotator); Character->SetActorRotation(FRotator::ZeroRotator);
        Target->SetActorLocation(Character->GetActorLocation() + FVector(115.f, 0.f, 8.f));
        HitsBefore = Target->GetTotalHitCount(); Key(EKeys::F, true); Advance(0.35f); break;
    case 40:
        UE_LOG(LogTemp, Display, TEXT("GUNNER_ANIM_PROBE melee_slot=%.4f montage_time=%.4f"),
            Anim ? Anim->GetSlotMontageGlobalWeight(TEXT("FullBody")) : 0.f,
            Anim ? Anim->Montage_GetPosition(Combat->GetWeaponData()->MeleeMontage) : -1.f);
        Key(EKeys::F, false); Check(Combat->IsMeleeing(), TEXT("F starts authored melee action"));
        Check(Anim && Anim->GetSlotMontageGlobalWeight(TEXT("FullBody")) > 0.5f, TEXT("Melee full-body montage contributes to the evaluated pose"));
        Capture(TEXT("motion_melee.png")); Advance(MontageDuration(Combat->PistolData->MeleeMontage) + 0.2f); break;
    case 41:
        Check(Target->GetTotalHitCount() == HitsBefore + 1 && !Combat->IsMeleeing(), TEXT("Melee commits one finite nearby hit then releases state"));
        Blocker = SpawnBox(Character->GetActorLocation() + FVector(60.f, 0.f, 10.f), FVector(10.f, 250.f, 220.f));
        HitsBefore = Target->GetTotalHitCount(); Key(EKeys::F, true); Advance(); break;
    case 42:
        Key(EKeys::F, false); Advance(MontageDuration(Combat->PistolData->MeleeMontage) + 0.2f); break;
    case 43:
        Check(Target->GetTotalHitCount() == HitsBefore, TEXT("Melee cannot pass through intervening wall"));
        if (Blocker) Blocker->Destroy(); Blocker = nullptr; Key(EKeys::One, true); Advance(); break;
    case 44:
        Key(EKeys::One, false); Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle, TEXT("1 re-equips rifle"));
        Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.05f, Combat->RifleData->EquipMontage); Advance(0.3f); break;
    case 45:
        Check(Combat->GetActionState() == EGunnerCombatAction::Idle, TEXT("Interrupted equip cannot leave combat locked"));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::RightMouseButton, true); Key(EKeys::LeftMouseButton, true); Advance(0.3f); break;
    case 46:
        Check(Combat->GetShotsFired() > ShotsBefore, TEXT("Fire remains usable after interrupted equip"));
        Player->UnPossess();
        Check(!Combat->IsAiming() && !Combat->IsReloading() && !Combat->IsMeleeing(), TEXT("Unpossess clears held combat actions"));
        Check(!Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->HasMappingContext(Character->GetInputConfig()->MappingContext),
            TEXT("Unpossess removes owned motion input context"));
        Key(EKeys::LeftMouseButton, false); Key(EKeys::RightMouseButton, false); ShotsBefore = Combat->GetShotsFired(); Advance(); break;
    case 47: Player->Possess(Character); Teleport(FVector(-1500.f, -350.f, 92.f)); Advance(0.3f); break;
    case 48: Start = Character->GetActorLocation(); Key(EKeys::W, true); Advance(0.5f); break;
    case 49:
        Check(Character->GetActorLocation().X > Start.X + 60.f, TEXT("Repossess restores movement input"));
        Check(Combat->GetShotsFired() == ShotsBefore, TEXT("Repossess does not resume stale held fire"));
        Key(EKeys::W, false); Capture(TEXT("motion_repossess.png"));
        Stage = 52; Elapsed = 0.f; NextDelay = 0.3f; break;
    case 50:
        Key(EKeys::R, false);
        Check(Combat->IsFireBlocked() && Combat->IsReloading(), TEXT("Protected high cover permits a safe reload"));
        Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.05f, Combat->PistolData->ReloadMontage);
        Advance(0.25f); break;
    case 51:
        Check(Combat->GetActionState() == EGunnerCombatAction::Idle && Combat->GetMagazine() == MagazineBefore && Combat->GetReserve() == ReserveBefore,
            TEXT("Protected reload interruption remains ammo-safe"));
        Key(EKeys::SpaceBar, true); Stage = 37; Elapsed = 0.f; NextDelay = 0.2f; break;
    case 52:
        ShoulderBefore = Character->GetShoulderSide(); Key(EKeys::Q, true); Advance(0.15f); break;
    case 53: Key(EKeys::Q, false); Advance(0.5f); break;
    case 54:
    {
        const auto* Boom = Character->FindComponentByClass<USpringArmComponent>();
        Check(Character->GetShoulderSide() * ShoulderBefore < 0.f, TEXT("Q switches selected shoulder"));
        Check(Boom && Boom->SocketOffset.Y * Character->GetShoulderSide() > 45.f,
            TEXT("Shoulder switch moves the live camera to the selected side"));
        Capture(TEXT("motion_shoulder_swap.png")); Key(EKeys::C, true); Advance(0.15f); break;
    }
    case 55: Key(EKeys::C, false); Advance(0.3f); break;
    case 56:
        Check(Character->bIsCrouched && Anim && Anim->bCrouched &&
            FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 62.f),
            TEXT("Action guard fixture enters authored crouch state"));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Key(EKeys::F, true);
        Key(EKeys::Two, true); Advance(0.3f); break;
    case 57:
        Key(EKeys::LeftMouseButton, false); Key(EKeys::F, false);
        Key(EKeys::Two, false);
        Check(Combat->GetActionState() == EGunnerCombatAction::Idle && Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle,
            TEXT("Protected crouch rejects the unsupported upright equip animation"));
        Check(Combat->GetShotsFired() == ShotsBefore, TEXT("Crouched hip fire is rejected without an authored firing pose"));
        Check(!Combat->IsMeleeing(), TEXT("Crouched melee is rejected without an authored attack pose"));
        Key(EKeys::RightMouseButton, true); Advance(0.5f); break;
    case 58:
        Check(Combat->IsAiming() && Character->bIsCrouched && Anim && Anim->bAiming && Anim->bCrouched,
            TEXT("Stationary crouched ADS preserves crouch and aiming animation state"));
        Start = Character->GetActorLocation(); Key(EKeys::LeftMouseButton, true); Key(EKeys::W, true); Advance(0.35f); break;
    case 59:
        Key(EKeys::LeftMouseButton, false); Key(EKeys::W, false);
        Check(Combat->GetShotsFired() > ShotsBefore, TEXT("Stationary crouched ADS permits fire"));
        Check(FVector::Dist2D(Character->GetActorLocation(), Start) < 2.f && Character->GetVelocity().Size2D() < 1.f,
            TEXT("Crouched ADS rejects translation until directional aim locomotion exists"));
        Capture(TEXT("motion_crouch_ads.png"));
        Key(EKeys::RightMouseButton, false); Key(EKeys::C, true); Advance(0.15f); break;
    case 60: Key(EKeys::C, false); Advance(0.35f); break;
    case 61:
        Check(!Character->bIsCrouched, TEXT("Action guard fixture returns to standing"));
        Start = Character->GetActorLocation(); Key(EKeys::F, true); Advance(0.08f); break;
    case 62:
        Key(EKeys::F, false); Check(Combat->IsMeleeing(), TEXT("Standing melee enters guarded attack state"));
        Key(EKeys::C, true); Key(EKeys::W, true); Advance(0.12f); break;
    case 63:
        Key(EKeys::C, false); Key(EKeys::W, false);
        Check(Combat->IsMeleeing() && !Character->bIsCrouched && !Movement->bWantsToCrouch,
            TEXT("Active standing melee rejects crouch toggle"));
        Check(FVector::Dist2D(Character->GetActorLocation(), Start) < 2.f && Character->GetVelocity().Size2D() < 1.f,
            TEXT("Active melee rejects movement input and stops translation"));
        Advance(MontageDuration(Combat->RifleData->MeleeMontage) + 0.1f); break;
    case 64:
        Check(!Combat->IsMeleeing() && Combat->GetActionState() == EGunnerCombatAction::Idle,
            TEXT("Guarded melee releases action lock on completion"));
        Check(Character->GetDodge() && Character->GetDodge()->RollMontage, TEXT("Dodge component and authored roll montage ready"));
        if (!Character->GetDodge() || !Character->GetDodge()->RollMontage) { Finish(); break; }
        Teleport(FVector(-1500.f, 350.f, 92.f)); Advance(0.3f); break;
    case 65:
        Start = Character->GetActorLocation(); Key(EKeys::E, true); Advance(0.3f); break;
    case 66:
        Key(EKeys::E, false);
        Check(Character->GetDodge()->IsDodging() && Movement->HasRootMotionSources() &&
            Character->GetMesh()->GetAnimInstance()->Montage_IsPlaying(Character->GetDodge()->RollMontage),
            TEXT("E starts the authored roll with a native root-motion source"));
        Check(FVector::Dist2D(Character->GetActorLocation(), Start) > 10.f &&
            FVector::Dist2D(Character->GetActorLocation(), Start) <= 355.f,
            TEXT("Active roll advances through the clear lane"));
        Capture(TEXT("motion_roll.png"));
        Advance(MontageDuration(Character->GetDodge()->RollMontage) + 0.3f); break;
    case 67:
        Check(!Character->GetDodge()->IsDodging() && !Movement->HasRootMotionSources() &&
            FVector::Dist2D(Character->GetActorLocation(), Start) >= 100.f &&
            FVector::Dist2D(Character->GetActorLocation(), Start) <= 355.f,
            TEXT("Completed roll travels a bounded distance and releases native motion"));
        Check(Character->GetVelocity().Size2D() < 1.f, TEXT("Completed roll leaves no residual slide"));
        Teleport(FVector(-1500.f, 350.f, 92.f));
        Blocker = SpawnBox(FVector(-1430.f, 350.f, 110.f), FVector(10.f, 250.f, 220.f));
        Check(Blocker != nullptr, TEXT("Close-wall roll clearance fixture spawned"));
        Advance(0.3f); break;
    case 68:
        Start = Character->GetActorLocation(); Key(EKeys::E, true); Advance(0.25f); break;
    case 69:
        Key(EKeys::E, false);
        Check(!Character->GetDodge()->IsDodging() && !Movement->HasRootMotionSources() &&
            FVector::Dist2D(Character->GetActorLocation(), Start) < 2.f,
            TEXT("Standing capsule sweep rejects a roll into a close wall"));
        if (Blocker) Blocker->Destroy(); Blocker = nullptr;
        Key(EKeys::C, true); Advance(0.15f); break;
    case 70: Key(EKeys::C, false); Advance(0.3f); break;
    case 71:
        Check(Character->bIsCrouched && Anim && Anim->bCrouched, TEXT("Roll stance fixture enters authored crouch"));
        Start = Character->GetActorLocation(); Key(EKeys::E, true); Advance(0.25f); break;
    case 72:
        Key(EKeys::E, false);
        Check(Character->bIsCrouched && !Character->GetDodge()->IsDodging() && !Movement->HasRootMotionSources() &&
            FVector::Dist2D(Character->GetActorLocation(), Start) < 2.f,
            TEXT("Crouched roll is rejected without a compatible transition"));
        Key(EKeys::C, true); Advance(0.15f); break;
    case 73: Key(EKeys::C, false); Advance(0.3f); break;
    case 74:
        Check(!Character->bIsCrouched, TEXT("Roll interruption fixture returns to standing"));
        Key(EKeys::E, true); Advance(0.2f); break;
    case 75:
        Key(EKeys::E, false);
        Check(Character->GetDodge()->IsDodging() && Movement->HasRootMotionSources(), TEXT("Second roll enters native motion before interruption"));
        Character->GetMesh()->GetAnimInstance()->Montage_Stop(0.05f, Character->GetDodge()->RollMontage);
        Advance(0.25f); break;
    case 76:
        Check(!Character->GetDodge()->IsDodging() && !Movement->HasRootMotionSources() && Character->GetVelocity().Size2D() < 1.f,
            TEXT("Interrupted roll removes native motion and releases action lock"));
        Start = Character->GetActorLocation(); Advance(0.25f); break;
    case 77:
        Check(FVector::Dist2D(Character->GetActorLocation(), Start) < 2.f,
            TEXT("Interrupted roll remains stopped after cleanup"));
        if (Character->GetShoulderSide() < 0.f) Key(EKeys::Q, true);
        Target->SetActorLocation(FVector(250.f, 1050.f, 130.f));
        Teleport(FVector(350.f, 550.f, 92.f), 90.f); Advance(0.3f); break;
    case 78:
        Key(EKeys::Q, false);
        Check(Character->GetShoulderSide() > 0.f, TEXT("High-cover edge fixture selects its open shoulder"));
        Key(EKeys::SpaceBar, true); Advance(0.25f); break;
    case 79:
        Key(EKeys::SpaceBar, false);
        Check(Cover->IsAttached() && !Cover->IsLowCover() &&
            FMath::IsNearlyEqual(Character->GetActorLocation().X, 350.f, 4.f) &&
            FMath::IsNearlyEqual(Character->GetActorLocation().Y, 598.f, 4.f),
            TEXT("High-cover edge attaches to the protected anchor"));
        Key(EKeys::RightMouseButton, true); Advance(1.f); break;
    case 80:
        Check(Cover->IsAttached() && Cover->IsPeeking() && Character->GetActorLocation().X < 300.f &&
            FMath::IsNearlyEqual(Character->GetActorLocation().Y, 598.f, 4.f),
            TEXT("High-cover ADS physically steps beyond the edge while retaining wall offset"));
        bTrackTarget = true; Advance(0.4f); break;
    case 81:
        Check(Combat->IsAiming() && Anim && Anim->bAiming && !Character->bIsCrouched,
            TEXT("Completed high-cover step-out evaluates standing aim state"));
        HitsBefore = Target->GetTotalHitCount(); ShotsBefore = Combat->GetShotsFired();
        Key(EKeys::LeftMouseButton, true); Advance(0.4f); break;
    case 82:
        Key(EKeys::LeftMouseButton, false);
        Check(Combat->GetShotsFired() > ShotsBefore && Target->GetTotalHitCount() > HitsBefore,
            TEXT("High-cover step-out clears the actual muzzle trace and damages the exposed target"));
        Capture(TEXT("motion_high_cover_fire.png"));
        bReturnWaitLogged = false;
        Key(EKeys::RightMouseButton, false); bTrackTarget = false;
        Player->SetControlRotation(FRotator(0.f, 90.f, 0.f)); Advance(1.f); break;
    case 83:
    {
        const bool bReturned = Cover->IsAttached() && !Cover->IsPeeking() && !Combat->IsAiming() &&
            FMath::IsNearlyEqual(Character->GetActorLocation().X, 350.f, 5.f) &&
            FMath::IsNearlyEqual(Character->GetActorLocation().Y, 598.f, 4.f);
        if (!bReturnWaitLogged || bReturned || !Cover->IsAttached() || Elapsed >= 2.6f)
        UE_LOG(LogTemp, Display, TEXT("GUNNER_COVER_RETURN elapsed=%.3f attached=%d peeking=%d aiming=%d blind=%d x=%.3f y=%.3f speed=%.3f"),
            Elapsed, Cover->IsAttached(), Cover->IsPeeking(), Combat->IsAiming(), Combat->IsBlindFiring(),
            Character->GetActorLocation().X, Character->GetActorLocation().Y, Character->GetVelocity().Size2D());
        bReturnWaitLogged = true;
        // Screenshot capture can stall a frame while native movement is settling.
        // Wait for the same state/anchor invariants, bounded by the runtime's
        // 2.5-second transition timeout; a detach is an immediate failure.
        if (!bReturned && Cover->IsAttached() && Elapsed < 2.6f) return;
        Check(bReturned,
            TEXT("Releasing high-cover ADS returns to the original protected anchor"));
        Teleport(FVector(0.f, -850.f, 92.f), 90.f); Advance(0.3f); break;
    }
    case 84: Key(EKeys::SpaceBar, true); Advance(0.2f); break;
    case 85:
        Key(EKeys::SpaceBar, false);
        Check(Cover->IsLowCover() && Character->bIsCrouched, TEXT("Low-cover melee guard fixture attaches and crouches"));
        Key(EKeys::RightMouseButton, true); Advance(0.45f); break;
    case 86:
        Check(Combat->IsAiming() && !Character->bIsCrouched, TEXT("Low-cover melee guard fixture pops up into standing ADS"));
        Key(EKeys::F, true); Key(EKeys::Two, true); Advance(0.2f); break;
    case 87:
        Key(EKeys::F, false);
        Key(EKeys::Two, false);
        Check(Combat->GetActionState() == EGunnerCombatAction::Idle && Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle,
            TEXT("Low-cover pop-up rejects equip that would force an unsupported protected pose"));
        Check(!Combat->IsMeleeing() && Combat->IsAiming() && !Character->bIsCrouched,
            TEXT("Low-cover pop-up rejects standing melee without triggering automatic crouch"));
        Key(EKeys::RightMouseButton, false); Key(EKeys::SpaceBar, true); Advance(0.2f); break;
    case 88:
        Key(EKeys::SpaceBar, false); Teleport(FVector(-1500.f, 350.f, 92.f)); Advance(0.3f); break;
    case 89:
        Check(!Character->bIsCrouched && !Movement->bWantsToCrouch, TEXT("Simultaneous action fixture starts standing"));
        Key(EKeys::C, true); Key(EKeys::F, true); Advance(0.15f); break;
    case 90:
        Key(EKeys::C, false); Key(EKeys::F, false);
        // Enhanced Input may order these actions either way; neither order may combine
        // a standing full-body attack with a requested or active crouched capsule.
        Check(!(Combat->IsMeleeing() && (Character->bIsCrouched || Movement->bWantsToCrouch)),
            TEXT("Same-frame crouch and melee inputs cannot combine incompatible stance and attack"));
        Advance(MontageDuration(Combat->RifleData->MeleeMontage) + 0.15f); break;
    case 91:
        Check(!Combat->IsMeleeing(), TEXT("Simultaneous action handling leaves no lingering melee lock"));
        Finish(); break;
    default: Finish(); break;
    }
#endif
}
