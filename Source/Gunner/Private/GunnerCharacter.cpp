#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "GunnerMotionSettings.h"
#include "GunnerCombatComponent.h"
#include "GunnerWeaponData.h"
#include "GunnerCoverComponent.h"
#include "GunnerDodgeComponent.h"
#include "GunnerAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Engine/SkeletalMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    const UEnhancedPlayerInput* CurrentInput(const ACharacter* Character)
    {
        const auto* Player = Cast<APlayerController>(Character->GetController());
        return Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
    }
    bool HeldInput(const ACharacter* Character, const UInputAction* Action, bool Fallback)
    {
        const auto* Input = CurrentInput(Character);
        return Input && Action ? Input->GetActionValue(Action).Get<bool>() : Fallback;
    }
}

AGunnerCharacter::AGunnerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Combat = CreateDefaultSubobject<UGunnerCombatComponent>(TEXT("Combat"));
    Cover = CreateDefaultSubobject<UGunnerCoverComponent>(TEXT("Cover"));
    Dodge = CreateDefaultSubobject<UGunnerDodgeComponent>(TEXT("Dodge"));
    GetCapsuleComponent()->InitCapsuleSize(36.f, 90.f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 480.f, 0.f);
    Movement->MaxWalkSpeed = 450.f;
    Movement->MinAnalogWalkSpeed = 20.f;
    Movement->MaxAcceleration = 1200.f;
    Movement->BrakingDecelerationWalking = 1600.f;
    Movement->JumpZVelocity = 440.f;
    Movement->AirControl = 0.2f;
    Movement->SetCrouchedHalfHeight(62.f);
    // Blueprint motion settings enable this only with an authored crouch graph.
    Movement->GetNavAgentPropertiesRef().bCanCrouch = false;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.f;
    CameraBoom->TargetOffset = FVector(0.f, 0.f, 65.f);
    CameraBoom->SocketOffset = FVector(0.f, 50.f, 0.f);
    CameraBoom->ProbeSize = 12.f;
    CameraBoom->bUsePawnControlRotation = true;
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->FieldOfView = 75.f;
    FollowCamera->bUsePawnControlRotation = false;
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGunnerCharacter::BeginPlay()
{
    // A local licensed profile is optional and read once at spawn. Fresh clones
    // retain the Blueprint's portable assets; the foundation has no motion profile.
    FString LocalProfile;
    if (MotionSettings && GConfig && !FParse::Param(FCommandLine::Get(), TEXT("GunnerIgnoreLocalMotion"))
        && GConfig->GetString(TEXT("Gunner.LocalMotion"), TEXT("Profile"), LocalProfile, GGameIni)
        && !LocalProfile.IsEmpty())
    {
        if (LocalProfile.StartsWith(TEXT("/Game/Gunner/LicensedLocal/"))
            && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(LocalProfile)))
        {
            auto* Profile = LoadObject<UGunnerMotionSettings>(nullptr, *LocalProfile);
            auto* Class = Profile ? Cast<UAnimBlueprintGeneratedClass>(Profile->AnimationClass.Get()) : nullptr;
            if (Class && Class->IsChildOf(UGunnerAnimInstance::StaticClass()) && GetMesh()->GetSkeletalMeshAsset()
                && Class->GetTargetSkeleton() == GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())
            {
                MotionSettings = Profile;
                GetMesh()->SetAnimInstanceClass(Class);
            }
            else UE_LOG(LogTemp, Warning, TEXT("Gunner local movement profile is incompatible; using the portable character defaults"));
        }
        else UE_LOG(LogTemp, Warning, TEXT("Gunner local movement profile is unavailable; using the portable character defaults"));
    }
    Super::BeginPlay();
    GetMesh()->AddTickPrerequisiteActor(this);
    if (MotionSettings)
    {
        auto* Movement = GetCharacterMovement();
        Movement->GetNavAgentPropertiesRef().bCanCrouch = MotionSettings->bCrouchReady;
        Movement->MaxWalkSpeedCrouched = MotionSettings->CrouchSpeed;
        Movement->bOrientRotationToMovement = false;
        Movement->MaxAcceleration = MotionSettings->Acceleration;
        Movement->BrakingDecelerationWalking = MotionSettings->Braking;
    }
}

