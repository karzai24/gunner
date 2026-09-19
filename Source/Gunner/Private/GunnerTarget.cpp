#include "GunnerTarget.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGunnerTarget::AGunnerTarget()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TargetRoot")));
    TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
    TargetMesh->SetupAttachment(RootComponent);
    TargetMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(14.f);
    Label->SetRelativeLocation(FVector(-55.f, 0.f, 110.f));
    Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    Label->SetTextRenderColor(FColor(200, 245, 235));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGunnerTarget::BeginPlay()
{
    Super::BeginPlay();
    ResetTarget();
}

void AGunnerTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorld()->GetTimerManager().ClearTimer(ResetTimer);
    Super::EndPlay(EndPlayReason);
}

float AGunnerTarget::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
    AActor* DamageCauser)
{
    if (!HasAuthority() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f || Durability <= 0.f) return 0.f;
    const float Accepted = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    const float Applied = FMath::Min(FMath::Max(0.f, Accepted), Durability);
    if (Applied <= 0.f) return 0.f;
    Durability -= Applied;
    ++HitCount;
    ++TotalHitCount;
    UpdateLabel();
    if (Durability <= 0.f)
    {
        GetWorld()->GetTimerManager().SetTimer(ResetTimer, this, &AGunnerTarget::ResetTarget,
            FMath::Max(0.1f, ResetDelay), false);
    }
    return Applied;
}

void AGunnerTarget::ResetTarget()
{
    if (!HasAuthority()) return;
    GetWorld()->GetTimerManager().ClearTimer(ResetTimer);
    Durability = FMath::Max(1.f, MaxDurability);
    HitCount = 0;
    UpdateLabel();
}

void AGunnerTarget::UpdateLabel()
{
    const FString Detail = Durability > 0.f
        ? FString::Printf(TEXT("%s\n%03.0f / %03.0f   HITS %d"), *TargetName.ToString(), Durability, MaxDurability, HitCount)
        : FString::Printf(TEXT("%s\nRESETTING"), *TargetName.ToString());
    Label->SetText(FText::FromString(Detail));
}
