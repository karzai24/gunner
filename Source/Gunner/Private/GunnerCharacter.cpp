#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "GunnerMotionSettings.h"
#include "GunnerCombatComponent.h"
#include "GunnerWeaponData.h"
#include "GunnerCoverComponent.h"
#include "GunnerDodgeComponent.h"
#include "GunnerAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

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
    Super::BeginPlay();
    GetMesh()->AddTickPrerequisiteActor(this);
    if (MotionSettings)
    {
        auto* Movement = GetCharacterMovement();
        Movement->GetNavAgentPropertiesRef().bCanCrouch = MotionSettings->bCrouchReady;
        Movement->MaxWalkSpeedCrouched = MotionSettings->CrouchSpeed;
        Movement->bOrientRotationToMovement = false;
    }
}

bool AGunnerCharacter::IsInCover() const { return Cover && Cover->IsAttached(); }

void AGunnerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!MotionSettings) return;
    auto* Movement = GetCharacterMovement();
    const bool InCover = IsInCover();
    const bool Dodging = Dodge->IsDodging();
    if (InCover) Cover->SetPeekDesired(bAimHeld && !Combat->IsMeleeing(), ShoulderSide);
    bSprinting = bSprintHeld && MotionSettings->bSprintReady && MoveAxis.Y > 0.4f && !bAimHeld
        && !bIsCrouched && !InCover && !Dodging && Movement->IsMovingOnGround();
    Combat->SetCombatBlocked(bSprinting || Dodging || !Controller || Movement->IsFalling());
    Combat->SetFireBlocked((InCover && (!bAimHeld || !Cover->CanPeek(ShoulderSide))) || (bIsCrouched && !bAimHeld));
    if (bAimHeld && !bSprinting && (!InCover || Cover->CanPeek(ShoulderSide))) Combat->StartAim();
    const bool Aiming = Combat->IsAiming();
    if (InCover && Cover->IsLowCover())
    {
        if (Aiming) UnCrouch(); else Crouch();
    }
    Movement->MaxWalkSpeed = bSprinting ? MotionSettings->SprintSpeed :
        (InCover ? Cover->MoveSpeed : (Aiming ? MotionSettings->AimSpeed : MotionSettings->WalkSpeed));
    Movement->bOrientRotationToMovement = bSprinting || (bIsCrouched && !Aiming && !MotionSettings->bDirectionalCrouchReady);
    if (Controller && !Dodging && !Movement->bOrientRotationToMovement)
    {
        const FRotator Desired = InCover && !Aiming ? Cover->GetNormal().Rotation() : FRotator(0, Controller->GetControlRotation().Yaw, 0);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, Desired.Yaw, 0), DeltaSeconds, Aiming ? 18.f : 10.f));
    }
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength,
        Aiming ? MotionSettings->AimArmLength : ((bSprinting || Dodging) ? 350.f : 320.f), DeltaSeconds, 10.f);
    CameraBoom->SocketOffset.Y = FMath::FInterpTo(CameraBoom->SocketOffset.Y, 50.f * ShoulderSide, DeltaSeconds, 10.f);
    CameraBoom->TargetOffset.Z = FMath::FInterpTo(CameraBoom->TargetOffset.Z, Dodging ? 5.f : (bIsCrouched ? 45.f : 65.f), DeltaSeconds, 10.f);
    FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView,
        Aiming ? MotionSettings->AimFOV : (bSprinting ? 82.f : 75.f), DeltaSeconds, 10.f);
    if (auto* Anim = Cast<UGunnerAnimInstance>(GetMesh()->GetAnimInstance()))
        Anim->SetGameplayState(Aiming, bSprinting, InCover, Combat->GetWeaponKind() == EGunnerWeaponKind::Pistol);
}

void AGunnerCharacter::UnPossessed()
{
    bSprintHeld = bSprinting = bAimHeld = false;
    MoveAxis = FVector2D::ZeroVector;
    Combat->StopAllActions();
    Dodge->CancelDodge();
    Cover->Detach();
    Super::UnPossessed();
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
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
    if (InputConfig->Aim)
    {
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Started, this, &AGunnerCharacter::StartAim);
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Completed, this, &AGunnerCharacter::StopAim);
        Input->BindAction(InputConfig->Aim, ETriggerEvent::Canceled, this, &AGunnerCharacter::StopAim);
    }
    if (InputConfig->Fire)
    {
        Input->BindAction(InputConfig->Fire, ETriggerEvent::Started, Combat.Get(), &UGunnerCombatComponent::StartFire);
        Input->BindAction(InputConfig->Fire, ETriggerEvent::Completed, Combat.Get(), &UGunnerCombatComponent::StopFire);
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
    if (Combat->IsMeleeing() || Dodge->IsDodging() || Cover->IsPeeking()) return;
    const FRotationMatrix Yaw(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
    FVector Direction = Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X;
    if (MotionSettings && bIsCrouched && Combat->IsAiming() && !MotionSettings->bDirectionalCrouchReady) return;
    if (IsInCover()) Direction = Cover->ConstrainMovement(Direction);
    AddMovementInput(Direction.GetSafeNormal(), FMath::Min(Direction.Size(), 1.f));
}
void AGunnerCharacter::StopMove() { MoveAxis = FVector2D::ZeroVector; }
void AGunnerCharacter::StartAim() { bAimHeld = true; bSprintHeld = false; }
void AGunnerCharacter::StopAim() { bAimHeld = false; Combat->StopAim(); }
void AGunnerCharacter::StartSprint() { bSprintHeld = true; }
void AGunnerCharacter::StopSprint() { bSprintHeld = false; }
void AGunnerCharacter::SwapShoulder() { ShoulderSide *= -1.f; }
void AGunnerCharacter::TryDodge()
{
    if (IsInCover() || Combat->IsMeleeing()) return;
    if (Dodge->TryDodge())
    {
        bAimHeld = bSprintHeld = false;
        Combat->SetCombatBlocked(true);
    }
}
void AGunnerCharacter::ToggleCrouch()
{
    if (!MotionSettings || !MotionSettings->bCrouchReady || Dodge->IsDodging() || Combat->IsMeleeing() || Cover->IsPeeking() || (IsInCover() && Cover->IsLowCover())) return;
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
    const FVector2D Axis = Value.Get<FVector2D>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGunnerCharacter::Traverse()
{
    if (Dodge->IsDodging() || Combat->IsMeleeing()) return;
    if (IsInCover()) { Cover->Detach(); StopAim(); return; }
    if (MotionSettings && MotionSettings->bCoverReady && Controller)
    {
        const FVector Direction = FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X);
        if (Cover->TryAttach(Direction)) { bSprintHeld = false; Combat->StopAllActions(); return; }
    }
    GroundedJump();
}
void AGunnerCharacter::GroundedJump()
{
    if (GetCharacterMovement()->IsMovingOnGround() && !IsInCover() && !bIsCrouched && !Dodge->IsDodging() && !Combat->IsMeleeing()) Jump();
}