bool AGunnerCharacter::IsInCover() const { return Cover && Cover->IsAttached(); }
bool AGunnerCharacter::UsesContextualTraversal() const { return MotionSettings && MotionSettings->bContextualTraversal; }
bool AGunnerCharacter::IsAimHeld() const
{
    return HeldInput(this, InputConfig ? InputConfig->Aim : nullptr, bAimHeld);
}

bool AGunnerCharacter::HasDirectionalCrouch() const
{
    const auto* Anim = Cast<UGunnerAnimInstance>(GetMesh()->GetAnimInstance());
    // The installed moving crouch source rises above the 115 cm low-cover top.
    // Preserve the proven protective forward gait there until a lower directional set is authored.
    return MotionSettings && MotionSettings->bDirectionalCrouchReady && Anim && Anim->bDirectionalCrouchPoseReady
        && !Cover->IsLowCover();
}

bool AGunnerCharacter::WantsSprint() const
{
    // Enhanced Input evaluates values before callbacks, but Started callbacks run
    // before Completed. Query values so release+fire in one frame is not lost.
    const bool SprintHeld = HeldInput(this, InputConfig ? InputConfig->Sprint : nullptr, bSprintHeld);
    const bool TraverseHeld = HeldInput(this, InputConfig ? InputConfig->Traverse : nullptr, bTraverseHeld);
    const auto* Input = CurrentInput(this);
    const FVector2D Axis = Input && InputConfig && InputConfig->Move
        ? Input->GetActionValue(InputConfig->Move).Get<FVector2D>() : MoveAxis;
    const bool Held = SprintHeld || (TraverseHeld && MotionSettings && MotionSettings->bContextualTraversal
        && TraverseHeldTime >= MotionSettings->TraverseHoldTime);
    return Held && MotionSettings && MotionSettings->bSprintReady && Axis.Y > 0.4f && !IsAimHeld()
        && !bIsCrouched && !GetCharacterMovement()->bWantsToCrouch && !IsInCover()
        && !Cover->IsTransitioning() && !Dodge->IsDodging() && GetCharacterMovement()->IsMovingOnGround();
}

bool AGunnerCharacter::IsCombatMovementBlocked() const
{
    return !Controller || GetCharacterMovement()->IsFalling() || Cover->IsTransitioning()
        || Dodge->IsDodging() || WantsSprint();
}

