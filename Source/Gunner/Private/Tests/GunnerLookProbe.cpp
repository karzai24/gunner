#include "Tests/GunnerLookProbe.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace GunnerLookProbe
{
    constexpr int32 StepCount = 15;
    const TCHAR* Names[StepCount] = {
        TEXT("rifle_free"), TEXT("rifle_ads"), TEXT("rifle_released"), TEXT("rifle_ads_repeat"), TEXT("rifle_released_repeat"),
        TEXT("pistol_free"), TEXT("pistol_ads"), TEXT("pistol_released"),
        TEXT("crouched_free"), TEXT("crouched_ads"), TEXT("crouched_released"),
        TEXT("cover_free"), TEXT("cover_ads"), TEXT("cover_released"), TEXT("cover_detached")
    };
    bool ExpectsAim(int32 Index) { return Index == 1 || Index == 3 || Index == 6 || Index == 9 || Index == 12; }
}

AGunnerLookProbe::AGunnerLookProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}

void AGunnerLookProbe::Check(bool bPassed, const TCHAR* Detail)
{
    const TCHAR* Name = Step >= 0 && Step < GunnerLookProbe::StepCount ? GunnerLookProbe::Names[Step] : TEXT("setup");
    if (bPassed) { UE_LOG(LogTemp, Display, TEXT("GUNNER_LOOK_CHECK PASS %s: %s"), Name, Detail); }
    else { ++Failures; UE_LOG(LogTemp, Error, TEXT("GUNNER_LOOK_CHECK FAIL %s: %s"), Name, Detail); }
}

void AGunnerLookProbe::Key(FKey InputKey, bool bPressed)
{
    if (!Player) return;
    if (bPressed) HeldKeys.AddUnique(InputKey); else HeldKeys.Remove(InputKey);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void AGunnerLookProbe::ConfigureStep()
{
    Phase = 0;
    Elapsed = 0.f;
    Wait = 0.35f;
    Key(EKeys::RightMouseButton, GunnerLookProbe::ExpectsAim(Step));
    if (Step == 5)
    {
        Key(EKeys::Two, true);
        const UAnimMontage* Equip = Character->GetCombat()->PistolData->EquipMontage;
        Wait = Equip ? Equip->GetPlayLength() / FMath::Max(0.01f, Equip->RateScale) + 0.25f : 1.5f;
    }
    if (Step == 8) Key(EKeys::C, true);
    if (Step == 11)
    {
        Character->GetCover()->Detach();
        Character->GetCombat()->StopAllActions();
        Character->GetCharacterMovement()->StopMovementImmediately();
        Character->UnCrouch();
        Character->SetActorLocation(FVector(0.f, -850.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f),
            false, nullptr, ETeleportType::TeleportPhysics);
        Character->SetActorRotation(FRotator(0.f, 90.f, 0.f));
        Player->SetControlRotation(FRotator(-5.f, 90.f, 0.f));
        Key(EKeys::SpaceBar, true);
        Wait = 0.5f;
    }
    if (Step == 14) Key(EKeys::SpaceBar, true);
}

void AGunnerLookProbe::InjectLook()
{
    for (const FKey Pulse : { EKeys::Two, EKeys::C, EKeys::SpaceBar })
        if (HeldKeys.Contains(Pulse)) Key(Pulse, false);
    Check(Character->GetCombat()->IsAiming() == GunnerLookProbe::ExpectsAim(Step), TEXT("Expected ADS transition completed"));
    Check(Character->GetCombat()->GetWeaponKind() == (Step < 5 ? EGunnerWeaponKind::Rifle : EGunnerWeaponKind::Pistol),
        TEXT("Expected weapon active"));
    if (Step >= 8 && Step <= 10) Check(Character->bIsCrouched, TEXT("Crouched look fixture remains crouched"));
    if (Step >= 11 && Step <= 13) Check(Character->IsInCover(), TEXT("Cover look fixture remains attached"));
    if (Step == 14) Check(!Character->IsInCover(), TEXT("Cover detach completed"));
    BeforeLook = Player->GetControlRotation();
    const float Sign = Step % 2 == 0 ? 1.f : -1.f;
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseX,
        Sign * 10.f, 1.f / 60.f, 1, FPlatformTime::Cycles64()));
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseY,
        Sign * 5.f, 1.f / 60.f, 1, FPlatformTime::Cycles64()));
    Phase = 1;
    Elapsed = 0.f;
    Wait = 0.25f;
}

void AGunnerLookProbe::CheckLook()
{
    const FRotator Control = Player->GetControlRotation();
    const FRotator View = Player->PlayerCameraManager ? Player->PlayerCameraManager->GetCameraRotation() : Camera->GetComponentRotation();
    const float Sign = Step % 2 == 0 ? 1.f : -1.f;
    Check(FMath::FindDeltaAngleDegrees(BeforeLook.Yaw, Control.Yaw) * Sign > 0.1f,
        TEXT("Mouse X still changes controller yaw in the requested direction"));
    Check(FMath::FindDeltaAngleDegrees(BeforeLook.Pitch, Control.Pitch) * Sign > 0.1f,
        TEXT("Mouse Y still changes controller pitch in the requested direction"));
    Check(FMath::Abs(FMath::FindDeltaAngleDegrees(Control.Yaw, View.Yaw)) < 0.5f &&
        FMath::Abs(FMath::FindDeltaAngleDegrees(Control.Pitch, View.Pitch)) < 0.5f,
        TEXT("Rendered camera rotation follows the controller"));
    Check(!Player->IsLookInputIgnored() && !Player->bShowMouseCursor, TEXT("Look remains enabled and game cursor remains hidden"));
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") /
        FString::Printf(TEXT("look_%s.png"), GunnerLookProbe::Names[Step]), false, false);
    Phase = 2;
    Elapsed = 0.f;
    Wait = 0.08f; // Let the requested gameplay view render before preparing the next fixture.
}

void AGunnerLookProbe::ReleaseKeys()
{
    while (!HeldKeys.IsEmpty()) Key(HeldKeys.Last(), false);
}

void AGunnerLookProbe::Finish()
{
    ReleaseKeys();
    UE_LOG(LogTemp, Display, TEXT("GUNNER_LOOK_COMPLETE failures=%d input=synthetic hardware_mouse_verified=false"), Failures);
    SetActorTickEnabled(false);
}

void AGunnerLookProbe::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseKeys();
    Super::EndPlay(Reason);
}

void AGunnerLookProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    Elapsed += DeltaSeconds;
    if (Elapsed < Wait) return;
    if (Step == -1)
    {
        Player = GetWorld()->GetFirstPlayerController(); // This opt-in diagnostic is explicitly single-player.
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
        Check(Character && Camera && Character->GetCombat() && Character->GetCombat()->PistolData,
            TEXT("Motion pawn, camera and weapon data are ready"));
        if (!Character || !Camera || !Character->GetCombat() || !Character->GetCombat()->PistolData) { Finish(); return; }
        UE_LOG(LogTemp, Display, TEXT("GUNNER_LOOK_BEGIN synthetic Enhanced Input only; physical macOS mouse capture requires separate verification"));
        Player->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
        Step = 0;
        ConfigureStep();
    }
    else if (Phase == 0) InjectLook();
    else if (Phase == 1) CheckLook();
    else
    {
        ++Step;
        if (Step == GunnerLookProbe::StepCount) Finish();
        else ConfigureStep();
    }
#endif
}
