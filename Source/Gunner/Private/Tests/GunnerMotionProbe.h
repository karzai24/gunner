#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerMotionProbe.generated.h"

class AGunnerCharacter;
class AGunnerTarget;
class APlayerController;
class AStaticMeshActor;
class UStaticMesh;

/** Opt-in single-player live input regression; transient fixtures never enter the saved map. */
UCLASS(NotBlueprintable, Transient)
class AGunnerMotionProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerMotionProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) int32 GetFailureCount() const { return Failures; }
private:
    FGunnerProbeInputGuard InputGuard;
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<AGunnerTarget> Target;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Blocker;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Ceiling;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    TArray<FKey> HeldKeys;
    FVector Start = FVector::ZeroVector;
    FVector HandBefore = FVector::ZeroVector;
    float Elapsed = 0.f;
    float NextDelay = 5.f;
    float ShoulderBefore = 1.f;
    int32 Stage = 0;
    int32 Failures = 0;
    int32 ShotsBefore = 0;
    int32 HitsBefore = 0;
    int32 MagazineBefore = 0;
    int32 ReserveBefore = 0;
    bool bTrackTarget = false;
    bool bReturnWaitLogged = false;
    void Check(bool bPass, const TCHAR* Name);
    void Key(const FKey& InputKey, bool bPressed);
    void Capture(const TCHAR* Name);
    void Advance(float Delay = 0.2f);
    void Teleport(const FVector& Location, float Yaw = 0.f);
    AStaticMeshActor* SpawnBox(const FVector& Location, const FVector& Size);
    void Cleanup();
    void Finish();
};
