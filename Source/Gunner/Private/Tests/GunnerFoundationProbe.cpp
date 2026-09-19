#include "Tests/GunnerFoundationProbe.h"
#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AGunnerFoundationProbe::AGunnerFoundationProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
#endif
}
void AGunnerFoundationProbe::Check(bool bPass, const TCHAR* Name)
{
    if (bPass) { UE_LOG(LogTemp, Display, TEXT("GUNNER_CHECK PASS %s"), Name); }
    else { ++Failures; UE_LOG(LogTemp, Error, TEXT("GUNNER_CHECK FAIL %s"), Name); }
}
void AGunnerFoundationProbe::Key(const FKey& InputKey, bool bPressed)
{
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InputKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
void AGunnerFoundationProbe::Capture(const TCHAR* Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name, false, false);
}
void AGunnerFoundationProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    Elapsed += DeltaSeconds;
    if (Elapsed < (Stage == -1 ? 5.f : (Stage == 5 ? 0.25f : 1.5f))) return;
    Elapsed = 0.f;
    if (Stage == -1)
    {
        Player = GetWorld()->GetFirstPlayerController();
        if (!Player) { Check(false, TEXT("Player controller ready")); Destroy(); return; }
        // Ignore desktop cursor position accumulated during window creation for the baseline.
        Player->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
    }
    else if (Stage == 0)
    {
        Player = GetWorld()->GetFirstPlayerController(); // Single-player test harness only.
        Character = Player ? Cast<AGunnerCharacter>(Player->GetPawn()) : nullptr;
        Check(Character != nullptr, TEXT("GameMode possesses Gunner character"));
        if (!Character) { Destroy(); return; }
        int32 Count = 0;
        for (TActorIterator<AGunnerCharacter> It(GetWorld()); It; ++It) ++Count;
        Check(Count == 1, TEXT("Exactly one player pawn"));
        Check(Character->GetMesh()->GetAnimInstance() != nullptr, TEXT("Animation Blueprint instantiated"));
        Check(FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),36.f)
            && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),90.f), TEXT("Standing capsule metrics"));
        Check(Character->GetCharacterMovement()->IsMovingOnGround(), TEXT("Spawn settles on floor"));
        auto* Input = Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        Check(Input && Character->GetInputConfig() && Input->HasMappingContext(Character->GetInputConfig()->MappingContext), TEXT("Local player context active"));
        Capture(TEXT("foundation_idle.png"));
        Start = Character->GetActorLocation();
        Key(EKeys::W, true);
    }
    else if (Stage == 1)
    {
        Check(Character->GetActorLocation().X > Start.X + 250.f, TEXT("W mapping drives forward movement"));
        Check(Character->GetVelocity().Size2D() > 100.f, TEXT("Moving velocity drives locomotion"));
        Capture(TEXT("foundation_move.png"));
        Key(EKeys::W, false);
    }
    else if (Stage == 2)
    {
        Check(Character->GetVelocity().Size2D() < 1.f, TEXT("Key release stops movement"));
        Start = Character->GetActorLocation();
        Key(EKeys::W, true); Key(EKeys::S, true);
    }
    else if (Stage == 3)
    {
        Check(FVector::Dist2D(Character->GetActorLocation(), Start) < 5.f, TEXT("Opposing keyboard axes cancel"));
        Key(EKeys::W,false); Key(EKeys::S,false);
        InitialYaw = Player->GetControlRotation().Yaw;
        InitialPitch = Player->GetControlRotation().Pitch;
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseX, 20.f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::MouseY, 20.f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
    }
    else if (Stage == 4)
    {
        Check(FMath::Abs(FMath::FindDeltaAngleDegrees(InitialYaw, Player->GetControlRotation().Yaw)) > 1.f, TEXT("Mouse look mapping rotates view"));
        Check(FMath::FindDeltaAngleDegrees(InitialPitch, Player->GetControlRotation().Pitch) > 0.1f, TEXT("Mouse up looks up"));
        Start = Character->GetActorLocation();
        Key(EKeys::SpaceBar,true);
    }
    else if (Stage == 5)
    {
        Check(Character->GetCharacterMovement()->IsFalling() && Character->GetActorLocation().Z > Start.Z + 15.f, TEXT("Traversal mapping performs jump"));
        Capture(TEXT("foundation_jump.png"));
        Key(EKeys::SpaceBar,false);
    }
    else if (Stage == 6)
    {
        Check(Character->GetCharacterMovement()->IsMovingOnGround(), TEXT("Jump lands cleanly"));
        Player->SetControlRotation(FRotator::ZeroRotator);
        Character->SetActorLocation(FVector(2070.f,0.f,92.f),false);
        Character->GetCharacterMovement()->StopMovementImmediately();
        Key(EKeys::W,true);
    }
    else if (Stage == 7)
    {
        Check(Character->GetActorLocation().X < 2170.f && Character->GetActorLocation().X > 2100.f, TEXT("Boundary wall blocks pawn"));
        Key(EKeys::W,false);
        Character->SetActorLocation(FVector(-2100.f,0.f,92.f),false);
        Character->GetCharacterMovement()->StopMovementImmediately();
    }
    else if (Stage == 8)
    {
        auto* Boom = Character->FindComponentByClass<USpringArmComponent>();
        Check(Boom && Boom->IsCollisionFixApplied(), TEXT("Shoulder camera retracts at wall"));
        Capture(TEXT("foundation_camera.png"));
    }
    else if (Stage == 9)
    {
        Player->UnPossess();
        auto* Input = Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        Check(!Input->HasMappingContext(Character->GetInputConfig()->MappingContext), TEXT("Unpossess removes owned context"));
        Character->SetActorLocation(FVector(-1500.f,0.f,92.f),false);
        Player->Possess(Character);
        Start = Character->GetActorLocation();
        Key(EKeys::D,true);
    }
    else if (Stage == 10)
    {
        Check(Character->GetActorLocation().Y > Start.Y + 250.f, TEXT("Repossess restores input and strafe mapping"));
        Key(EKeys::D,false);
        Capture(TEXT("foundation_repossess.png"));
    }
    else if (Stage == 11)
    {
        InitialPitch = Player->GetControlRotation().Pitch;
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Gamepad_RightY, 0.5f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
    }
    else if (Stage == 12)
    {
        Check(FMath::FindDeltaAngleDegrees(InitialPitch, Player->GetControlRotation().Pitch) > 1.f, TEXT("Right stick up looks up"));
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Gamepad_RightY, 0.f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
        Player->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
        Start = Character->GetActorLocation();
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Gamepad_LeftY, 1.f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
    }
    else if (Stage == 13)
    {
        Check(Character->GetActorLocation().X > Start.X + 250.f, TEXT("Left stick moves forward"));
        Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, EKeys::Gamepad_LeftY, 0.f, DeltaSeconds, 1, FPlatformTime::Cycles64()));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("GUNNER_SMOKE_COMPLETE failures=%d"), Failures);
        SetActorTickEnabled(false);
    }
    ++Stage;
#endif
}
