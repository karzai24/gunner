#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GunnerCharacter.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UGunnerInputConfig;
class UGunnerMotionSettings;
class UGunnerCombatComponent;
class UGunnerCoverComponent;
class UGunnerDodgeComponent;
struct FInputActionValue;

UCLASS()
class GUNNER_API AGunnerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AGunnerCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void UnPossessed() override;
    const UGunnerInputConfig* GetInputConfig() const { return InputConfig; }
    UFUNCTION(BlueprintPure) bool IsSprinting() const { return bSprinting; }
    UFUNCTION(BlueprintPure) bool IsInCover() const;
    UFUNCTION(BlueprintPure) UGunnerCombatComponent* GetCombat() const { return Combat; }
    UFUNCTION(BlueprintPure) UGunnerCoverComponent* GetCover() const { return Cover; }
    UFUNCTION(BlueprintPure) UGunnerDodgeComponent* GetDodge() const { return Dodge; }
    UFUNCTION(BlueprintPure) float GetShoulderSide() const { return ShoulderSide; }
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<UGunnerCombatComponent> Combat;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cover") TObjectPtr<UGunnerCoverComponent> Cover;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement") TObjectPtr<UGunnerDodgeComponent> Dodge;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement") TObjectPtr<UGunnerMotionSettings> MotionSettings;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UGunnerInputConfig> InputConfig;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input", meta=(ClampMin="0"))
    float StickLookDegreesPerSecond = 120.f;
private:
    void Move(const FInputActionValue& Value);
    void MouseLook(const FInputActionValue& Value);
    void StickLook(const FInputActionValue& Value);
    void Traverse();
    void GroundedJump();
    void StopMove();
    void StartAim();
    void StopAim();
    void StartSprint();
    void StopSprint();
    void ToggleCrouch();
    void SwapShoulder();
    void TryDodge();
    FVector2D MoveAxis = FVector2D::ZeroVector;
    bool bSprintHeld = false;
    bool bSprinting = false;
    bool bAimHeld = false;
    float ShoulderSide = 1.f;
};
