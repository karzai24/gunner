#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerControllerProbe.generated.h"

class AGunnerCharacter;
class AGunnerPlayerController;
class AStaticMeshActor;
class UStaticMesh;
class UAnimMontage;

/** Exercises real Enhanced Input mappings using synthetic physical gamepad keys/axes. */
UCLASS(NotBlueprintable, Transient)
class AGunnerControllerProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerControllerProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) int32 GetFailureCount() const { return Failures; }
private:
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<AGunnerPlayerController> Player;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Fixtures;
    FGunnerProbeInputGuard InputGuard;
    TArray<FKey> Held;
    FVector2D MoveStick = FVector2D::ZeroVector;
    FVector2D LookStick = FVector2D::ZeroVector;
    FVector Start = FVector::ZeroVector;
    FRotator BeforeLook = FRotator::ZeroRotator;
    float Elapsed = 0.f, Delay = 3.f, Total = 0.f, InjectedTime = 0.f;
    float PartialSpeed = 0.f, FreeLookRate = 0.f, ShoulderBefore = 0.f;
    int32 Stage = 0, Failures = 0, ShotsBefore = 0, MagazineBefore = 0, ReserveBefore = 0;
    void Key(FKey InputKey, bool bPressed);
    void Axes(float DeltaSeconds);
    void ReleaseInput();
    void Advance(float NextDelay = .3f);
    void Check(bool bPass, const TCHAR* Label);
    void Capture(const TCHAR* Name);
    void ResetPawn();
    AStaticMeshActor* SpawnBox(FVector Location, FVector Size);
    float MontageDuration(const UAnimMontage* Montage) const;
    void Finish();
    void Cleanup();
};
