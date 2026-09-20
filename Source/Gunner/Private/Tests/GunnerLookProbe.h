#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GunnerLookProbe.generated.h"

class AGunnerCharacter;
class APlayerController;
class UCameraComponent;

/** Synthetic Enhanced Input regression. It does not test physical macOS mouse delivery. */
UCLASS(NotBlueprintable, Transient)
class AGunnerLookProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerLookProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    TArray<FKey> HeldKeys;
    FRotator BeforeLook = FRotator::ZeroRotator;
    float Elapsed = 0.f;
    float Wait = 3.f;
    int32 Step = -1;
    int32 Phase = 0;
    int32 Failures = 0;
    void Check(bool bPassed, const TCHAR* Detail);
    void Key(FKey InputKey, bool bPressed);
    void ConfigureStep();
    void InjectLook();
    void CheckLook();
    void ReleaseKeys();
    void Finish();
};
