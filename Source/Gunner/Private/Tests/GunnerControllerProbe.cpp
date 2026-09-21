#include "Tests/GunnerControllerProbe.h"
#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerDodgeComponent.h"
#include "GunnerInputConfig.h"
#include "GunnerPlayerController.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
    constexpr float FloorZ = 400.f;
    const FVector OpenPosition(0.f, -700.f, FloorZ + 92.f);
}

AGunnerControllerProbe::AGunnerControllerProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}

void AGunnerControllerProbe::Key(FKey InputKey, bool bPressed)
{
    if (!Player) return;
    if (bPressed) Held.AddUnique(InputKey); else Held.Remove(InputKey);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void AGunnerControllerProbe::Axes(float DeltaSeconds)
{
    if (!Player) return;
    // Inject the same four 1D keys reported by Unreal's platform gamepad layer.
    // A 2D action injection would bypass the authored swizzle/dead-zone mappings.
    const auto Send = [this, DeltaSeconds](FKey InputKey, float Value)
    {
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
            Value, DeltaSeconds, 1, FPlatformTime::Cycles64()));
    };
    Send(EKeys::Gamepad_LeftX, MoveStick.X);
    Send(EKeys::Gamepad_LeftY, MoveStick.Y);
    Send(EKeys::Gamepad_RightX, LookStick.X);
    Send(EKeys::Gamepad_RightY, LookStick.Y);
}

void AGunnerControllerProbe::ReleaseInput()
{
    MoveStick = LookStick = FVector2D::ZeroVector;
    while (!Held.IsEmpty()) Key(Held.Last(), false);
    Axes(1.f / 60.f);
}

void AGunnerControllerProbe::Advance(float NextDelay)
{
    ++Stage;
    Elapsed = InjectedTime = 0.f;
    Delay = NextDelay;
}

void AGunnerControllerProbe::Check(bool bPass, const TCHAR* Label)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_CONTROLLER_CHECK PASS stage=%d %s"), Stage, Label); }
    else { ++Failures; UE_LOG(LogTemp, Error, TEXT("GUNNER_CONTROLLER_CHECK FAIL stage=%d %s"), Stage, Label); }
}

void AGunnerControllerProbe::Capture(const TCHAR* Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name, false, false);
}

float AGunnerControllerProbe::MontageDuration(const UAnimMontage* Montage) const
{
    return Montage ? Montage->GetPlayLength() / FMath::Max(.01f, Montage->RateScale) : 1.f;
}

AStaticMeshActor* AGunnerControllerProbe::SpawnBox(FVector Location, FVector Size)
{
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Box = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
    if (!Box) return nullptr;
    auto* Mesh = Box->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(Cube);
    Mesh->SetWorldScale3D(Size / 100.f);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetMobility(EComponentMobility::Static);
    Fixtures.Add(Box);
    return Box;
}

void AGunnerControllerProbe::ResetPawn()
{
    ReleaseInput();
    Character->GetCover()->Detach();
    Character->GetDodge()->CancelDodge();
    Character->GetCombat()->StopAllActions();
    Character->StopJumpPresentation();
    Character->UnCrouch();
    auto* Movement = Character->GetCharacterMovement();
    Movement->StopMovementImmediately();
    FVector Location = OpenPosition;
    Location.Z = FloorZ + Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
    Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0.f, 90.f, 0.f));
    Player->SetControlRotation(FRotator(-5.f, 90.f, 0.f));
    Movement->SetMovementMode(MOVE_Walking);
}

void AGunnerControllerProbe::Cleanup()
{
    ReleaseInput();
    if (Character)
    {
        Character->GetCover()->Detach();
        Character->GetDodge()->CancelDodge();
        Character->GetCombat()->StopAllActions();
        Character->StopJumpPresentation();
    }
    for (auto& Fixture : Fixtures) if (Fixture) Fixture->Destroy();
    Fixtures.Reset();
    InputGuard.Restore();
}

void AGunnerControllerProbe::Finish()
{
    Cleanup();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_CONTROLLER_COMPLETE failures=%d input=synthetic physical_controller_verified=false"), Failures);
    SetActorTickEnabled(false);
}

void AGunnerControllerProbe::EndPlay(const EEndPlayReason::Type Reason)
{
    Cleanup();
    Super::EndPlay(Reason);
}

void AGunnerControllerProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    InputGuard.Begin(GetWorld());
    Total += DeltaSeconds;
    if (Total > 110.f)
    {
        Check(false, TEXT("Bounded native controller probe timed out"));
        Finish();
        return;
    }
    Axes(DeltaSeconds);
    if (!LookStick.IsNearlyZero()) InjectedTime += DeltaSeconds;
    Elapsed += DeltaSeconds;
    if (Elapsed < Delay) return;

    if (Stage == 0)
    {
        // Only the explicit single-player diagnostic assumes the first controller.
        Player = Cast<AGunnerPlayerController>(GetWorld()->GetFirstPlayerController());
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        const bool bReady = Character && Character->GetInputConfig() && Character->GetCombat() &&
            Character->GetCombat()->RifleData && Character->GetCombat()->PistolData &&
            Character->GetCover() && Character->GetDodge() && Character->GetMesh()->GetAnimInstance();
        Check(bReady, TEXT("Motion pawn, controller, input, weapons and animation ready"));
        if (!bReady) { Finish(); return; }
        Check(Character->UsesContextualTraversal(), TEXT("Installed movement profile enables contextual A tap/hold"));
        Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Check(Cube && SpawnBox(FVector(0.f, -400.f, FloorZ - 10.f), FVector(2800.f, 2800.f, 20.f)),
            TEXT("Transient supported test floor spawned"));
        if (!Cube || Fixtures.IsEmpty()) { Finish(); return; }
        Player->FlushPressedKeys();
        ResetPawn();
        UE_LOG(LogTemp, Display, TEXT("GUNNER_CONTROLLER_BEGIN physical gamepad key/axis mappings; OS delivery and hardware pairing unverified"));
        Advance(.4f);
        return;
    }

    auto* Combat = Character->GetCombat();
    auto* Cover = Character->GetCover();
    auto* Movement = Character->GetCharacterMovement();
    auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    switch (Stage)
    {
    case 1:
        Key(EKeys::K, true); Key(EKeys::K, false); // Unmapped key selects keyboard hints without a gameplay action.
        Check(!Player->IsUsingGamepad(), TEXT("Keyboard activity selects keyboard HUD hints"));
        Start = Character->GetActorLocation(); BeforeLook = Player->GetControlRotation();
        MoveStick = LookStick = FVector2D(.1f, -.1f);
        Advance(.4f); break;
    case 2:
        Check(FVector::Dist2D(Start, Character->GetActorLocation()) < 1.f && Character->GetVelocity().Size2D() < 1.f,
            TEXT("Small stick drift stays inside movement dead zone"));
        Check(Player->GetControlRotation().Equals(BeforeLook, .05f), TEXT("Small stick drift does not rotate camera"));
        Check(!Player->IsUsingGamepad(), TEXT("Small stick drift does not flicker HUD into controller hints"));
        MoveStick = FVector2D(0.f, .5f); LookStick = FVector2D::ZeroVector;
        Start = Character->GetActorLocation();
        Advance(.8f); break;
    case 3:
        PartialSpeed = Character->GetVelocity().Size2D();
        Check(PartialSpeed > 25.f && Character->GetActorLocation().Y > Start.Y + 15.f,
            TEXT("Partial left-stick forward produces real forward motion"));
        Check(Player->IsUsingGamepad(), TEXT("Intentional stick activity selects controller HUD hints"));
        MoveStick.Y = 1.f;
        Advance(.8f); break;
    case 4:
        UE_LOG(LogTemp, Display, TEXT("GUNNER_CONTROLLER_ANALOG partial_speed=%.3f full_speed=%.3f"), PartialSpeed, Character->GetVelocity().Size2D());
        Check(Character->GetVelocity().Size2D() > PartialSpeed * 1.6f,
            TEXT("Full stick is faster than partial stick; analog magnitude is retained"));
        MoveStick = FVector2D::ZeroVector;
        Advance(.3f); break;
    case 5:
        Check(Character->GetVelocity().Size2D() < 1.f, TEXT("Releasing left stick stops movement"));
        BeforeLook = Player->GetControlRotation(); LookStick = FVector2D(.7f, .4f);
        Advance(.45f); break;
    case 6:
    {
        const FRotator After = Player->GetControlRotation();
        const float Yaw = FMath::FindDeltaAngleDegrees(BeforeLook.Yaw, After.Yaw);
        FreeLookRate = Yaw / FMath::Max(.01f, InjectedTime);
        Check(Yaw > 5.f, TEXT("Right stick right turns view right"));
        Check(FMath::FindDeltaAngleDegrees(BeforeLook.Pitch, After.Pitch) < -2.f &&
            After.Vector().Z < BeforeLook.Vector().Z, TEXT("Right stick up lowers view with inverted pitch"));
        Check(Player->PlayerCameraManager && Player->PlayerCameraManager->GetCameraRotation().Equals(After, .5f),
            TEXT("Rendered camera follows controller rotation"));
        LookStick = FVector2D::ZeroVector;
        Key(EKeys::Gamepad_LeftTrigger, true);
        Advance(.35f); break;
    }
    case 7:
        Check(Combat->IsAiming() && Character->IsAimHeld(), TEXT("Held LT enters shoulder ADS"));
        Capture(TEXT("controller_ads_hud.png"));
        Advance(.3f); break;
    case 8:
        BeforeLook = Player->GetControlRotation(); LookStick = FVector2D(.7f, .4f);
        Advance(.45f); break;
    case 9:
    {
        const float Rate = FMath::FindDeltaAngleDegrees(BeforeLook.Yaw, Player->GetControlRotation().Yaw) / FMath::Max(.01f, InjectedTime);
        const float Ratio = Rate / FMath::Max(.01f, FreeLookRate);
        UE_LOG(LogTemp, Display, TEXT("GUNNER_CONTROLLER_LOOK free_rate=%.3f ads_rate=%.3f ratio=%.3f configured_scale=%.3f"),
            FreeLookRate, Rate, Ratio, Character->GetInputConfig()->StickAimSensitivityScale);
        Check(FMath::Abs(Ratio - Character->GetInputConfig()->StickAimSensitivityScale) < .15f,
            TEXT("ADS stick look uses configured lower sensitivity"));
        LookStick = FVector2D::ZeroVector;
        Player->SetControlRotation(FRotator(-5.f, 90.f, 0.f));
        ShotsBefore = Combat->GetShotsFired(); Key(EKeys::Gamepad_RightTrigger, true);
        Advance(.6f); break;
    }
    case 10:
        Check(Combat->IsAiming() && Combat->GetShotsFired() >= ShotsBefore + 3, TEXT("Held RT fires rifle repeatedly while LT aims"));
        Key(EKeys::Gamepad_RightTrigger, false); ShotsBefore = Combat->GetShotsFired();
        Advance(.3f); break;
    case 11:
        Check(Combat->GetShotsFired() == ShotsBefore, TEXT("Releasing RT stops automatic fire"));
        Key(EKeys::Gamepad_LeftTrigger, false);
        Advance(.25f); break;
    case 12:
        Check(!Combat->IsAiming() && !Character->IsAimHeld(), TEXT("Releasing LT exits ADS"));
        BeforeLook = Player->GetControlRotation(); LookStick = FVector2D(.7f, .4f);
        Advance(.45f); break;
    case 13:
    {
        const float Rate = FMath::FindDeltaAngleDegrees(BeforeLook.Yaw, Player->GetControlRotation().Yaw) / FMath::Max(.01f, InjectedTime);
        Check(Rate > FreeLookRate * .8f && Rate < FreeLookRate * 1.2f,
            TEXT("Free camera look returns after releasing ADS"));
        LookStick = FVector2D::ZeroVector;
        MagazineBefore = Combat->GetMagazine(); ReserveBefore = Combat->GetReserve();
        Key(EKeys::Gamepad_FaceButton_Left, true);
        Advance(.2f); break;
    }
    case 14:
        Key(EKeys::Gamepad_FaceButton_Left, false);
        Check(Combat->IsReloading() && Combat->GetMagazine() == MagazineBefore, TEXT("X begins animated reload before ammo commits"));
        Advance(MontageDuration(Combat->GetWeaponData()->ReloadMontage) + .2f); break;
    case 15:
        Check(!Combat->IsReloading() && Combat->GetMagazine() > MagazineBefore && Combat->GetReserve() < ReserveBefore,
            TEXT("Controller reload completes and transfers ammunition"));
        Key(EKeys::Gamepad_FaceButton_Top, true);
        Advance(.2f); break;
    case 16:
        Key(EKeys::Gamepad_FaceButton_Top, false);
        Check(Combat->GetActionState() == EGunnerCombatAction::Equipping, TEXT("Y starts animated weapon cycle"));
        Advance(MontageDuration(Combat->PistolData->EquipMontage) + .2f); break;
    case 17:
        Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol, TEXT("Y cycles rifle to pistol"));
        Key(EKeys::Gamepad_DPad_Up, true);
        Advance(.1f); break;
    case 18:
        Key(EKeys::Gamepad_DPad_Up, false);
        Advance(MontageDuration(Combat->RifleData->EquipMontage) + .2f); break;
    case 19:
        Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle, TEXT("D-pad up selects rifle"));
        Key(EKeys::Gamepad_DPad_Down, true);
        Advance(.1f); break;
    case 20:
        Key(EKeys::Gamepad_DPad_Down, false);
        Advance(MontageDuration(Combat->PistolData->EquipMontage) + .2f); break;
    case 21:
        Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol, TEXT("D-pad down selects pistol"));
        Key(EKeys::Gamepad_FaceButton_Top, true);
        Advance(.1f); break;
    case 22:
        Key(EKeys::Gamepad_FaceButton_Top, false);
        Advance(MontageDuration(Combat->RifleData->EquipMontage) + .2f); break;
    case 23:
        Check(Combat->GetWeaponKind() == EGunnerWeaponKind::Rifle, TEXT("Y cycles pistol back to rifle"));
        Key(EKeys::Gamepad_FaceButton_Right, true);
        Advance(.15f); break;
    case 24:
        Key(EKeys::Gamepad_FaceButton_Right, false);
        Check(Combat->IsMeleeing(), TEXT("B starts the authored melee action"));
        Advance(MontageDuration(Combat->GetWeaponData()->MeleeMontage) + .3f); break;
    case 25:
        Check(!Combat->IsMeleeing(), TEXT("Controller melee ends without an action lock"));
        Key(EKeys::Gamepad_RightThumbstick, true);
        Advance(.4f); break;
    case 26:
        Key(EKeys::Gamepad_RightThumbstick, false);
        Check(Character->bIsCrouched && Anim && Anim->bCrouched, TEXT("R3 toggles genuine crouch animation and capsule"));
        Capture(TEXT("controller_crouch_hud.png"));
        Advance(.3f); break;
    case 27:
        Key(EKeys::Gamepad_RightThumbstick, true);
        Advance(.4f); break;
    case 28:
        Key(EKeys::Gamepad_RightThumbstick, false);
        Check(!Character->bIsCrouched, TEXT("Second R3 restores standing stance"));
        ResetPawn();
        Advance(.3f); break;
    case 29:
        Start = Character->GetActorLocation();
        Key(EKeys::Gamepad_LeftShoulder, true);
        Advance(.12f); break;
    case 30:
        Key(EKeys::Gamepad_LeftShoulder, false);
        Check(Character->GetDodge()->IsDodging(), TEXT("LB starts an animated dodge roll"));
        Advance(MontageDuration(Character->GetDodge()->RollMontage) + .3f); break;
    case 31:
        Check(!Character->GetDodge()->IsDodging() && FVector::Dist2D(Start, Character->GetActorLocation()) > 100.f,
            TEXT("LB roll moves through CharacterMovement and settles"));
        ResetPawn();
        Advance(.3f); break;
    case 32:
        Key(EKeys::Gamepad_FaceButton_Bottom, true);
        Advance(.07f); break;
    case 33:
        Key(EKeys::Gamepad_FaceButton_Bottom, false);
        Advance(.12f); break;
    case 34:
        Check(Character->GetDodge()->IsDodging(), TEXT("Short A tap rolls in open space"));
        Advance(MontageDuration(Character->GetDodge()->RollMontage) + .3f); break;
    case 35:
        ResetPawn();
        Advance(.3f); break;
    case 36:
        MoveStick = FVector2D(0.f, 1.f); Key(EKeys::Gamepad_FaceButton_Bottom, true);
        Advance(.65f); break;
    case 37:
        Check(Character->IsSprinting() && Character->GetVelocity().Size2D() > 430.f && !Character->GetDodge()->IsDodging(),
            TEXT("Holding A with left stick sprints instead of rolling"));
        ReleaseInput();
        Advance(.3f); break;
    case 38:
        Check(!Character->IsSprinting() && Character->GetVelocity().Size2D() < 1.f && !Character->GetDodge()->IsDodging(),
            TEXT("Releasing a held A stops sprint without a release roll"));
        ResetPawn();
        Advance(.3f); break;
    case 39:
        MoveStick = FVector2D(0.f, 1.f); Key(EKeys::Gamepad_LeftThumbstick, true);
        Advance(.5f); break;
    case 40:
        Check(Character->IsSprinting() && Character->GetVelocity().Size2D() > 430.f, TEXT("Held L3 also supports sprint"));
        ReleaseInput();
        Advance(.3f); break;
    case 41:
        ShoulderBefore = Character->GetShoulderSide();
        Key(EKeys::Gamepad_RightShoulder, true);
        Advance(.2f); break;
    case 42:
        Key(EKeys::Gamepad_RightShoulder, false);
        Check(Character->GetShoulderSide() == -ShoulderBefore, TEXT("RB changes the camera shoulder"));
        Advance(.1f); break;
    case 43:
        Key(EKeys::Gamepad_RightShoulder, true); // A new input frame restores the tested blind-fire shoulder.
        Start = Character->GetActorLocation();
        Key(EKeys::Gamepad_DPad_Right, true);
        Advance(.16f); break;
    case 44:
        Key(EKeys::Gamepad_RightShoulder, false);
        Key(EKeys::Gamepad_DPad_Right, false);
        Check(Movement->IsFalling() && Character->GetActorLocation().Z > Start.Z + 10.f,
            TEXT("D-pad right starts separate weapon-specific jump"));
        Advance(1.1f); break;
    case 45:
        Check(Movement->IsMovingOnGround(), TEXT("Controller jump returns to a supported floor"));
        ResetPawn();
        Check(SpawnBox(FVector(0.f, -550.f, FloorZ + 57.5f), FVector(500.f, 40.f, 115.f)) != nullptr,
            TEXT("Transient 115 cm low-cover fixture spawned"));
        Advance(.3f); break;
    case 46:
        Key(EKeys::Gamepad_FaceButton_Bottom, true);
        Advance(.07f); break;
    case 47:
        Key(EKeys::Gamepad_FaceButton_Bottom, false);
        Advance(.7f); break;
    case 48:
        Check(Cover->IsAttached() && Cover->IsLowCover() && Character->bIsCrouched,
            TEXT("A near a barricade attaches to crouched low cover"));
        ShotsBefore = Combat->GetShotsFired();
        Key(EKeys::Gamepad_RightTrigger, true);
        Advance(.8f); break;
    case 49:
        Check(Cover->IsLowCover() && Character->bIsCrouched && Combat->IsBlindFiring() && Combat->GetShotsFired() > ShotsBefore,
            TEXT("RT fires over low cover while the character stays crouched"));
        Capture(TEXT("controller_cover_blind_fire_hud.png"));
        Key(EKeys::Gamepad_RightTrigger, false);
        Advance(.5f); break;
    case 50:
        Check(Cover->IsLowCover() && !Combat->IsBlindFiring() && !Combat->IsBlindFirePending(),
            TEXT("RT release restores protected cover state"));
        Check(Player->IsUsingGamepad(), TEXT("Controller HUD remains active through combat and traversal"));
        Capture(TEXT("controller_cover_idle_hud.png"));
        Advance(.3f); break;
    case 51:
        ResetPawn();
        Advance(.3f); break;
    case 52:
        Start = Character->GetActorLocation();
        MoveStick = FVector2D(.8f, 0.f);
        Advance(.4f); break;
    case 53:
        Check(Character->GetActorLocation().X < Start.X - 20.f &&
            FMath::Abs(Character->GetActorLocation().Y - Start.Y) < 2.f,
            TEXT("Left-stick right moves camera-relative right without forward-axis leakage"));
        MoveStick = FVector2D::ZeroVector;
        Advance(.3f); break;
    case 54:
        Check(Character->GetVelocity().Size2D() < 1.f, TEXT("Releasing lateral stick restores idle"));
        Finish(); break;
    }
#endif
}
