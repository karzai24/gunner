#include "GunnerGameMode.h"
#include "Tests/GunnerFoundationProbe.h"
#include "Tests/GunnerMotionProbe.h"
#include "Tests/GunnerLookProbe.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"
#include "GunnerCharacter.h"
#include "GunnerPlayerController.h"
AGunnerGameMode::AGunnerGameMode()
{
    DefaultPawnClass = AGunnerCharacter::StaticClass();
    PlayerControllerClass = AGunnerPlayerController::StaticClass();
}

void AGunnerGameMode::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("GunnerLookSmoke")))
        GetWorld()->SpawnActor<AGunnerLookProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("GunnerMotionSmoke")))
        GetWorld()->SpawnActor<AGunnerMotionProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("GunnerSmoke")))
        GetWorld()->SpawnActor<AGunnerFoundationProbe>();
#endif
}
