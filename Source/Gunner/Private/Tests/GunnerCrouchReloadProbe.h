#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerCrouchReloadProbe.generated.h"

class AGunnerCharacter;
class APlayerController;
class UAnimMontage;

/** Opt-in rendered regression. All ordinary actions enter through the owning player's input. */
UCLASS(NotBlueprintable, Transient)
class AGunnerCrouchReloadProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerCrouchReloadProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) int32 GetFailureCount() const { return Failures; }
private:
    enum class EPhase : uint8
    {
        Initialize, Settle, Equip, Spend, Stance, ReadinessGate, RaiseBlind,
        CancelReload, CancelSettled, AimBeforeReload, CompleteReload,
        CompletionSettled, FullMagazine, ReleaseAim, ToggleStand, ToggleCrouch,
        Move, AimAfterReload
    };
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<UAnimMontage> ReloadMontage;
    FGunnerProbeInputGuard InputGuard;
    TArray<FKey> HeldKeys;
    TArray<FKey> PulseReleases;
    EPhase Phase = EPhase::Initialize;
    int32 Scenario = 0;
    int32 Failures = 0;
    int32 ShotsBefore = 0;
    int32 SpendCount = 0;
    int32 MagazineBefore = 0;
    int32 ReserveBefore = 0;
    int32 LastMagazine = 0;
    int32 LastReserve = 0;
    int32 AmmoChanges = 0;
    float Elapsed = 0.f;
    float ReloadDuration = 0.f;
    float CoverTop = 115.f;
    float MaxHeadZ = -BIG_NUMBER;
    float MaxLeftTravel = 0.f;
    float MaxRightTravel = 0.f;
    float FirstPosition = 0.f;
    float LastPosition = 0.f;
    FVector FirstLeft = FVector::ZeroVector;
    FVector FirstRight = FVector::ZeroVector;
    FVector MoveStart = FVector::ZeroVector;
    bool bOriginalReadiness = false;
    bool bObservedMontage = false;
    bool bObservedSlot = false;
    bool bSampledHands = false;
    bool bCrouchPreserved = true;
    bool bEarlyTransfer = false;
    bool bToggleAttempted = false;
    bool bCapturedEarly = false;
    bool bCapturedMid = false;
    bool IsCoverCase() const { return (Scenario % 2) == 1; }
    bool IsPistolCase() const { return Scenario >= 2; }
    void Check(bool bPass, const TCHAR* Name);
    void LogState(const TCHAR* Reason) const;
    UFUNCTION() void HandleObservedMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void Key(FKey InputKey, bool bPressed);
    void Pulse(FKey InputKey);
    void Transition(EPhase Next);
    void PrepareScenario();
    void BeginReload(bool bCancel);
    void ObserveReload();
    void CheckReloadPresentation();
    void ObserveAmmo();
    void BeginMove();
    void Capture(const TCHAR* Moment);
    void Cleanup();
    void Finish();
};
