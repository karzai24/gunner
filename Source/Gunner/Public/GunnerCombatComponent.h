#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GunnerWeaponData.h"
#include "GunnerCombatComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGunnerCombatAction : uint8
{
    Idle,
    Firing,
    Reloading,
    Equipping,
    Melee
};

/** Standalone sandbox authority. This does not implement network requests or replicated weapons. */
UCLASS(ClassGroup=(Gunner), meta=(BlueprintSpawnableComponent))
class GUNNER_API UGunnerCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UGunnerCombatComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapons")
    TObjectPtr<UGunnerWeaponData> RifleData;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapons")
    TObjectPtr<UGunnerWeaponData> PistolData;

    UFUNCTION(BlueprintCallable, Category="Combat") void StartFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void StopFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void StartAim();
    UFUNCTION(BlueprintCallable, Category="Combat") void StopAim();
    UFUNCTION(BlueprintCallable, Category="Combat") void Reload();
    UFUNCTION(BlueprintCallable, Category="Combat") void EquipRifle();
    UFUNCTION(BlueprintCallable, Category="Combat") void EquipPistol();
    UFUNCTION(BlueprintCallable, Category="Combat") void Melee();
    UFUNCTION(BlueprintCallable, Category="Combat") void SetCombatBlocked(bool bBlocked);
    UFUNCTION(BlueprintCallable, Category="Combat") void SetFireBlocked(bool bBlocked);
    UFUNCTION(BlueprintCallable, Category="Combat") void StopAllActions();

    UFUNCTION(BlueprintPure, Category="Combat") bool IsAiming() const { return bAiming; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsReloading() const { return ActionState == EGunnerCombatAction::Reloading; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsMeleeing() const { return ActionState == EGunnerCombatAction::Melee; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsCombatBlocked() const { return bCombatBlocked; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsFireBlocked() const { return bFireBlocked; }
    UFUNCTION(BlueprintPure, Category="Combat") EGunnerCombatAction GetActionState() const { return ActionState; }
    UFUNCTION(BlueprintPure, Category="Combat") EGunnerWeaponKind GetWeaponKind() const;
    UFUNCTION(BlueprintPure, Category="Combat") UGunnerWeaponData* GetWeaponData() const { return ActiveData; }
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetMagazine() const;
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetReserve() const;
    UFUNCTION(BlueprintPure, Category="Combat") int32 GetShotsFired() const { return ShotsFired; }
    UFUNCTION(BlueprintPure, Category="Combat") float GetLastHitTime() const { return LastHitTime; }
    UFUNCTION(BlueprintPure, Category="Combat") float GetLastFireTime() const { return LastFireTime; }
    UFUNCTION(BlueprintPure, Category="Combat") bool WasLastShotObstructed() const { return bLastShotObstructed; }
    UFUNCTION(BlueprintPure, Category="Combat") FVector GetMuzzleLocation() const;
    UFUNCTION(BlueprintPure, Category="Combat") UStaticMeshComponent* GetWeaponVisual() const { return WeaponVisual; }

private:
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UGunnerWeaponData> ActiveData;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WeaponVisual;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActionMontage;
    EGunnerCombatAction ActionState = EGunnerCombatAction::Idle;
    int32 Magazines[2] = { 0, 0 };
    int32 Reserves[2] = { 0, 0 };
    int32 ActiveSlot = 0;
    int32 ActionSerial = 0;
    int32 ShotsFired = 0;
    float LastFireTime = -100.f;
    float LastHitTime = -100.f;
    bool bAiming = false;
    bool bFireHeld = false;
    bool bCombatBlocked = false;
    bool bFireBlocked = false;
    bool bLastShotObstructed = false;
    bool bMeleeCommitted = false;
    FTimerHandle FireTimer;
    FTimerHandle ActionTimeoutTimer;
    FTimerHandle MeleeImpactTimer;

    bool CanStartAction() const;
    bool HasCombatAuthority() const;
    UAnimInstance* GetAnimInstance() const;
    void FireOnce();
    void EquipSlot(int32 Slot);
    void UpdateWeaponVisual();
    bool BeginAction(EGunnerCombatAction NewAction, UAnimMontage* Montage);
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ExpectedSerial);
    void HandleActionTimeout();
    void CommitMelee();
    void FinishAction(bool bCompleted);
    void DrawShot(const FVector& Start, const FVector& End, bool bHit) const;
};
