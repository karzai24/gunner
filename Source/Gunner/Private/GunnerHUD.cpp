#include "GunnerHUD.h"

#include "GunnerCombatComponent.h"
#include "GunnerCharacter.h"
#include "GunnerCoverComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

void AGunnerHUD::DrawHUD()
{
    Super::DrawHUD();
    APlayerController* OwnerController = GetOwningPlayerController();
    if (!Canvas || !OwnerController) return;
    APawn* Pawn = OwnerController->GetPawn();
    if (Pawn != CachedPawn.Get())
    {
        CachedPawn = Pawn;
        CachedCombat = Pawn ? Pawn->FindComponentByClass<UGunnerCombatComponent>() : nullptr;
    }
    UGunnerCombatComponent* Combat = CachedCombat.Get();
    if (!Combat) return;

    const float W = Canvas->ClipX;
    const float H = Canvas->ClipY;
    const float Scale = FMath::Clamp(W / 1440.f, 0.75f, 1.3f);
    const float Margin = 28.f * Scale;
    const FLinearColor Ink(0.84f, 0.94f, 0.92f, 1.f);
    const FLinearColor Muted(0.57f, 0.66f, 0.66f, 1.f);
    const FLinearColor Panel(0.015f, 0.025f, 0.03f, 0.8f);
    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;

    const float CX = W * 0.5f;
    const float CY = H * 0.5f;
    const float Gap = Combat->IsBlindFiring() ? 18.f : (Combat->IsAiming() ? 4.f : 9.f);
    const bool bObstructed = Combat->WasLastShotObstructed() && GetWorld()->GetTimeSeconds() - Combat->GetLastFireTime() < 0.3f;
    const FLinearColor ReticleColor = bObstructed ? FLinearColor(1.f, 0.6f, 0.2f) : Ink;
    if (!Combat->IsCombatBlocked() && !Combat->IsFireBlocked())
    {
        DrawLine(CX - Gap - 5.f, CY, CX - Gap, CY, ReticleColor, 1.5f);
        DrawLine(CX + Gap, CY, CX + Gap + 5.f, CY, ReticleColor, 1.5f);
        DrawLine(CX, CY - Gap - 5.f, CX, CY - Gap, ReticleColor, 1.5f);
        DrawLine(CX, CY + Gap, CX, CY + Gap + 5.f, ReticleColor, 1.5f);
    }
    if (GetWorld()->GetTimeSeconds() - Combat->GetLastHitTime() < 0.16f)
    {
        const FLinearColor Hit(0.4f, 1.f, 0.8f);
        DrawLine(CX - 12.f, CY - 12.f, CX - 7.f, CY - 7.f, Hit, 2.f);
        DrawLine(CX + 12.f, CY - 12.f, CX + 7.f, CY - 7.f, Hit, 2.f);
        DrawLine(CX - 12.f, CY + 12.f, CX - 7.f, CY + 7.f, Hit, 2.f);
        DrawLine(CX + 12.f, CY + 12.f, CX + 7.f, CY + 7.f, Hit, 2.f);
    }

    const float BoxW = 260.f * Scale;
    const float BoxH = 93.f * Scale;
    const float X = W - Margin - BoxW;
    const float Y = H - Margin - BoxH;
    DrawRect(Panel, X, Y, BoxW, BoxH);
    const UGunnerWeaponData* Data = Combat->GetWeaponData();
    DrawText(Data ? Data->DisplayName.ToString().ToUpper() : TEXT("NO WEAPON"), Ink, X + 14.f * Scale,
        Y + 10.f * Scale, Font, Scale);
    DrawText(FString::Printf(TEXT("%02d  /  %03d"), Combat->GetMagazine(), Combat->GetReserve()), Ink,
        X + 14.f * Scale, Y + 33.f * Scale, Font, 1.65f * Scale);

    FString Status = Data && Data->bAutomatic ? TEXT("AUTO") : TEXT("SEMI");
    switch (Combat->GetActionState())
    {
        case EGunnerCombatAction::Reloading: Status = TEXT("RELOADING"); break;
        case EGunnerCombatAction::Equipping: Status = TEXT("EQUIPPING"); break;
        case EGunnerCombatAction::Melee: Status = TEXT("MELEE"); break;
        default: break;
    }
    // Context hints must not replace the active handling action, particularly
    // a protected reload that stays in low cover throughout the montage.
    if (Combat->GetActionState() == EGunnerCombatAction::Idle || Combat->GetActionState() == EGunnerCombatAction::Firing)
    {
        if (Combat->IsDryFiring()) Status = TEXT("EMPTY / R RELOAD");
        else if (Combat->IsCombatBlocked()) Status = TEXT("WEAPON LOWERED");
        else if (Combat->IsBlindFiring()) Status = TEXT("BLIND FIRE");
        else if (Combat->IsFireBlocked() && Combat->GetActionState() == EGunnerCombatAction::Idle)
        {
            const auto* Warden = Cast<AGunnerCharacter>(Pawn);
            if (Warden && Warden->IsInCover())
                Status = Warden->GetCover()->IsLowCover() ? TEXT("LMB BLIND FIRE / RMB EXPOSE") : TEXT("HIGH COVER / HOLD ADS AT EDGE");
            else Status = TEXT("ADS TO FIRE / R RELOAD");
        }
        else if (bObstructed) Status = TEXT("MUZZLE BLOCKED");
        else if (const auto* Warden = Cast<AGunnerCharacter>(Pawn); Warden && Warden->GetCover()->IsLowCover())
            Status = TEXT("LMB BLIND FIRE / RMB EXPOSE");
    }
    DrawText(Status, Muted, X + 14.f * Scale, Y + 71.f * Scale, Font, 0.85f * Scale);
    DrawText(TEXT("GUNNER  /  MOVEMENT RANGE"), Ink, Margin, Margin, Font, Scale);
    DrawText(TEXT("WASD Move   Mouse Aim   RMB Focus   LMB Fire   R Reload   1/2 Weapon   F Melee"), Muted,
        Margin, H - Margin - 28.f * Scale, Font, 0.82f * Scale);
    DrawText(TEXT("Shift Sprint   C Crouch   Space Cover/Jump   E Roll   Q Shoulder"), Muted,
        Margin, H - Margin - 10.f * Scale, Font, 0.82f * Scale);
}
