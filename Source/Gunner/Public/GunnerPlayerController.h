#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GunnerPlayerController.generated.h"
class UInputMappingContext;

UCLASS()
class GUNNER_API AGunnerPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void AcknowledgePossession(APawn* InPawn) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void RefreshInputContext();
    void RemoveInputContext();
    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveContext;
};
