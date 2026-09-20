#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tests/GunnerProbeInputGuard.h"
#include "GunnerPolishProbe.generated.h"
class AGunnerCharacter;
class AGunnerTarget;
class APlayerController;
class UAnimMontage;
/** Opt-in single-player rendered input/pose regression; never part of normal play. */
UCLASS(NotBlueprintable, Transient)
class AGunnerPolishProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerPolishProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) int32 GetFailureCount() const { return Failures; }
private:
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<AGunnerTarget> Target;
    UPROPERTY() TObjectPtr<UAnimMontage> Selected;
    FGunnerProbeInputGuard InputGuard;
    TArray<FKey> Held, Releases;
    int32 Phase=0, Scenario=0, Failures=0, Attack=0, LastVariant=-1;
    int32 Shots=0, Hits=0, Dry=0, Magazine=0, Reserve=0;
    float Elapsed=0.f, PulseAt=0.f, MaxHeight=0.f, MinPelvis=BIG_NUMBER, MaxHead=-BIG_NUMBER;
    float MaxHandTravel=0.f, MaxRootOffset=0.f;
    FVector Start, FirstHand;
    bool bObserved=false, bCaptured=false, bPoseSafe=true, bEquipReady=false, bDryReady=false;
    bool IsCover() const { return Scenario%2==1; }
    bool IsPistol() const { return Scenario>=2; }
    void Key(FKey K, bool Pressed);
    void Pulse(FKey K);
    void Go(int32 Next);
    void Check(bool Pass, const TCHAR* Name);
    void Prepare();
    void Capture(const TCHAR* Moment);
    void BeginAttack();
    void BeginDrain();
    void BeginEquip(bool Original);
    void ObserveEquip();
    void Cleanup();
    void Finish();
};
