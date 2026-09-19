#include "GunnerPlayerController.h"
#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Camera/PlayerCameraManager.h"

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