FVector AGunnerCharacter::GetMoveDirection() const
{
    if (!Controller) return FVector::ZeroVector;
    const auto* Input = CurrentInput(this);
    const FVector2D Axis = Input && InputConfig && InputConfig->Move
        ? Input->GetActionValue(InputConfig->Move).Get<FVector2D>() : MoveAxis;
    const FRotationMatrix Yaw(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
    return (Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X).GetClampedToMaxSize(1.f);
}

void AGunnerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!MotionSettings) return;
    auto* Movement = GetCharacterMovement();
    const bool AimHeld = IsAimHeld();
    const bool SprintHeld = HeldInput(this, InputConfig ? InputConfig->Sprint : nullptr, bSprintHeld);
    const bool TraverseHeld = HeldInput(this, InputConfig ? InputConfig->Traverse : nullptr, bTraverseHeld);
    if (TraverseHeld) TraverseHeldTime += DeltaSeconds;
    if (MotionSettings->bContextualTraversal && TraverseHeld && TraverseHeldTime >= MotionSettings->TraverseHoldTime)
    {
        bTraverseConsumed = true;
        if (!IsInCover() && !Cover->IsTransitioning() && !Dodge->IsDodging() && bIsCrouched) UnCrouch();
    }
    const bool InCover = IsInCover();
    const bool Dodging = Dodge->IsDodging();
    const bool EnteringCover = Cover->IsTransitioning();
    if (InCover) Cover->SetPeekDesired(AimHeld && !Combat->IsMeleeing() && !Combat->IsReloading() && Combat->GetActionState() != EGunnerCombatAction::Equipping, ShoulderSide);
    const bool WasSprinting = bSprinting;
    bSprinting = WantsSprint();
    if (bSprinting && !WasSprinting) SprintHeading = GetActorRotation().Yaw;
    if (JumpPresentation && (InCover || EnteringCover || bIsCrouched || Dodging || bSprinting)) StopJumpPresentation();
    Combat->SetCombatBlocked(IsCombatMovementBlocked());
    const bool LowCoverBlind = InCover && Cover->IsLowCover() && bIsCrouched && !AimHeld;
    Combat->SetFireBlocked((InCover && !LowCoverBlind && (!AimHeld || !Cover->CanPeek(ShoulderSide))) ||
        (bIsCrouched && !AimHeld && !LowCoverBlind));
    if (AimHeld && !IsCombatMovementBlocked() && (!InCover || Cover->CanPeek(ShoulderSide))) Combat->StartAim();
    const bool Aiming = Combat->IsAiming();
    const bool BlindFiring = Combat->IsBlindFiring();
    if (InCover && Cover->IsLowCover())
    {
        if (Aiming) UnCrouch(); else Crouch();
    }
    const bool FastCover = InCover && HasDirectionalCrouch() && (SprintHeld || TraverseHeld) && !AimHeld;
    const float CoverSpeed = FastCover ? MotionSettings->FastCoverSpeed : Cover->MoveSpeed;
    Movement->MaxWalkSpeed = bSprinting ? MotionSettings->SprintSpeed :
        (InCover ? CoverSpeed : (Aiming ? MotionSettings->AimSpeed : MotionSettings->WalkSpeed));
    Movement->MaxWalkSpeedCrouched = FastCover ? MotionSettings->FastCoverSpeed : MotionSettings->CrouchSpeed;
    if (BlindFiring)
    {
        ConsumeMovementInputVector();
        Movement->StopMovementImmediately();
    }
    Movement->bOrientRotationToMovement = bSprinting || EnteringCover || (bIsCrouched && !Aiming && !BlindFiring && !HasDirectionalCrouch());
    if (Controller && !Dodging && !Movement->bOrientRotationToMovement)
    {
        const FRotator Desired = InCover && !Aiming && !BlindFiring ? Cover->GetNormal().Rotation() : FRotator(0, Controller->GetControlRotation().Yaw, 0);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, Desired.Yaw, 0), DeltaSeconds, (Aiming || BlindFiring) ? 18.f : 10.f));
    }
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength,
        Aiming ? MotionSettings->AimArmLength : ((bSprinting || Dodging) ? 350.f : 320.f), DeltaSeconds, 10.f);
    CameraBoom->SocketOffset.Y = FMath::FInterpTo(CameraBoom->SocketOffset.Y, 50.f * ShoulderSide, DeltaSeconds, 10.f);
    CameraBoom->TargetOffset.Z = FMath::FInterpTo(CameraBoom->TargetOffset.Z,
        Dodging ? 5.f : (BlindFiring ? 90.f : (bIsCrouched ? 45.f : (bSprinting && MotionSettings->bContextualTraversal ? 35.f : 65.f))), DeltaSeconds, 10.f);
    FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView,
        Aiming ? MotionSettings->AimFOV : (bSprinting ? 82.f : 75.f), DeltaSeconds, 10.f);
    if (auto* Anim = Cast<UGunnerAnimInstance>(GetMesh()->GetAnimInstance()))
        Anim->SetGameplayState(Aiming, bSprinting, InCover, Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol);
    if (MotionSettings->bContextualTraversal && MotionSettings->bCoverReady && bSprinting
        && GetWorld()->GetTimeSeconds() >= NextAutoCoverTime)
    {
        NextAutoCoverTime = GetWorld()->GetTimeSeconds() + 0.08f;
        if (Cover->TryAttach(FRotator(0.f, SprintHeading, 0.f).Vector(), true))
        {
            Combat->StopAllActions();
            bTraverseConsumed = true;
        }
    }
}

