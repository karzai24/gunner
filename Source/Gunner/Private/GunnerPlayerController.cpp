#include "GunnerPlayerController.h"
#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/InputSettings.h"
#include "InputKeyEventArgs.h"

bool AGunnerPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    // Observe the same owning-player stream that Unreal forwards to Enhanced Input.
    // Its return value describes legacy binding handling, not input acceptance.
    const bool bHandled = Super::InputKey(Params);
    if (!IsLocalController() || !GetLocalPlayer() || !PlayerInput || !Params.Key.IsValid()
        || Params.bIsTouchEvent || Params.Key.IsTouch()
        || (GetDefault<UInputSettings>()->bFilterInputByPlatformUser
            && Params.GetPlatformUser() != GetPlatformUserId()))
        return bHandled;

    if (Params.Event == IE_Released) return bHandled;

    const bool bGamepad = Params.IsGamepad();
    bool bMeaningful = false;
    if (Params.Key.IsAnalog() || Params.Key.IsButtonAxis() || Params.Event == IE_Axis)
    {
        const float Magnitude = FMath::Max(FMath::Abs(Params.AmountDepressed),
            static_cast<float>(Params.AmountDepressed2D.Size()));
        if (bGamepad)
            bMeaningful = Magnitude > 0.2f;
        else if (Params.Key == EKeys::MouseX || Params.Key == EKeys::MouseY || Params.Key == EKeys::Mouse2D)
            bMeaningful = Params.NumSamples > 0 && Magnitude >= 1.f;
        else if (Params.Key == EKeys::MouseWheelAxis)
            bMeaningful = Magnitude > 0.01f;
    }
    else
        bMeaningful = Params.Event == IE_Pressed || Params.Event == IE_Repeat || Params.Event == IE_DoubleClick;

    // Neutral sticks, trigger releases and sub-pixel mouse noise must not flicker prompts.
    if (bMeaningful) bUsingGamepad = bGamepad;
    return bHandled;
}

void AGunnerPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    RefreshInputContext();
}
void AGunnerPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    RefreshInputContext();
}
void AGunnerPlayerController::AcknowledgePossession(APawn* InPawn)
{
    Super::AcknowledgePossession(InPawn);
    RefreshInputContext();
}
void AGunnerPlayerController::OnUnPossess()
{
    RemoveInputContext();
    Super::OnUnPossess();
}
void AGunnerPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveInputContext();
    Super::EndPlay(EndPlayReason);
}
void AGunnerPlayerController::RemoveInputContext()
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        if (auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            if (ActiveContext) Input->RemoveMappingContext(ActiveContext);
    ActiveContext = nullptr;
}
void AGunnerPlayerController::RefreshInputContext()
{
    RemoveInputContext();
    if (!IsLocalController()) return;
    const auto* Character = Cast<AGunnerCharacter>(GetPawn());
    const auto* Config = Character ? Character->GetInputConfig() : nullptr;
    if (!Config || !Config->MappingContext) return;
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        if (auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            ActiveContext = Config->MappingContext;
            Input->AddMappingContext(ActiveContext, 0);
            SetInputMode(FInputModeGameOnly());
            bShowMouseCursor = false;
        }
}

void AGunnerPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = -60.f;
        PlayerCameraManager->ViewPitchMax = 50.f;
    }
}
