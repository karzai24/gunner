#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GunnerInputConfig.generated.h"
class UInputAction;
class UInputMappingContext;

/** Designer-authored input contract; each LocalPlayer owns its own active context. */
UCLASS(BlueprintType)
class GUNNER_API UGunnerInputConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputMappingContext> MappingContext;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> Move;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> MouseLook;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> StickLook;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> Traverse;
};