void AGunnerCharacter::UnPossessed()
{
    bSprintHeld = bSprinting = bAimHeld = bTraverseHeld = false;
    MoveAxis = FVector2D::ZeroVector;
    Combat->StopAllActions();
    StopJumpPresentation();
    Dodge->CancelDodge();
    Cover->Detach();
    Super::UnPossessed();
}

void AGunnerCharacter::StopJumpPresentation()
{
    if (JumpPresentation && GetMesh()->GetAnimInstance())
        GetMesh()->GetAnimInstance()->Montage_Stop(0.06f, JumpPresentation);
    JumpPresentation = nullptr;
}

void AGunnerCharacter::PlayJumpPresentation(UAnimMontage* Montage)
{
    StopJumpPresentation();
    if (Montage && GetMesh()->GetAnimInstance() && GetMesh()->GetAnimInstance()->Montage_Play(Montage, 1.f) > 0.f)
        JumpPresentation = Montage;
}

void AGunnerCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    Combat->StopAllActions();
    const auto* Weapon = Combat->GetWeaponData();
    PlayJumpPresentation(Weapon ? Weapon->JumpStartMontage.Get() : nullptr);
}

void AGunnerCharacter::Landed(const FHitResult& Hit)
{
    const float ImpactSpeed = -GetVelocity().Z;
    Super::Landed(Hit);
    const auto* Weapon = Combat->GetWeaponData();
    if (Controller && ImpactSpeed > 100.f && !IsInCover() && !bIsCrouched && !Dodge->IsDodging())
        PlayJumpPresentation(Weapon ? Weapon->JumpLandMontage.Get() : nullptr);
    else StopJumpPresentation();
}

void AGunnerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!ensureMsgf(Input && InputConfig && InputConfig->Move && InputConfig->MouseLook &&
        InputConfig->StickLook && InputConfig->Traverse, TEXT("Gunner pawn needs a complete input data asset"))) return;
    Input->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &AGunnerCharacter::Move);
    Input->BindAction(InputConfig->Move, ETriggerEvent::Completed, this, &AGunnerCharacter::StopMove);
    Input->BindAction(InputConfig->Move, ETriggerEvent::Canceled, this, &AGunnerCharacter::StopMove);
    Input->BindAction(InputConfig->MouseLook, ETriggerEvent::Triggered, this, &AGunnerCharacter::MouseLook);
    Input->BindAction(InputConfig->StickLook, ETriggerEvent::Triggered, this, &AGunnerCharacter::StickLook);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Started, this, &AGunnerCharacter::Traverse);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Completed, this, &AGunnerCharacter::EndTraverse);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Canceled, this, &AGunnerCharacter::CancelTraverse);
    if (InputConfig->Aim)
    {
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Started, this, &AGunnerCharacter::StartAim);
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Completed, this, &AGunnerCharacter::StopAim);
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Canceled, this, &AGunnerCharacter::StopAim);
    }
    if (InputConfig->Fire)
    {
        Input->BindAction(InputConfig->Fire, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::StartFire);
        Input->BindAction(InputConfig->Fire, ETriggerEvent::Completed, Combat.Get(), &UGunnerCombatComponent::ReleaseFire);
        Input->BindAction(InputConfig->Fire, ETriggerEvent::Canceled, Combat.Get(), &UGunnerCombatComponent::StopFire);
    }
    if (InputConfig->Sprint)
    {
        Input->BindAction(InputConfig->Sprint, ETriggerEvent::Started, this, &AGunnerCharacter::StartSprint);
        Input->BindAction(InputConfig->Sprint, ETriggerEvent::Completed, this, &AGunnerCharacter::StopSprint);
        Input->BindAction(InputConfig->Sprint, ETriggerEvent::Canceled, this, &AGunnerCharacter::StopSprint);
    }
    if (InputConfig->Reload) Input->BindAction(InputConfig->Reload, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::Reload);
    if (InputConfig->Rifle) Input->BindAction(InputConfig->Rifle, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::EquipRifle);
    if (InputConfig->Pistol) Input->BindAction(InputConfig->Pistol, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::EquipPistol);
    if (InputConfig->CycleWeapon) Input->BindAction(InputConfig->CycleWeapon, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::CycleWeapon);
    if (InputConfig->Melee) Input->BindAction(InputConfig->Melee, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::Melee);
    if (InputConfig->CrouchToggle) Input->BindAction(InputConfig->CrouchToggle, ETriggerEvent::Started, this, &AGunnerCharacter::ToggleCrouch);
    if (InputConfig->ShoulderSwap) Input->BindAction(InputConfig->ShoulderSwap, ETriggerEvent::Started, this, &AGunnerCharacter::SwapShoulder);
    if (InputConfig->Dodge) Input->BindAction(InputConfig->Dodge, ETriggerEvent::Started, this, &AGunnerCharacter::TryDodge);
    if (InputConfig->Jump)
    {
        Input->BindAction(InputConfig->Jump, ETriggerEvent::Started, this, &AGunnerCharacter::GroundedJump);
        Input->BindAction(InputConfig->Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        Input->BindAction(InputConfig->Jump, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
    }
}

void AGunnerCharacter::Move(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    MoveAxis = Axis;
    if (Combat->IsMeleeing() || Combat->IsBlindFiring() || Dodge->IsDodging() || Cover->IsPeeking() || Cover->IsTransitioning()) return;
    const FRotationMatrix Yaw(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
    FVector Direction = Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X;
    if (MotionSettings && bIsCrouched && Combat->IsAiming() && !HasDirectionalCrouch()) return;
    if (IsInCover())
    {
        if (MotionSettings && MotionSettings->bContextualTraversal && !IsAimHeld()
            && FVector::DotProduct(Direction.GetSafeNormal2D(), Cover->GetNormal()) > 0.65f)
        {
            Cover->Detach();
            Combat->StopAllActions();
            NextAutoCoverTime = GetWorld()->GetTimeSeconds() + 0.3f;
        }
        else Direction = Cover->ConstrainMovement(Direction);
    }
    if (MotionSettings && MotionSettings->bContextualTraversal && WantsSprint())
    {
        if (!bSprinting) SprintHeading = GetActorRotation().Yaw;
        SprintHeading = FMath::FixedTurn(SprintHeading, Direction.Rotation().Yaw,
            MotionSettings->SprintTurnRate * GetWorld()->GetDeltaSeconds());
        Direction = FRotator(0.f, SprintHeading, 0.f).Vector() * FMath::Min(Axis.Size(), 1.f);
    }
    AddMovementInput(Direction.GetSafeNormal(), FMath::Min(Direction.Size(), 1.f));
}
void AGunnerCharacter::StopMove() { MoveAxis = FVector2D::ZeroVector; }
void AGunnerCharacter::StartAim()
{
    if (Combat->IsBlindFiring()) Combat->StopFire();
    bAimHeld = true;
    bSprintHeld = false;
}
void AGunnerCharacter::StopAim() { bAimHeld = false; Combat->StopAim(); }
void AGunnerCharacter::StartSprint()
{
    bSprintHeld = true;
    if (MotionSettings && MotionSettings->bContextualTraversal && !IsInCover() && !Cover->IsTransitioning() && !Dodge->IsDodging()) UnCrouch();
}
void AGunnerCharacter::StopSprint() { bSprintHeld = false; }
void AGunnerCharacter::SwapShoulder() { ShoulderSide *= -1.f; }
void AGunnerCharacter::TryDodge()
{
    if (Cover->IsTransitioning() || Combat->IsMeleeing()) return;
    if (Dodge->TryDodge(GetMoveDirection()))
    {
        bAimHeld = bSprintHeld = false;
        Combat->SetCombatBlocked(true);
    }
}
void AGunnerCharacter::ToggleCrouch()
{
    if (!MotionSettings || !MotionSettings->bCrouchReady || Dodge->IsDodging() || Cover->IsTransitioning() || Combat->IsMeleeing() || Cover->IsPeeking() || (IsInCover() && Cover->IsLowCover())) return;
    if (Combat->IsReloading() || Combat->GetActionState() == EGunnerCombatAction::Equipping) return;
    bSprintHeld = false;
    if (bIsCrouched || GetCharacterMovement()->bWantsToCrouch) UnCrouch(); else Crouch();
}

void AGunnerCharacter::MouseLook(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGunnerCharacter::StickLook(const FInputActionValue& Value)
{
    const float AimScale = IsAimHeld() && InputConfig
        ? FMath::Clamp(InputConfig->StickAimSensitivityScale, .1f, 1.f) : 1.f;
    const FVector2D Axis = Value.Get<FVector2D>() * StickLookDegreesPerSecond * AimScale * GetWorld()->GetDeltaSeconds();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGunnerCharacter::Traverse()
{
    bTraverseHeld = true;
    TraverseHeldTime = 0.f;
    bTraverseConsumed = false;
    if (Cover->IsTransitioning())
    {
        Cover->CancelTransition();
        bTraverseConsumed = true;
        NextAutoCoverTime = GetWorld()->GetTimeSeconds() + 0.3f;
        return;
    }
    if (Dodge->IsDodging() || Combat->IsMeleeing()) { bTraverseConsumed = true; return; }
    if (IsInCover())
    {
        bTraverseConsumed = true;
        const FVector Direction = GetMoveDirection();
        // Holding traversal while travelling along a wall accelerates the authored
        // cover gait. Away input still detaches and E always requests a guarded roll.
        if (MotionSettings && MotionSettings->bContextualTraversal && Direction.SizeSquared() > 0.1f
            && FVector::DotProduct(Direction.GetSafeNormal2D(), Cover->GetNormal()) < 0.65f) return;
        Combat->StopFire(); Cover->Detach(); StopAim();
        NextAutoCoverTime = GetWorld()->GetTimeSeconds() + 0.3f;
        return;
    }
    if (MotionSettings && MotionSettings->bCoverReady && Controller)
    {
        const FVector Direction = FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X);
        if (Cover->TryAttach(Direction, WantsSprint())) { bSprintHeld = false; bTraverseConsumed = true; Combat->StopAllActions(); return; }
    }
    if (!MotionSettings || !MotionSettings->bContextualTraversal) { bTraverseConsumed = true; GroundedJump(); }
}
void AGunnerCharacter::EndTraverse()
{
    const bool Roll = MotionSettings && MotionSettings->bContextualTraversal && bTraverseHeld
        && !bTraverseConsumed && TraverseHeldTime < MotionSettings->TraverseHoldTime;
    CancelTraverse();
    if (Roll) TryDodge();
}
void AGunnerCharacter::CancelTraverse()
{
    bTraverseHeld = false;
    TraverseHeldTime = 0.f;
    StopJumping();
}
void AGunnerCharacter::GroundedJump()
{
    if (GetCharacterMovement()->IsMovingOnGround() && !IsInCover() && !Cover->IsTransitioning() && !bIsCrouched && !Dodge->IsDodging() && !Combat->IsMeleeing()) Jump();
}
