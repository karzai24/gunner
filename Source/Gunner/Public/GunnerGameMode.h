#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GunnerGameMode.generated.h"
UCLASS()
class GUNNER_API AGunnerGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AGunnerGameMode();
    virtual void BeginPlay() override;
};
