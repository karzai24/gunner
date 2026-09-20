#include "Tests/GunnerPolishProbe.h"
#include "GunnerAnimInstance.h"
#include "GunnerCharacter.h"
#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "GunnerTarget.h"
#include "GunnerWeaponData.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
AGunnerPolishProbe::AGunnerPolishProbe()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
#endif
}
void AGunnerPolishProbe::Key(FKey K, bool Pressed)
{
    if (!Player) return;
    if (Pressed) Held.AddUnique(K); else Held.Remove(K);
    Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, K, Pressed?IE_Pressed:IE_Released,
        Pressed?1.f:0.f, false, FPlatformTime::Cycles64()));
}
void AGunnerPolishProbe::Pulse(FKey K) { Key(K,true); Releases.AddUnique(K); }
void AGunnerPolishProbe::Go(int32 Next) { Phase=Next; Elapsed=0.f; }
void AGunnerPolishProbe::Check(bool Pass, const TCHAR* Name)
{
    if (Pass) { UE_LOG(LogTemp,Display,TEXT("GUNNER_POLISH_CHECK PASS scenario=%d phase=%d %s"),Scenario,Phase,Name); }
    else { ++Failures; UE_LOG(LogTemp,Error,TEXT("GUNNER_POLISH_CHECK FAIL scenario=%d phase=%d %s"),Scenario,Phase,Name); }
}
void AGunnerPolishProbe::Capture(const TCHAR* Moment)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/
        FString::Printf(TEXT("polish_%d_%s.png"),Scenario,Moment),false,false);
}
void AGunnerPolishProbe::Prepare()
{
    while (!Held.IsEmpty()) Key(Held.Last(),false);
    Releases.Reset();
    Character->GetCombat()->StopAllActions(); Character->StopJumpPresentation();
    Character->GetCover()->Detach(); Character->UnCrouch();
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->SetActorLocation(FVector(0, IsCover()?-850.f:-1300.f,
        Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f),false,nullptr,ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0,90,0)); Player->SetControlRotation(FRotator(0,90,0));
    Go(1);
}
void AGunnerPolishProbe::BeginAttack()
{
    Hits=Target->GetTotalHitCount(); bCaptured=false;
    Pulse(EKeys::F); Go(6);
}
void AGunnerPolishProbe::BeginDrain()
{
    if (Target) Target->SetActorLocation(FVector(-1800,0,100));
    Character->GetCharacterMovement()->StopMovementImmediately();
    Shots=Character->GetCombat()->GetShotsFired(); Magazine=Character->GetCombat()->GetMagazine();
    PulseAt=0.f;
    if (IsPistol()) Pulse(EKeys::LeftMouseButton); else Key(EKeys::LeftMouseButton,true);
    Go(10);
}
void AGunnerPolishProbe::BeginEquip(bool Original)
{
    auto* Combat=Character->GetCombat();
    const bool Pistol=Original?IsPistol():!IsPistol();
    Selected=Pistol?Combat->PistolData->EquipMontage:Combat->RifleData->EquipMontage;
    MaxHead=-BIG_NUMBER; MaxHandTravel=0.f; bObserved=bCaptured=false; bPoseSafe=true;
    FirstHand=Character->GetMesh()->GetComponentTransform().InverseTransformPosition(Character->GetMesh()->GetSocketLocation(TEXT("hand_r")));
    Pulse(Pistol?EKeys::Two:EKeys::One);
}
void AGunnerPolishProbe::ObserveEquip()
{
    const auto* Combat=Character->GetCombat();
    auto* Anim=Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
    if (Elapsed>0.35f && Combat->GetActionState()==EGunnerCombatAction::Equipping)
    {
        bObserved |= Anim->Montage_IsPlaying(Selected) && Anim->IsSlotActive(TEXT("UpperBody")) && Anim->CrouchHandlingAlpha>0.9f;
        bPoseSafe &= Character->bIsCrouched && !Combat->IsAiming() && FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),62.f);
        MaxHead=FMath::Max(MaxHead,Character->GetMesh()->GetSocketLocation(TEXT("head")).Z);
        const FVector Hand=Character->GetMesh()->GetComponentTransform().InverseTransformPosition(Character->GetMesh()->GetSocketLocation(TEXT("hand_r")));
        MaxHandTravel=FMath::Max(MaxHandTravel,FVector::Distance(FirstHand,Hand));
        if (!bCaptured) { Capture(Phase==23?TEXT("equip_from_ads"):TEXT("equip")); bCaptured=true; }
    }
}
void AGunnerPolishProbe::Cleanup()
{
    while (!Held.IsEmpty()) Key(Held.Last(),false);
    Releases.Reset();
    if (Character)
    {
        if (auto* Anim=Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance()))
        { Anim->bCrouchEquipPoseReady=bEquipReady; Anim->bCrouchDryFirePoseReady=bDryReady; }
        Character->GetCombat()->StopAllActions(); Character->StopJumpPresentation(); Character->GetCover()->Detach();
    }
    if (Target) Target->Destroy(); Target=nullptr;
    InputGuard.Restore();
}
void AGunnerPolishProbe::Finish()
{
    Cleanup(); UE_LOG(LogTemp,Display,TEXT("GUNNER_POLISH_COMPLETE failures=%d scenarios=%d"),Failures,Scenario);
    SetActorTickEnabled(false);
}
void AGunnerPolishProbe::EndPlay(const EEndPlayReason::Type Reason) { Cleanup(); Super::EndPlay(Reason); }
void AGunnerPolishProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    InputGuard.Begin(GetWorld());
    while (!Releases.IsEmpty()) Key(Releases.Pop(),false);
    Elapsed+=DeltaSeconds;
    if (Phase==0)
    {
        if (Elapsed<3.f) return;
        Player=GetWorld()->GetFirstPlayerController(); // Deliberately single-player test, not reusable gameplay.
        Character=Player?Cast<AGunnerCharacter>(Player->GetPawn()):nullptr;
        Check(Character!=nullptr,TEXT("Possessed animated pawn exists"));
        if (!Character) { Finish(); return; }
        Player->FlushPressedKeys();
        auto* Anim=Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
        bEquipReady=Anim&&Anim->bCrouchEquipPoseReady; bDryReady=Anim&&Anim->bCrouchDryFirePoseReady;
        Check(bEquipReady&&bDryReady,TEXT("Both protective handling capabilities are authored"));
        if (!bEquipReady||!bDryReady) { Finish(); return; }
        FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Target=GetWorld()->SpawnActor<AGunnerTarget>(FVector(-1800,0,100),FRotator::ZeroRotator,Params);
        Check(Target!=nullptr,TEXT("Transient damage fixture exists"));
        if (!Target) { Finish(); return; }
        Target->TargetMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Target->TargetMesh->SetRelativeScale3D(FVector(.6f,.6f,1.6f)); Target->MaxDurability=10000; Target->ResetTarget();
        Prepare(); return;
    }
    auto* Combat=Character->GetCombat(); auto* Cover=Character->GetCover();
    auto* Movement=Character->GetCharacterMovement(); auto* Mesh=Character->GetMesh();
    auto* Anim=Cast<UGunnerAnimInstance>(Mesh->GetAnimInstance());
    if (!Anim || Elapsed>20.f) { Check(false,TEXT("Phase completed with live animation before timeout")); Finish(); return; }
    const auto Kind=IsPistol()?EGunnerWeaponKind::Pistol:EGunnerWeaponKind::Rifle;
    switch (Phase)
    {
    case 1:
        if (Elapsed<.4f) return;
        Pulse(IsPistol()?EKeys::Two:EKeys::One); Go(2); break;
    case 2:
        if (Elapsed<.4f || Combat->GetActionState()==EGunnerCombatAction::Equipping) return;
        Check(Combat->GetWeaponKind()==Kind,TEXT("Scenario weapon equipped"));
        Check(Combat->GetWeaponData()->JumpStartMontage&&Combat->GetWeaponData()->JumpLandMontage&&Combat->GetWeaponData()->DryFireMontage&&Combat->GetWeaponData()->AlternateMeleeMontage,
            TEXT("Weapon has all four new authored actions"));
        if (IsCover()) { BeginDrain(); break; }
        Start=Character->GetActorLocation(); MaxHeight=Start.Z; MaxRootOffset=0; bObserved=bCaptured=false;
        Pulse(EKeys::J); Go(3); break;
    case 3:
        MaxHeight=FMath::Max(MaxHeight,Character->GetActorLocation().Z);
        MaxRootOffset=FMath::Max(MaxRootOffset,FVector::Dist2D(Mesh->GetSocketLocation(TEXT("root")),Character->GetActorLocation()));
        if (Anim->Montage_IsPlaying(Combat->GetWeaponData()->JumpStartMontage))
        { bObserved=true; if (!bCaptured && Elapsed>.07f) { Capture(TEXT("takeoff")); bCaptured=true; } }
        if (Elapsed<.25f || Movement->IsFalling()) return;
        Check(bObserved,TEXT("Jump input evaluates the selected takeoff montage"));
        Check(MaxHeight>Start.Z+45.f && FVector::Dist2D(Start,Character->GetActorLocation())<2.f && MaxRootOffset<5.f,
            TEXT("CharacterMovement owns jump displacement without horizontal root drift"));
        Check(Anim->Montage_IsActive(Combat->GetWeaponData()->JumpLandMontage),TEXT("Ground contact starts the selected landing montage"));
        MinPelvis=BIG_NUMBER; bCaptured=false; Go(4); break;
    case 4:
        MinPelvis=FMath::Min(MinPelvis,Mesh->GetSocketLocation(TEXT("pelvis")).Z);
        MaxRootOffset=FMath::Max(MaxRootOffset,FVector::Dist2D(Mesh->GetSocketLocation(TEXT("root")),Character->GetActorLocation()));
        if (!bCaptured && Elapsed>.14f) { Capture(TEXT("landing")); bCaptured=true; }
        if (Elapsed<1.2f) return;
        UE_LOG(LogTemp,Display,TEXT("GUNNER_POLISH_JUMP scenario=%d apex=%.2f pelvis_min=%.2f pelvis_idle=%.2f root_drift=%.2f"),Scenario,MaxHeight,MinPelvis,Mesh->GetSocketLocation(TEXT("pelvis")).Z,MaxRootOffset);
        Check(MinPelvis>35.f && MinPelvis+8.f<Mesh->GetSocketLocation(TEXT("pelvis")).Z && MaxRootOffset<5.f,
            TEXT("Landing visibly compresses pelvis and recovers without moving root or sinking below floor"));
        Check(Movement->IsMovingOnGround()&&!Anim->Montage_IsActive(Combat->GetWeaponData()->JumpLandMontage),TEXT("Landing finishes without movement lock"));
        Pulse(EKeys::J); Go(8); break;
    case 6:
        if (Elapsed<.08f) return;
        Check(Combat->IsMeleeing() && Target->GetTotalHitCount()==Hits,TEXT("Accepted melee has no damage before impact"));
        Check(LastVariant<0 || Combat->GetMeleeVariant()!=LastVariant,TEXT("Successive accepted melee presses alternate jab and cross"));
        LastVariant=Combat->GetMeleeVariant();
        Selected=LastVariant?Combat->GetWeaponData()->AlternateMeleeMontage:Combat->GetWeaponData()->MeleeMontage;
        Check(Anim->Montage_IsPlaying(Selected)&&Anim->IsSlotActive(TEXT("FullBody")),TEXT("Selected melee variant plays on FullBody"));
        if (Attack==2) Anim->Montage_Stop(.05f,Selected);
        Go(7); break;
    case 7:
        if (Attack<2 && Elapsed>.22f && !bCaptured) { Capture(LastVariant?TEXT("cross"):TEXT("jab")); bCaptured=true; }
        if (Elapsed<1.4f) return;
        Check(Target->GetTotalHitCount()==Hits+(Attack==2?0:1),TEXT("Completed attack deals one hit; interrupted attack deals none"));
        Check(!Combat->IsMeleeing(),TEXT("Melee completion or interruption releases action lock"));
        bCaptured=false;
        if (++Attack<3) BeginAttack(); else BeginDrain();
        break;
    case 8:
        if (Elapsed<.3f || Movement->IsFalling()) return;
        Check(Anim->Montage_IsActive(Combat->GetWeaponData()->JumpLandMontage),TEXT("Repeated jump reaches animated landing"));
        Shots=Combat->GetShotsFired(); Pulse(EKeys::LeftMouseButton); Go(9); break;
    case 9:
        if (Elapsed<.2f) return;
        Check(Combat->GetShotsFired()==Shots+1 && !Anim->Montage_IsActive(Combat->GetWeaponData()->JumpLandMontage),
            TEXT("Fire interrupts landing recovery immediately and produces one normal shot"));
        Target->SetActorLocation(Character->GetActorLocation()+Character->GetActorForwardVector()*120.f);
        Attack=0; BeginAttack(); break;
    case 10:
        if (Combat->GetMagazine()>0)
        {
            if (IsPistol() && Elapsed-PulseAt>.3f) { PulseAt=Elapsed; Pulse(EKeys::LeftMouseButton); }
            return;
        }
        Check(Combat->GetShotsFired()==Shots+Magazine,TEXT("Input drains exactly the current magazine"));
        Key(EKeys::LeftMouseButton,false); Go(11); break;
    case 11:
        if (Elapsed<1.f) return;
        Pulse(IsCover()?EKeys::SpaceBar:EKeys::C); Go(12); break;
    case 12:
        if (Elapsed<.6f) return;
        Check(Character->bIsCrouched && Cover->IsLowCover()==IsCover(),TEXT("Dry-fire fixture is in requested genuine crouch context"));
        Dry=Combat->GetDryFireCount(); Shots=Combat->GetShotsFired(); Hits=Target->GetTotalHitCount(); Reserve=Combat->GetReserve();
        Anim->bCrouchDryFirePoseReady=false; Pulse(EKeys::LeftMouseButton); Go(13); break;
    case 13:
        if (Elapsed<.3f) return;
        Check(Combat->GetDryFireCount()==Dry,TEXT("Missing crouched dry-fire graph capability rejects presentation"));
        Anim->bCrouchDryFirePoseReady=bDryReady;
        Key(EKeys::LeftMouseButton,true); Go(14); break;
    case 14:
        if (Elapsed<.25f) return;
        Check(Combat->GetDryFireCount()==Dry+1 && Combat->IsDryFiring() && Anim->IsSlotActive(TEXT("UpperBody")) && Anim->CrouchHandlingAlpha>.9f,
            TEXT("Empty trigger plays selected dry-fire through protective arm layer"));
        Check(Character->bIsCrouched && !Combat->IsBlindFiring() && (!IsCover() || Mesh->GetSocketLocation(TEXT("head")).Z<110.f),
            TEXT("Dry fire stays crouched below low cover without raising blind-fire IK"));
        Capture(TEXT("dry_fire")); Go(15); break;
    case 15:
        if (Elapsed<1.f) return;
        Check(Combat->GetDryFireCount()==Dry+1 && Combat->GetShotsFired()==Shots && Combat->GetMagazine()==0 && Combat->GetReserve()==Reserve && Target->GetTotalHitCount()==Hits,
            TEXT("Held empty trigger causes one presentation and no shot, damage or ammo change"));
        Key(EKeys::LeftMouseButton,false); Go(16); break;
    case 16:
        if (Elapsed<.15f) return;
        Pulse(EKeys::LeftMouseButton); Go(17); break;
    case 17:
        if (Elapsed<.15f) return;
        Check(Combat->GetDryFireCount()==Dry+2 && Combat->IsDryFiring(),TEXT("A fresh empty trigger press can replay feedback"));
        Pulse(EKeys::R); Go(18); break;
    case 18:
        if (Elapsed<.2f) return;
        Check(Combat->IsReloading()&&!Combat->IsDryFiring(),TEXT("Reload interrupts dry fire immediately without action lock"));
        Go(19); break;
    case 19:
        if (Elapsed<.3f || Combat->IsReloading()) return;
        Check(Combat->GetMagazine()==Combat->GetWeaponData()->MagazineCapacity,TEXT("Reload after dry fire transfers ammunition successfully"));
        Magazine=Combat->GetMagazine(); Reserve=Combat->GetReserve();
        Anim->bCrouchEquipPoseReady=false; Pulse(IsPistol()?EKeys::One:EKeys::Two); Go(20); break;
    case 20:
        if (Elapsed<.3f) return;
        Check(Combat->GetWeaponKind()==Kind && Combat->GetActionState()==EGunnerCombatAction::Idle,TEXT("Missing protective equip capability rejects crouched switch"));
        Anim->bCrouchEquipPoseReady=bEquipReady; BeginEquip(false); Go(21); break;
    case 21:
    case 23:
        ObserveEquip();
        if (Elapsed<.4f || Combat->GetActionState()==EGunnerCombatAction::Equipping) return;
        UE_LOG(LogTemp,Display,TEXT("GUNNER_POLISH_EQUIP scenario=%d phase=%d head_max=%.2f hand_travel=%.2f"),Scenario,Phase,MaxHead,MaxHandTravel);
        Check(bObserved && bPoseSafe && MaxHandTravel>6.f && (!IsCover() || MaxHead<110.f),TEXT("Evaluated equip moves arms while genuine crouch and cover head clearance persist"));
        Check(Combat->GetWeaponKind()!=Kind,TEXT("Crouched switch selects the other weapon"));
        if (Phase==23) { Go(24); break; }
        BeginEquip(true); Go(22); break;
    case 22:
        if (Elapsed<.3f) return;
        Check(Combat->GetActionState()==EGunnerCombatAction::Equipping && Combat->GetWeaponKind()==Kind,TEXT("Reverse crouched switch starts"));
        Anim->Montage_Stop(.05f,Selected); Go(25); break;
    case 25:
        if (Elapsed<.5f) return;
        Check(Combat->GetActionState()==EGunnerCombatAction::Idle && Combat->GetMagazine()==Magazine && Combat->GetReserve()==Reserve && Anim->CrouchHandlingAlpha<.05f,
            TEXT("Interrupted equip clears action/layer and preserves selected weapon ammunition"));
        if (IsCover()) { Key(EKeys::RightMouseButton,true); Go(26); }
        else Go(24);
        break;
    case 26:
        if (Elapsed<.6f) return;
        Check(Combat->IsAiming()&&!Character->bIsCrouched,TEXT("Held low-cover ADS established before switching"));
        BeginEquip(false); Go(23); break;
    case 24:
        if (Elapsed<.6f) return;
        if (IsCover()) Check(Combat->IsAiming()&&!Character->bIsCrouched,TEXT("Held ADS resumes after protected weapon switch completes"));
        else Check(Character->bIsCrouched&&Anim->CrouchHandlingAlpha<.05f,TEXT("Free crouch survives switch and handling layer clears"));
        Key(EKeys::RightMouseButton,false);
        if (++Scenario==4) Finish(); else Prepare();
        break;
    default: Check(false,TEXT("Known phase")); Finish(); break;
    }
#endif
}
