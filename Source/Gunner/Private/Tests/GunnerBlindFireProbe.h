#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerBlindFireProbe.generated.h"
class AGunnerCharacter;
class AGunnerTarget;
class APlayerController;
class AStaticMeshActor;
class UStaticMesh;

/** Rendered single-player regression with transient fixtures and actual evaluated grip checks. */
UCLASS(NotBlueprintable, Transient)
class AGunnerBlindFireProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerBlindFireProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    FGunnerProbeInputGuard InputGuard;
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<AGunnerTarget> Target;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Blocker;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    TArray<FKey> HeldKeys;
    FVector Start = FVector::ZeroVector;
    float Elapsed = 0.f;
    float Delay = 3.f;
    int32 Stage = 0;
    int32 Failures = 0;
    int32 ShotsBefore = 0;
    int32 HitsBefore = 0;
    bool bTrackTarget = true;
    void Check(bool bPass, const TCHAR* Name);
    void Key(FKey InputKey, bool bPressed);
    void Advance(float Wait = 0.2f);
    void Capture(const TCHAR* Name);
    void TeleportToCover();
    void CheckPose();
    void Cleanup();
    void Finish();
};
