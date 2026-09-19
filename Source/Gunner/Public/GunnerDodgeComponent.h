#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GunnerDodgeComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UGunnerCombatComponent;
class UGunnerCoverComponent;

/** Authored in-place roll moved by CharacterMovement's native root-motion source. */
UCLASS(ClassGroup=(Gunner), meta=(BlueprintSpawnableComponent))
class GUNNER_API UGunnerDodgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGunnerDodgeComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dodge")
    TObjectPtr<UAnimMontage> RollMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dodge", meta=(ClampMin="100", ClampMax="350"))
    float DodgeDistance = 350.f;

    UFUNCTION(BlueprintCallable, Category="Dodge") bool TryDodge();
    UFUNCTION(BlueprintCallable, Category="Dodge") void CancelDodge();
    UFUNCTION(BlueprintPure, Category="Dodge") bool IsDodging() const { return bDodging; }

private:
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UGunnerCombatComponent> Combat;
    UPROPERTY(Transient) TObjectPtr<UGunnerCoverComponent> Cover;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveMontage;
    FTimerHandle TimeoutTimer;
    uint16 MotionSourceId = 0;
    int32 ActionSerial = 0;
    bool bDodging = false;
    bool bPreviousOrientToMovement = false;

    bool FindSafeDestination(const FVector& Direction, FVector& Destination) const;
    void FinishDodge(bool bStopMontage);
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ExpectedSerial);

    UFUNCTION()
    void OnMovementModeChanged(ACharacter* ChangedCharacter, EMovementMode PreviousMode, uint8 PreviousCustomMode);
};
