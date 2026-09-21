#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerTraversalProbe.generated.h"
class AGunnerCharacter;
class APlayerController;
class AStaticMeshActor;
class UStaticMesh;
class UGunnerMotionSettings;

/** Rendered, transient geometry checks for bounded cover approach and cancellation. */
UCLASS(NotBlueprintable, Transient)
class AGunnerTraversalProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerTraversalProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) int32 GetFailureCount() const { return Failures; }
private:
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UGunnerMotionSettings> Settings;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Fixtures;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Wall;
    FGunnerProbeInputGuard InputGuard;
    TArray<FKey> Held;
    int32 Phase = 0, Scenario = 0, Failures = 0, Skipped = 0, DirectionStep = 0, SprintShotsBefore = 0;
    int32 DryFiresBefore = 0, ReserveBefore = 0;
    uint16 ExternalSourceId = 0;
    float Elapsed = 0.f, Total = 0.f, MaxFeetDrift = 0.f, NormalDuration = 0.f;
    float StandingHead = 0.f, MaxHead = 0.f, BeforeYaw = 0.f, BeforePitch = 0.f, PreviousTravelYaw = 0.f;
    float MaxHandTravel = 0.f;
    float StanceMinHead = BIG_NUMBER, StanceMaxHead = -BIG_NUMBER;
    FVector Start = FVector::ZeroVector, Previous = FVector::ZeroVector, Stopped = FVector::ZeroVector;
    FVector FirstHand = FVector::ZeroVector;
    bool bObservedIntermediate = false, bObservedGait = false, bCaptured = false, bActionDone = false, bRouteSafe = true;
    bool bPoseSafe = true, bHeadingSafe = true;
    bool bCoverReadinessOverridden = false, bOriginalCoverReady = false;
    void Key(FKey Key, bool bPressed);
    void Check(bool bPass, const TCHAR* Label);
    void Go(int32 Next);
    void Prepare();
    void NextScenario();
    void Cleanup();
    void RestoreSettings();
    void Finish();
    void Capture(const TCHAR* Moment);
    bool RequireCapability(bool bReady, const TCHAR* Name);
    FKey DirectionKey() const;
    AStaticMeshActor* SpawnBox(const FVector& Location, const FVector& Size, bool bStatic = true);
};
