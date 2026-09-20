#include "Tests/GunnerBlindFireProbe.h"
#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerTarget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AGunnerBlindFireProbe::AGunnerBlindFireProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}
void AGunnerBlindFireProbe::Check(bool bPass, const TCHAR* Name)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_BLIND_CHECK PASS %s"), Name); }
    else { ++Failures; UE_LOG(LogTemp, Error, TEXT("GUNNER_BLIND_CHECK FAIL stage=%d %s"), Stage, Name); }
}
void AGunnerBlindFireProbe::Key(FKey InputKey, bool bPressed)
{
    if (!Player) return;
    if (bPressed) HeldKeys.AddUnique(InputKey); else HeldKeys.Remove(InputKey);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
void AGunnerBlindFireProbe::Advance(float Wait) { ++Stage; Elapsed = 0.f; Delay = Wait; }
void AGunnerBlindFireProbe::Capture(const TCHAR* Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name, false, false);
}
void AGunnerBlindFireProbe::TeleportToCover()
{
    Character->GetCombat()->StopAllActions();
    Character->GetCover()->Detach();
    Character->UnCrouch();
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->SetActorLocation(FVector(0.f, -850.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
        false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0.f, 90.f, 0.f));
    Player->SetControlRotation(FRotator(0.f, 90.f, 0.f));
}
void AGunnerBlindFireProbe::CheckPose()
{
    const auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    float Top = 0.f;
    Check(Character->GetCover()->GetLowCoverTop(Top) && FMath::IsNearlyEqual(Top, 115.f, 1.f), TEXT("Actual barricade top measured at 115 cm"));
    const FVector Head = Character->GetMesh()->GetSocketLocation(TEXT("head"));
    const FVector Right = Character->GetMesh()->GetSocketLocation(TEXT("hand_r"));
    const FVector Left = Character->GetMesh()->GetSocketLocation(TEXT("hand_l"));
    const FVector Muzzle = Character->GetCombat()->GetMuzzleLocation();
    const float LeftError = Anim ? FVector::Distance(Left, Character->GetMesh()->GetComponentTransform().TransformPosition(Anim->BlindLeftHandLocation)) : 999.f;
    UE_LOG(LogTemp, Display, TEXT("GUNNER_BLIND_POSE stage=%d top=%.2f head=%.2f right=%.2f left=%.2f muzzle=%.2f left_error=%.2f alpha=%.3f"),
        Stage, Top, Head.Z, Right.Z, Left.Z, Muzzle.Z, LeftError, Anim ? Anim->BlindFireAlpha : -1.f);
    UE_LOG(LogTemp, Display, TEXT("GUNNER_BLIND_STATE class=%s active=%d pending=%d reason=%s shots=%d"),
        Anim ? *Anim->GetClass()->GetPathName() : TEXT("None"), Character->GetCombat()->IsBlindFiring(),
        Character->GetCombat()->IsBlindFirePending(), *Character->GetCombat()->GetBlindFireWaitReason().ToString(),
        Character->GetCombat()->GetShotsFired());
    Check(Character->bIsCrouched && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 62.f), TEXT("Blind fire preserves actual crouch and 62 cm capsule"));
    Check(Head.Z + 5.f < Top, TEXT("Head stays protected below barricade top"));
    Check(Right.Z > Top + 4.f && Muzzle.Z > Top + 4.f, TEXT("Evaluated firing hand and attached muzzle clear barricade"));
    Check(Anim && Anim->BlindFireAlpha > 0.95f && Anim->UpperBodyWeight < 0.05f, TEXT("Blind arm animation is evaluated without replacing crouch torso"));
    Check(LeftError < 7.f, TEXT("Support hand reaches its actual weapon grip"));
}
void AGunnerBlindFireProbe::Cleanup()
{
    InputGuard.Restore();
    while (!HeldKeys.IsEmpty()) Key(HeldKeys.Last(), false);
    if (Character) Character->GetCombat()->StopAllActions();
    if (Blocker) Blocker->Destroy();
    if (Target) Target->Destroy();
    Blocker = nullptr; Target = nullptr;
}
void AGunnerBlindFireProbe::Finish()
{
    Cleanup();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_BLIND_COMPLETE failures=%d"), Failures);
    SetActorTickEnabled(false);
}
void AGunnerBlindFireProbe::EndPlay(const EEndPlayReason::Type Reason) { Cleanup(); Super::EndPlay(Reason); }
void AGunnerBlindFireProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    InputGuard.Begin(GetWorld());
    if (bTrackTarget && Player && Character && Target)
        if (const auto* Camera = Character->FindComponentByClass<UCameraComponent>())
            Player->SetControlRotation((Target->GetActorLocation() - Camera->GetComponentLocation()).Rotation());
    Elapsed += DeltaSeconds;
    if (Elapsed < Delay) return;
    if (Stage == 0)
    {
        Player = GetWorld()->GetFirstPlayerController(); // Explicitly single-player diagnostic.
        if (Player) Player->FlushPressedKeys();
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Check(Character != nullptr, TEXT("Motion pawn possessed"));
        if (!Character) { Finish(); return; }
        const auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
        Check(Anim && Anim->bBlindFirePoseReady, TEXT("Authored blind-fire graph is active"));
        Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Target = GetWorld()->SpawnActor<AGunnerTarget>(FVector(0.f, -350.f, 160.f), FRotator::ZeroRotator, Spawn);
        Check(Cube && Target, TEXT("Transient blind-fire target ready"));
        if (!Cube || !Target) { Finish(); return; }
        Target->TargetMesh->SetStaticMesh(Cube);
        Target->TargetMesh->SetRelativeScale3D(FVector(2.f, 0.6f, 2.f));
        Target->MaxDurability = 10000.f; Target->ResetTarget();
        TeleportToCover(); Advance(0.5f); return;
    }
    auto* Combat = Character->GetCombat();
    auto* Cover = Character->GetCover();
    const auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    switch (Stage)
    {
    case 1: Key(EKeys::SpaceBar, true); Advance(0.1f); break;
    case 2: Key(EKeys::SpaceBar, false); Advance(0.5f); break;
    case 3:
        Check(Cover->IsLowCover() && Character->bIsCrouched, TEXT("Low cover starts protected and crouched"));
        ShotsBefore = Combat->GetShotsFired(); HitsBefore = Target->GetTotalHitCount();
        Key(EKeys::LeftMouseButton, true); Advance(0.1f); break;
    case 4:
        Check(Combat->IsBlindFiring() && Combat->GetShotsFired() == ShotsBefore, TEXT("First shot waits for raised evaluated pose"));
        Advance(0.65f); break;
    case 5:
        CheckPose();
        Check(Combat->GetShotsFired() >= ShotsBefore + 2 && Target->GetTotalHitCount() > HitsBefore, TEXT("Held rifle blind fire repeatedly damages target beyond wall"));
        Capture(TEXT("blind_rifle_fire.png")); Start = Character->GetActorLocation();
        Key(EKeys::W, true); Advance(0.3f); break;
    case 6:
        Check(FVector::Dist2D(Start, Character->GetActorLocation()) < 2.f, TEXT("Blind fire blocks travel while arms are raised"));
        Key(EKeys::W, false); Key(EKeys::LeftMouseButton, false);
        ShotsBefore = Combat->GetShotsFired(); Advance(0.5f); break;
    case 7:
        Check(!Combat->IsBlindFiring() && Combat->GetShotsFired() == ShotsBefore && Anim && Anim->BlindFireAlpha < 0.05f,
            TEXT("Rifle release stops fire and lowers arms"));
        Capture(TEXT("blind_rifle_released.png"));
        Key(EKeys::LeftMouseButton, true); Advance(0.03f); break;
    case 8: Key(EKeys::LeftMouseButton, false); Advance(0.85f); break;
    case 9:
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Quick rifle click queues exactly one raised shot"));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.03f); break;
    case 10: Key(EKeys::SpaceBar, true); Key(EKeys::LeftMouseButton, false); Advance(0.3f); break;
    case 11:
        Key(EKeys::SpaceBar, false);
        Check(!Cover->IsAttached() && !Combat->IsBlindFiring() && Combat->GetShotsFired() == ShotsBefore, TEXT("Detach cancels pending blind-fire shot"));
        TeleportToCover(); Advance(0.4f); break;
    case 12: Key(EKeys::Two, true); Advance(2.f); break;
    case 13:
        Key(EKeys::Two, false); Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol, TEXT("Pistol equipped for second fixture"));
        TeleportToCover(); Advance(0.35f); break;
    case 14: Key(EKeys::SpaceBar, true); Advance(0.15f); break;
    case 15: Key(EKeys::SpaceBar, false); Advance(0.45f); break;
    case 16:
        ShotsBefore = Combat->GetShotsFired(); HitsBefore = Target->GetTotalHitCount();
        Key(EKeys::LeftMouseButton, true); Advance(0.65f); break;
    case 17:
        CheckPose();
        Check(Combat->GetShotsFired() == ShotsBefore + 1 && Target->GetTotalHitCount() == HitsBefore + 1, TEXT("Pistol blind fire commits one target hit"));
        Capture(TEXT("blind_pistol_fire.png")); Advance(0.4f); break;
    case 18:
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Held pistol does not repeat blind fire"));
        Key(EKeys::LeftMouseButton, false); Advance(0.4f); break;
    case 19: ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.03f); break;
    case 20: Key(EKeys::LeftMouseButton, false); Advance(0.8f); break;
    case 21:
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Quick pistol click queues exactly one raised shot"));
        Key(EKeys::RightMouseButton, true); Advance(0.55f); break;
    case 22:
        Check(Combat->IsAiming() && !Character->bIsCrouched && !Combat->IsBlindFiring(), TEXT("RMB still enters existing standing low-cover ADS"));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.3f); break;
    case 23:
        Check(Combat->GetShotsFired() == ShotsBefore + 1, TEXT("Normal cover ADS still fires"));
        Key(EKeys::LeftMouseButton, false); Key(EKeys::RightMouseButton, false); Advance(0.45f); break;
    case 24:
        Check(Character->bIsCrouched && !Combat->IsAiming(), TEXT("ADS release restores protected crouch"));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.03f); break;
    case 25: Player->UnPossess(); Key(EKeys::LeftMouseButton, false); Advance(0.35f); break;
    case 26:
        Check(!Combat->IsBlindFiring() && Combat->GetShotsFired() == ShotsBefore, TEXT("Unpossession clears pending shot without delayed damage"));
        Player->Possess(Character); TeleportToCover(); Advance(0.4f); break;
    case 27: Key(EKeys::SpaceBar, true); Advance(0.15f); break;
    case 28: Key(EKeys::SpaceBar, false); Advance(0.5f); break;
    case 29:
    {
        FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient;
        Blocker = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0.f, -580.f, 160.f), FRotator::ZeroRotator, Spawn);
        Check(Blocker != nullptr, TEXT("Intervening wall fixture spawned"));
        if (Blocker)
        {
            auto* Mesh = Blocker->GetStaticMeshComponent(); Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(Cube); Mesh->SetWorldScale3D(FVector(3.f, 0.2f, 2.f));
            Mesh->SetCollisionProfileName(TEXT("BlockAll")); Mesh->SetMobility(EComponentMobility::Static);
        }
        ShotsBefore = Combat->GetShotsFired(); HitsBefore = Target->GetTotalHitCount();
        Key(EKeys::LeftMouseButton, true); Advance(0.7f); break;
    }
    case 30:
        Check(Combat->GetShotsFired() == ShotsBefore + 1 && Target->GetTotalHitCount() == HitsBefore,
            TEXT("Raised muzzle cannot damage through intervening geometry"));
        Key(EKeys::LeftMouseButton, false); if (Blocker) Blocker->Destroy(); Blocker = nullptr;
        bTrackTarget = false; Player->SetControlRotation(FRotator(0.f, -90.f, 0.f)); Advance(0.4f); break;
    case 31: ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(1.1f); break;
    case 32:
        Check(Combat->GetShotsFired() == ShotsBefore && !Combat->IsBlindFiring(), TEXT("Looking away from cover rejects overhead firing without consuming ammo"));
        Key(EKeys::LeftMouseButton, false); bTrackTarget = true; Advance(0.4f); break;
    case 33: ShotsBefore = Combat->GetShotsFired(); Key(EKeys::LeftMouseButton, true); Advance(0.03f); break;
    case 34: Key(EKeys::RightMouseButton, true); Key(EKeys::LeftMouseButton, false); Advance(0.45f); break;
    case 35:
        Check(Combat->GetShotsFired() == ShotsBefore && Combat->IsAiming() && !Combat->IsBlindFiring(),
            TEXT("ADS cancels a queued blind shot before exposure"));
        Key(EKeys::RightMouseButton, false); Advance(0.3f); break;
    case 36: Finish(); break;
    default: Check(false, TEXT("Unexpected diagnostic stage")); Finish(); break;
    }
#endif
}
