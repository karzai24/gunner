#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GunnerCoverComponent.generated.h"
class ACharacter;

/** Bounded static-wall attachment. CharacterMovement still owns collision and movement. */
UCLASS(ClassGroup=(Gunner), meta=(BlueprintSpawnableComponent))
class GUNNER_API UGunnerCoverComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UGunnerCoverComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UFUNCTION(BlueprintCallable) bool TryAttach(const FVector& SearchDirection);
    UFUNCTION(BlueprintCallable) void Detach();
    UFUNCTION(BlueprintPure) bool IsAttached() const { return bAttached; }
    UFUNCTION(BlueprintPure) bool IsLowCover() const { return bAttached && bLow; }
    /** High cover uses a swept, animated step around the edge rather than a fabricated lean pose. */
    UFUNCTION(BlueprintCallable) void SetPeekDesired(bool bDesired, float Side);
    UFUNCTION(BlueprintPure) bool IsPeeking() const { return PeekState != EPeekState::None; }
    UFUNCTION(BlueprintPure) bool CanPeek(float Side) const;
    FVector ConstrainMovement(const FVector& DesiredDirection) const;
    FVector GetNormal() const { return WallNormal; }
    /** Trace the real static barricade top; never assume the fixture's authored height. */
    bool GetLowCoverTop(float& OutWorldZ) const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cover", meta=(ClampMin="1")) float QueryReach = 135.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cover", meta=(ClampMin="36")) float AttachedOffset = 52.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cover") float MoveSpeed = 150.f;
private:
    enum class EPeekState : uint8 { None, SteppingOut, Exposed, Returning };
    bool WallAt(const FVector& Center, FHitResult& Hit, float Height = 55.f) const;
    bool FindOpenEdge(const FVector& Anchor, float Side, FVector& OutDirection) const;
    bool FindPeekDestination(const FVector& Anchor, float Side, FVector& OutDestination,
        FVector& OutDirection) const;
    void BeginReturn();
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    TWeakObjectPtr<UPrimitiveComponent> Wall;
    FVector WallNormal = FVector::ZeroVector;
    bool bAttached = false;
    bool bLow = false;
    EPeekState PeekState = EPeekState::None;
    FVector PeekAnchor = FVector::ZeroVector;
    FVector PeekDestination = FVector::ZeroVector;
    FVector PeekDirection = FVector::ZeroVector;
    float PeekTransitionTime = 0.f;
    bool bPeekRequestBlocked = false;
};
