#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GunnerWeaponData.generated.h"

class UAnimMontage;
class UStaticMesh;

UENUM(BlueprintType)
enum class EGunnerWeaponKind : uint8
{
    Rifle,
    Pistol
};

/** Authored presentation and tuning. Runtime ammunition belongs to the owning combat component. */
UCLASS(BlueprintType)
class GUNNER_API UGunnerWeaponData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FName ContentId = TEXT("weapon_arc_carbine");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FText DisplayName = FText::FromString(TEXT("Arc Carbine"));
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    EGunnerWeaponKind WeaponKind = EGunnerWeaponKind::Rifle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    TObjectPtr<UStaticMesh> GunMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FName HandSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FTransform GripTransform;
    /** Derived from the authored armed idle: left hand relative to the held weapon. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FTransform LeftHandGripTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    bool bBlindFireGripReady = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FVector MuzzleOffset = FVector(65.f, 0.f, 0.f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> FireMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> DryFireMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> JumpStartMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> JumpLandMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> EquipMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> MeleeMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
    TObjectPtr<UAnimMontage> AlternateMeleeMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="0.1", ClampMax="0.85"))
    float AlternateMeleeImpactFraction = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire", meta=(ClampMin="1", ClampMax="200"))
    int32 MagazineCapacity = 30;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire", meta=(ClampMin="0"))
    int32 InitialReserve = 180;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire")
    bool bAutomatic = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire", meta=(ClampMin="0.03"))
    float FireInterval = 0.12f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire", meta=(ClampMin="100"))
    float Range = 10000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire", meta=(ClampMin="0"))
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="0"))
    float MeleeDamage = 40.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="30", ClampMax="200"))
    float MeleeReach = 135.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="5", ClampMax="60"))
    float MeleeRadius = 35.f;
    // A bounded impact request, validated against the currently playing action montage.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="0.1", ClampMax="0.85"))
    float MeleeImpactFraction = 0.38f;
};
