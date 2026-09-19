#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GunnerCharacter.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UGunnerInputConfig;
struct FInputActionValue;

UCLASS()
class GUNNER_API AGunnerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AGunnerCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    const UGunnerInputConfig* GetInputConfig() const { return InputConfig; }
protected:
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
};
