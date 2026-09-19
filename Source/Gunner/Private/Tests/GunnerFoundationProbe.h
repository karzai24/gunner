#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GunnerFoundationProbe.generated.h"
class AGunnerCharacter;
class APlayerController;

/** Opt-in development-only live smoke; never spawned during normal gameplay. */
UCLASS(NotBlueprintable, Transient)
class AGunnerFoundationProbe : public AActor
{
    GENERATED_BODY()
public:
    AGunnerFoundationProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    void Check(bool bPass, const TCHAR* Name);
    void Key(const FKey& InputKey, bool bPressed);
    void Capture(const TCHAR* Name);
    UPROPERTY() TObjectPtr<AGunnerCharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Player;
    FVector Start;
    float InitialYaw = 0.f;
    float InitialPitch = 0.f;
    float Elapsed = 0.f;
    int32 Stage = -1;
    int32 Failures = 0;
};
