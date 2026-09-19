#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GunnerHUD.generated.h"

class APawn;
class UGunnerCombatComponent;

/** Passive owning-player range HUD. It never mutates ammunition, targets or character state. */
UCLASS()
class GUNNER_API AGunnerHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    TWeakObjectPtr<APawn> CachedPawn;
    TWeakObjectPtr<UGunnerCombatComponent> CachedCombat;
};
