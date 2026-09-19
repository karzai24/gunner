#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GunnerTarget.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** Passive, resettable range fixture. No enemy AI, spawn rules or player health. */
UCLASS()
class GUNNER_API AGunnerTarget : public AActor
{
    GENERATED_BODY()
public:
    AGunnerTarget();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
        AActor* DamageCauser) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target")
    TObjectPtr<UStaticMeshComponent> TargetMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target")
    TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
    FText TargetName = FText::FromString(TEXT("RANGE TARGET"));
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="1"))
    float MaxDurability = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0.1"))
    float ResetDelay = 2.f;

    UFUNCTION(BlueprintCallable, Category="Target") void ResetTarget();
    UFUNCTION(BlueprintPure, Category="Target") float GetDurability() const { return Durability; }
    UFUNCTION(BlueprintPure, Category="Target") int32 GetHitCount() const { return HitCount; }
    UFUNCTION(BlueprintPure, Category="Target") int32 GetTotalHitCount() const { return TotalHitCount; }
private:
    float Durability = 100.f;
    int32 HitCount = 0;
    int32 TotalHitCount = 0;
    FTimerHandle ResetTimer;
    void UpdateLabel();
};
